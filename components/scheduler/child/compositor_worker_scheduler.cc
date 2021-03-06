// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/scheduler/child/compositor_worker_scheduler.h"

#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"

namespace scheduler {

// TODO(scheduler-dev): Get rid of this asap!
namespace {
class CompositorWorkerTaskRunnerWrapper : public TaskQueue {
 public:
  explicit CompositorWorkerTaskRunnerWrapper(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : task_runner_(task_runner) {}

  // TaskQueue implementation:
  void UnregisterTaskQueue() override { NOTREACHED(); }

  bool RunsTasksOnCurrentThread() const override {
    return task_runner_->RunsTasksOnCurrentThread();
  }

  bool PostDelayedTask(const tracked_objects::Location& from_here,
                       const base::Closure& task,
                       base::TimeDelta delay) override {
    return task_runner_->PostDelayedTask(from_here, task, delay);
  }

  bool PostNonNestableDelayedTask(const tracked_objects::Location& from_here,
                                  const base::Closure& task,
                                  base::TimeDelta delay) override {
    return task_runner_->PostNonNestableDelayedTask(from_here, task, delay);
  }

  void SetQueueEnabled(bool enabled) override { NOTREACHED(); }

  bool IsQueueEnabled() const override {
    NOTREACHED();
    return true;
  }

  bool IsEmpty() const override {
    NOTREACHED();
    return false;
  };

  bool HasPendingImmediateWork() const override {
    NOTREACHED();
    return false;
  };

  bool NeedsPumping() const override {
    NOTREACHED();
    return false;
  };

  const char* GetName() const override {
    NOTREACHED();
    return nullptr;
  };

  void SetQueuePriority(QueuePriority priority) override { NOTREACHED(); }

  QueuePriority GetQueuePriority() const override {
    NOTREACHED();
    return QueuePriority::NORMAL_PRIORITY;
  };

  void SetPumpPolicy(PumpPolicy pump_policy) override { NOTREACHED(); }

  PumpPolicy GetPumpPolicy() const override {
    NOTREACHED();
    return PumpPolicy::AUTO;
  };

  void PumpQueue(bool may_post_dowork) override { NOTREACHED(); }

  void AddTaskObserver(
      base::MessageLoop::TaskObserver* task_observer) override {
    NOTREACHED();
  }

  void RemoveTaskObserver(
      base::MessageLoop::TaskObserver* task_observer) override {
    NOTREACHED();
  }

  void SetTimeDomain(TimeDomain* domain) override { NOTREACHED(); }

  TimeDomain* GetTimeDomain() const override {
    NOTREACHED();
    return nullptr;
  }

 private:
  ~CompositorWorkerTaskRunnerWrapper() override {}

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};
}  // namespace

CompositorWorkerScheduler::CompositorWorkerScheduler(base::Thread* thread)
    : thread_(thread) {}

CompositorWorkerScheduler::~CompositorWorkerScheduler() {}

void CompositorWorkerScheduler::Init() {}

scoped_refptr<TaskQueue> CompositorWorkerScheduler::DefaultTaskRunner() {
  // TODO(sad): Implement a more robust scheduler that can do idle tasks for GC
  // without regressing performance of the rest of the system.
  return make_scoped_refptr(
      new CompositorWorkerTaskRunnerWrapper(thread_->task_runner()));
}

scoped_refptr<scheduler::SingleThreadIdleTaskRunner>
CompositorWorkerScheduler::IdleTaskRunner() {
  // TODO(sad): Not having a task-runner for idle tasks means v8 has to fallback
  // to inline GC, which might cause jank.
  return nullptr;
}

bool CompositorWorkerScheduler::CanExceedIdleDeadlineIfRequired() const {
  return false;
}

bool CompositorWorkerScheduler::ShouldYieldForHighPriorityWork() {
  return false;
}

void CompositorWorkerScheduler::AddTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  thread_->message_loop()->AddTaskObserver(task_observer);
}

void CompositorWorkerScheduler::RemoveTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  thread_->message_loop()->RemoveTaskObserver(task_observer);
}

void CompositorWorkerScheduler::Shutdown() {}

}  // namespace scheduler
