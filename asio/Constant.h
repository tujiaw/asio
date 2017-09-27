#pragma once

#include <functional>
#include "google/protobuf/message.h"

class session;
typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
typedef std::shared_ptr<session> SessionPtr;
typedef std::function<void(const SessionPtr&, const MessagePtr&)> Task;
typedef std::function<void(int error, const MessagePtr &reqMsgPtr, const MessagePtr &rspMsgPtr)> Response;