/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
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
 *
 */

#include "server_credentials.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hphp/runtime/ext/extension.h"

#include <grpc/grpc.h>
#include <grpc/grpc_security.h>

namespace HPHP {

const StaticString s_ServerCredentialsWrapper("ServerCredentialsWrapper");

class ServerCredentialsWrapper {
  private:
    grpc_server_credentials* wrapped{nullptr};
  public:
    ServerCredentialsWrapper() {}
    ~ServerCredentialsWrapper() { sweep(); }

    void new(grpc_server_credentials* server_credentials) {
      memcpy(wrapped, server_credentials, sizeof(grpc_server_credentials));
    }

    void sweep() {
      if (wrapped) {
        grpc_server_credentials_release(wrapped);
        req::free(wrapped);
        wrapped = nullptr;
      }
    }

    grpc_timespec* getWrapped() {
      return wrapped;
    }
}

Object HHVM_METHOD(ServerCredentials, createSsl,
  const String& pem_root_certs,
  const String& pem_private_key,
  const String& pem_cert_chain) {
  auto serverCredentialsWrapper = Native::data<ServerCredentialsWrapper>(this_);

  /* TODO: add a client_certificate_request field in ServerCredentials and pass
   * it as the last parameter. */
  grpc_server_credentials *server_credentials = grpc_ssl_server_credentials_create_ex(
    pem_root_certs.toCppString(),
    &pem_key_cert_pair.toCppString(),
    1,
    GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE,
    NULL
  );

  serverCredentialsWrapper->new(server_credentials);

  return Object(std::move(serverCredentialsWrapper));
}

} // namespace HPHP
