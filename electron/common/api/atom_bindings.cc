// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/common/api/atom_bindings.h"

#include <algorithm>
#include <string>
#include <iostream>

#include "electron/common/atom_version.h"
#include "electron/common/chrome_version.h"
#include "electron/common/native_mate_converters/string16_converter.h"
#include "base/logging.h"
#include "base/process/process_metrics.h"
#include "native_mate/dictionary.h"

#include "electron/common/node_includes.h"

namespace electron {

namespace {

// Dummy class type that used for crashing the program.
struct DummyClass { bool crash; };

void Crash() {
  static_cast<DummyClass*>(NULL)->crash = true;
}

void Hang() {
  for (;;)
    base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(1));
}

// Called when there is a fatal error in V8, we just crash the process here so
// we can get the stack trace.
void FatalErrorCallback(const char* location, const char* message) {
  LOG(ERROR) << "Fatal error in V8: " << location << " " << message;
  Crash();
}

void Log(const base::string16& message) {
  std::cout << message << std::flush;
}

}  // namespace


ElectronBindings::ElectronBindings() {
  uv_async_init(uv_default_loop(), &call_next_tick_async_, OnCallNextTick);
  call_next_tick_async_.data = this;
}

ElectronBindings::~ElectronBindings() {
}

void ElectronBindings::BindTo(v8::Isolate* isolate,
                          v8::Local<v8::Object> process) {
  v8::V8::SetFatalErrorHandler(FatalErrorCallback);

  mate::Dictionary dict(isolate, process);
  dict.SetMethod("crash", &Crash);
  dict.SetMethod("hang", &Hang);
  dict.SetMethod("log", &Log);
#if defined(OS_POSIX)
  dict.SetMethod("setFdLimit", &base::SetFdLimit);
#endif
  dict.SetMethod("activateUvLoop",
      base::Bind(&ElectronBindings::ActivateUVLoop, base::Unretained(this)));

#if defined(MAS_BUILD)
  dict.Set("mas", true);
#endif

  mate::Dictionary versions;
  if (dict.Get("versions", &versions)) {
    versions.Set(ATOM_PROJECT_NAME, ATOM_VERSION_STRING);
    versions.Set("atom-shell", ATOM_VERSION_STRING);  // For compatibility.
    versions.Set("chrome", CHROME_VERSION_STRING);
  }
}

void ElectronBindings::ActivateUVLoop(v8::Isolate* isolate) {
  node::Environment* env = node::Environment::GetCurrent(isolate);
  if (std::find(pending_next_ticks_.begin(), pending_next_ticks_.end(), env) !=
      pending_next_ticks_.end())
    return;

  pending_next_ticks_.push_back(env);
  uv_async_send(&call_next_tick_async_);
}

// static
void ElectronBindings::OnCallNextTick(uv_async_t* handle) {
  ElectronBindings* self = static_cast<ElectronBindings*>(handle->data);
  for (std::list<node::Environment*>::const_iterator it =
           self->pending_next_ticks_.begin();
       it != self->pending_next_ticks_.end(); ++it) {
    node::Environment* env = *it;
    node::Environment::AsyncCallbackScope callback_scope(env);
    env->KickNextTick(&callback_scope);
  }

  self->pending_next_ticks_.clear();
}

}  // namespace electron
