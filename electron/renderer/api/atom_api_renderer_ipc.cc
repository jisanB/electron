// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/common/api/api_messages.h"
#include "electron/common/native_mate_converters/string16_converter.h"
#include "electron/common/native_mate_converters/value_converter.h"
#include "electron/common/node_includes.h"
#include "content/public/renderer/render_view.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

using content::RenderView;
using blink::WebLocalFrame;
using blink::WebView;

namespace {

RenderView* GetCurrentRenderView() {
  WebLocalFrame* frame = WebLocalFrame::frameForCurrentContext();
  if (!frame)
    return NULL;

  WebView* view = frame->view();
  if (!view)
    return NULL;  // can happen during closing.

  return RenderView::FromWebView(view);
}

void Send(mate::Arguments* args,
          const base::string16& channel,
          const base::ListValue& arguments) {
  RenderView* render_view = GetCurrentRenderView();
  if (render_view == NULL)
    return;

  bool success = render_view->Send(new ElectronViewHostMsg_Message(
      render_view->GetRoutingID(), channel, arguments));

  if (!success)
    args->ThrowError("Unable to send ElectronViewHostMsg_Message");
}

base::string16 SendSync(mate::Arguments* args,
                        const base::string16& channel,
                        const base::ListValue& arguments) {
  base::string16 json;

  RenderView* render_view = GetCurrentRenderView();
  if (render_view == NULL)
    return json;

  IPC::SyncMessage* message = new ElectronViewHostMsg_Message_Sync(
      render_view->GetRoutingID(), channel, arguments, &json);
  bool success = render_view->Send(message);

  if (!success)
    args->ThrowError("Unable to send ElectronViewHostMsg_Message_Sync");

  return json;
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("send", &Send);
  dict.SetMethod("sendSync", &SendSync);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_renderer_ipc, Initialize)
