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

#ifndef GRPC_HHVM_GRPC_COMPLETION_QUEUE_H_
#define GRPC_HHVM_GRPC_COMPLETION_QUEUE_H_

#include <grpc/grpc.h>

#include "hphp/runtime/ext/extension.h"

namespace HPHP {

struct CompletionQueue {
  CompletionQueue();
  ~CompletionQueue();
  grpc_completion_queue *getQueue();

  /* The global completion queue for all operations */
  grpc_completion_queue *completion_queue;

  static DECLARE_THREAD_LOCAL(CompletionQueue, tl_obj);
};

}

#endif /* GRPC_HHVM_GRPC_COMPLETION_QUEUE_H_ */
