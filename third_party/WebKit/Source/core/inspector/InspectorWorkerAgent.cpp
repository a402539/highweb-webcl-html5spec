/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/inspector/InspectorWorkerAgent.h"

#include "core/inspector/IdentifiersFactory.h"
#include "core/inspector/InspectedFrames.h"
#include "core/inspector/InstrumentingAgents.h"
#include "core/inspector/PageConsoleAgent.h"
#include "platform/weborigin/KURL.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/RefPtr.h"
#include "wtf/text/WTFString.h"

namespace blink {

namespace WorkerAgentState {
static const char workerInspectionEnabled[] = "workerInspectionEnabled";
static const char waitForDebuggerOnStart[] = "waitForDebuggerOnStart";
};

PassOwnPtrWillBeRawPtr<InspectorWorkerAgent> InspectorWorkerAgent::create(InspectedFrames* inspectedFrames, PageConsoleAgent* consoleAgent)
{
    return adoptPtrWillBeNoop(new InspectorWorkerAgent(inspectedFrames, consoleAgent));
}

InspectorWorkerAgent::InspectorWorkerAgent(InspectedFrames* inspectedFrames, PageConsoleAgent* consoleAgent)
    : InspectorBaseAgent<InspectorWorkerAgent, protocol::Frontend::Worker>("Worker")
    , m_inspectedFrames(inspectedFrames)
    , m_consoleAgent(consoleAgent)
{
}

InspectorWorkerAgent::~InspectorWorkerAgent()
{
#if !ENABLE(OILPAN)
    m_instrumentingAgents->setInspectorWorkerAgent(nullptr);
#endif
}

void InspectorWorkerAgent::init()
{
    m_instrumentingAgents->setInspectorWorkerAgent(this);
}

void InspectorWorkerAgent::restore()
{
    if (m_state->booleanProperty(WorkerAgentState::workerInspectionEnabled, false))
        createWorkerAgentClientsForExistingWorkers();
}

void InspectorWorkerAgent::enable(ErrorString*)
{
    if (!m_state->booleanProperty(WorkerAgentState::workerInspectionEnabled, false)) {
        m_state->setBoolean(WorkerAgentState::workerInspectionEnabled, true);
        createWorkerAgentClientsForExistingWorkers();
    }
}

void InspectorWorkerAgent::disable(ErrorString*)
{
    m_state->setBoolean(WorkerAgentState::workerInspectionEnabled, false);
    m_state->setBoolean(WorkerAgentState::waitForDebuggerOnStart, false);
    destroyWorkerAgentClients();
}

void InspectorWorkerAgent::sendMessageToWorker(ErrorString* error, const String& workerId, const String& message)
{
    WorkerAgentClient* client = m_idToClient.get(workerId);
    if (client)
        client->proxy()->sendMessageToInspector(message);
    else
        *error = "Worker is gone";
}

void InspectorWorkerAgent::setWaitForDebuggerOnStart(ErrorString*, bool value)
{
    m_state->setBoolean(WorkerAgentState::waitForDebuggerOnStart, value);
}

void InspectorWorkerAgent::setTracingSessionId(const String& sessionId)
{
    m_tracingSessionId = sessionId;
    if (sessionId.isEmpty())
        return;
    for (auto& info : m_workerInfos)
        info.key->writeTimelineStartedEvent(sessionId, info.value.id);
}

bool InspectorWorkerAgent::shouldWaitForDebuggerOnWorkerStart()
{
    return m_state->booleanProperty(WorkerAgentState::workerInspectionEnabled, false) && m_state->booleanProperty(WorkerAgentState::waitForDebuggerOnStart, false);
}

void InspectorWorkerAgent::didStartWorker(WorkerInspectorProxy* workerInspectorProxy, const KURL& url, bool waitingForDebugger)
{
    String id = "dedicated:" + IdentifiersFactory::createIdentifier();
    m_workerInfos.set(workerInspectorProxy, WorkerInfo(url.getString(), id));
    if (frontend() && m_state->booleanProperty(WorkerAgentState::workerInspectionEnabled, false))
        createWorkerAgentClient(workerInspectorProxy, url.getString(), id, waitingForDebugger);
    if (!m_tracingSessionId.isEmpty())
        workerInspectorProxy->writeTimelineStartedEvent(m_tracingSessionId, id);
}

void InspectorWorkerAgent::workerTerminated(WorkerInspectorProxy* proxy)
{
    m_workerInfos.remove(proxy);
    for (WorkerClients::iterator it = m_idToClient.begin(); it != m_idToClient.end(); ++it) {
        if (proxy == it->value->proxy()) {
            frontend()->workerTerminated(it->key);
            it->value->dispose();
            m_idToClient.remove(it);
            return;
        }
    }
}

void InspectorWorkerAgent::createWorkerAgentClientsForExistingWorkers()
{
    for (auto& info : m_workerInfos)
        createWorkerAgentClient(info.key, info.value.url, info.value.id, false);
}

void InspectorWorkerAgent::destroyWorkerAgentClients()
{
    for (auto& client : m_idToClient)
        client.value->dispose();
    m_idToClient.clear();
}

void InspectorWorkerAgent::didCommitLoadForLocalFrame(LocalFrame* frame)
{
    if (frame != m_inspectedFrames->root())
        return;

    // During navigation workers from old page may die after a while.
    // Usually, it's fine to report them terminated later, but some tests
    // expect strict set of workers, and we reuse renderer between tests.
    for (auto& client : m_idToClient) {
        frontend()->workerTerminated(client.key);
        client.value->dispose();
    }
    m_idToClient.clear();
    m_workerInfos.clear();
}

void InspectorWorkerAgent::createWorkerAgentClient(WorkerInspectorProxy* workerInspectorProxy, const String& url, const String& id, bool waitingForDebugger)
{
    OwnPtrWillBeRawPtr<WorkerAgentClient> client = WorkerAgentClient::create(frontend(), workerInspectorProxy, id, m_consoleAgent);
    WorkerAgentClient* rawClient = client.get();
    m_idToClient.set(id, client.release());
    rawClient->connectToWorker();

    ASSERT(frontend());
    frontend()->workerCreated(id, url, waitingForDebugger);
}

DEFINE_TRACE(InspectorWorkerAgent)
{
#if ENABLE(OILPAN)
    visitor->trace(m_idToClient);
    visitor->trace(m_consoleAgent);
    visitor->trace(m_workerInfos);
#endif
    visitor->trace(m_inspectedFrames);
    InspectorBaseAgent<InspectorWorkerAgent, protocol::Frontend::Worker>::trace(visitor);
}

PassOwnPtrWillBeRawPtr<InspectorWorkerAgent::WorkerAgentClient> InspectorWorkerAgent::WorkerAgentClient::create(protocol::Frontend::Worker* frontend, WorkerInspectorProxy* proxy, const String& id, PageConsoleAgent* consoleAgent)
{
    return adoptPtrWillBeNoop(new InspectorWorkerAgent::WorkerAgentClient(frontend, proxy, id, consoleAgent));
}

InspectorWorkerAgent::WorkerAgentClient::WorkerAgentClient(protocol::Frontend::Worker* frontend, WorkerInspectorProxy* proxy, const String& id, PageConsoleAgent* consoleAgent)
    : m_frontend(frontend)
    , m_proxy(proxy)
    , m_id(id)
    , m_connected(false)
    , m_consoleAgent(consoleAgent)
{
    ASSERT(!proxy->pageInspector());
}
InspectorWorkerAgent::WorkerAgentClient::~WorkerAgentClient()
{
    ASSERT(!m_frontend);
    ASSERT(!m_proxy);
    ASSERT(!m_consoleAgent);
}

void InspectorWorkerAgent::WorkerAgentClient::connectToWorker()
{
    if (m_connected)
        return;
    m_connected = true;
    m_proxy->connectToInspector(this);
}

void InspectorWorkerAgent::WorkerAgentClient::dispose()
{
    if (m_connected) {
        m_connected = false;
        m_proxy->disconnectFromInspector();
    }
    m_frontend = nullptr;
    m_proxy = nullptr;
    m_consoleAgent = nullptr;
}

void InspectorWorkerAgent::WorkerAgentClient::dispatchMessageFromWorker(const String& message)
{
    m_frontend->dispatchMessageFromWorker(m_id, message);
}

void InspectorWorkerAgent::WorkerAgentClient::workerConsoleAgentEnabled(WorkerGlobalScopeProxy* proxy)
{
    m_consoleAgent->workerConsoleAgentEnabled(proxy);
}

DEFINE_TRACE(InspectorWorkerAgent::WorkerAgentClient)
{
    visitor->trace(m_proxy);
    visitor->trace(m_consoleAgent);
    WorkerInspectorProxy::PageInspector::trace(visitor);
}

} // namespace blink
