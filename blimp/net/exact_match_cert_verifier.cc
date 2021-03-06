// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "blimp/net/exact_match_cert_verifier.h"

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/x509_certificate.h"

namespace blimp {

ExactMatchCertVerifier::ExactMatchCertVerifier(
    scoped_refptr<net::X509Certificate> engine_cert)
    : engine_cert_(std::move(engine_cert)) {
  DCHECK(engine_cert_);

  net::SHA1HashValue sha1_hash;
  sha1_hash = net::X509Certificate::CalculateFingerprint(
      engine_cert_->os_cert_handle());
  engine_cert_hashes_.push_back(net::HashValue(sha1_hash));

  net::SHA256HashValue sha256_hash;
  sha256_hash = net::X509Certificate::CalculateFingerprint256(
      engine_cert_->os_cert_handle());
  engine_cert_hashes_.push_back(net::HashValue(sha256_hash));
}

ExactMatchCertVerifier::~ExactMatchCertVerifier() {}

int ExactMatchCertVerifier::Verify(net::X509Certificate* cert,
                                   const std::string& hostname,
                                   const std::string& ocsp_response,
                                   int flags,
                                   net::CRLSet* crl_set,
                                   net::CertVerifyResult* verify_result,
                                   const net::CompletionCallback& callback,
                                   scoped_ptr<Request>* out_req,
                                   const net::BoundNetLog& net_log) {
  verify_result->Reset();
  verify_result->verified_cert = engine_cert_;

  if (!cert->Equals(engine_cert_.get())) {
    verify_result->cert_status = net::CERT_STATUS_INVALID;
    return net::ERR_CERT_INVALID;
  }

  verify_result->public_key_hashes = engine_cert_hashes_;
  return net::OK;
}

}  // namespace blimp
