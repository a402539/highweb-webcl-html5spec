// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "components/policy/core/common/cloud/mock_cloud_policy_client.h"
#include "net/url_request/url_request_context_getter.h"
#include "policy/proto/device_management_backend.pb.h"

namespace em = enterprise_management;

namespace policy {

MockCloudPolicyClient::MockCloudPolicyClient()
    : CloudPolicyClient(std::string(),
                        std::string(),
                        std::string(),
                        nullptr,
                        nullptr) {}

MockCloudPolicyClient::~MockCloudPolicyClient() {}

void MockCloudPolicyClient::SetDMToken(const std::string& token) {
  dm_token_ = token;
}

void MockCloudPolicyClient::SetPolicy(const std::string& policy_type,
                                      const std::string& settings_entity_id,
                                      const em::PolicyFetchResponse& policy) {
  em::PolicyFetchResponse*& response =
      responses_[std::make_pair(policy_type, settings_entity_id)];
  delete response;
  response = new enterprise_management::PolicyFetchResponse(policy);
}

void MockCloudPolicyClient::SetFetchedInvalidationVersion(
    int64_t fetched_invalidation_version) {
  fetched_invalidation_version_ = fetched_invalidation_version;
}

void MockCloudPolicyClient::SetStatus(DeviceManagementStatus status) {
  status_ = status;
}

MockCloudPolicyClientObserver::MockCloudPolicyClientObserver() {}

MockCloudPolicyClientObserver::~MockCloudPolicyClientObserver() {}

}  // namespace policy
