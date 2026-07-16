#include "eventmessage.h"

#include <utility>

namespace netbridge {

EventMessage::EventMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QObject(nullptr),
      message_(std::move(message)) {
}

} // namespace netbridge
