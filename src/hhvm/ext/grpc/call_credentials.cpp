/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "channel_credentials.h"
#include "call_credentials.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/base/req-containers.h"
#include "hphp/runtime/base/type-resource.h"
#include "hphp/runtime/base/object-data.h"
#include "hphp/runtime/vm/native-data.h"
#include "hphp/runtime/base/builtin-functions.h"
#include "hphp/runtime/base/array-init.h"
#include "hphp/util/process.h"

#include "call.h"

#include <grpc/grpc.h>
#include <grpc/grpc_security.h>
#include <grpc/support/alloc.h>

namespace HPHP {

Class* CallCredentialsData::s_class = nullptr;
const StaticString CallCredentialsData::s_className("Grpc\\CallCredentials");

IMPLEMENT_GET_CLASS(CallCredentialsData);

CallCredentialsData::CallCredentialsData() {}
CallCredentialsData::~CallCredentialsData() { sweep(); }

void CallCredentialsData::init(grpc_call_credentials* call_credentials) {
  wrapped = call_credentials;
}

void CallCredentialsData::sweep() {
  if (wrapped) {
    grpc_call_credentials_release(wrapped);
    wrapped = nullptr;
  }
}

grpc_call_credentials* CallCredentialsData::getWrapped() {
  return wrapped;
}

/**
 * Create composite credentials from two existing credentials.
 * @param CallCredentials $cred1_obj The first credential
 * @param CallCredentials $cred2_obj The second credential
 * @return CallCredentials The new composite credentials object
 */
Object HHVM_STATIC_METHOD(CallCredentials, createComposite,
  const Object& cred1_obj,
  const Object& cred2_obj) {
  auto callCredentialsData1 = Native::data<CallCredentialsData>(cred1_obj);
  auto callCredentialsData2 = Native::data<CallCredentialsData>(cred2_obj);

  grpc_call_credentials *call_credentials =
        grpc_composite_call_credentials_create(callCredentialsData1->getWrapped(),
                                               callCredentialsData2->getWrapped(),
                                               NULL);

  auto newCallCredentialsObj = Object{CallCredentialsData::getClass()};
  auto newCallCredentialsData = Native::data<CallCredentialsData>(newCallCredentialsObj);
  newCallCredentialsData->init(call_credentials);

  return newCallCredentialsObj;
}

/**
 * Create a call credentials object from the plugin API
 * @param callable $fci The callback
 * @return CallCredentials The new call credentials object
 */
Object HHVM_STATIC_METHOD(CallCredentials, createFromPlugin,
  const Variant& callback) {

  if (!is_callable(callback)) {
    throw_invalid_argument("Callback argument is not a valid callback");
  }

  plugin_state *state;
  state = (plugin_state *)gpr_zalloc(sizeof(plugin_state));
  state->callback = callback;
  state->req_thread_id = Process::GetThreadId();

  grpc_metadata_credentials_plugin plugin;
  plugin.get_metadata = plugin_get_metadata;
  plugin.destroy = plugin_destroy_state;
  plugin.state = (void *)state;
  plugin.type = "";

  auto newCallCredentialsObj = Object{CallCredentialsData::getClass()};
  auto newCallCredentialsData = Native::data<CallCredentialsData>(newCallCredentialsObj);
  newCallCredentialsData->init(grpc_metadata_credentials_create_from_plugin(plugin, NULL));

  return newCallCredentialsObj;
}

// This work done in this function MUST be done on the same thread as the HHVM request
void plugin_do_get_metadata(void *ptr, grpc_auth_metadata_context context,
                             grpc_credentials_plugin_metadata_cb cb,
                             void *user_data) {
  Object returnObj = SystemLib::AllocStdClassObject();
  returnObj.o_set("service_url", String(context.service_url, CopyString));
  returnObj.o_set("method_name", String(context.method_name, CopyString));

  plugin_state *state = (plugin_state *)ptr;

  Variant retval = vm_call_user_func(state->callback, make_packed_array(returnObj));
  if (!retval.isArray()) {
    throw_invalid_argument("Callback return value expected an array.");
    return;
  }

  grpc_status_code code = GRPC_STATUS_OK;

  grpc_metadata_array metadata;
  grpc_metadata_array_init(&metadata);

  if (!hhvm_create_metadata_array(retval.toArray(), &metadata)) {
    code = GRPC_STATUS_INVALID_ARGUMENT;
  }

  /* Pass control back to core */
  cb(user_data, metadata.metadata, metadata.count, code, NULL);

  for (int i = 0; i < metadata.count; i++) {
    grpc_slice_unref(metadata.metadata[i].key);
    grpc_slice_unref(metadata.metadata[i].value);
  }
  grpc_metadata_array_destroy(&metadata);
}

void plugin_get_metadata(void *ptr, grpc_auth_metadata_context context,
                         grpc_credentials_plugin_metadata_cb cb,
                         void *user_data) {
  plugin_state *state = (plugin_state *)ptr;

  plugin_get_metadata_params *params = (plugin_get_metadata_params *)gpr_zalloc(sizeof(plugin_get_metadata_params));

  params->ptr = ptr;
  params->context = context;
  params->cb = cb;
  params->user_data = user_data;

  PluginGetMetadataHandler::getInstance().set(state->req_thread_id, params);
}


void plugin_destroy_state(void *ptr) {
  plugin_state *state = (plugin_state *)ptr;
  gpr_free(state);
}

} // namespace HPHP
