// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SHELL_PUBLIC_CPP_CONNECT_H_
#define MOJO_SHELL_PUBLIC_CPP_CONNECT_H_

#include <utility>

#include "mojo/shell/public/interfaces/interface_provider.mojom.h"

namespace mojo {

// Binds |ptr| to a remote implementation of Interface from |interfaces|.
template <typename Interface>
inline void GetInterface(shell::mojom::InterfaceProvider* interfaces,
                         InterfacePtr<Interface>* ptr) {
  MessagePipe pipe;
  ptr->Bind(InterfacePtrInfo<Interface>(std::move(pipe.handle0), 0u));
  interfaces->GetInterface(Interface::Name_, std::move(pipe.handle1));
}

}  // namespace mojo

#endif  // MOJO_SHELL_PUBLIC_CPP_CONNECT_H_
