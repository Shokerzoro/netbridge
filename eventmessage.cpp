#include "eventmessage.h"

#include "messagemanager.h"

#include <utility>

namespace messaging {

std::shared_ptr<EventMessage> EventMessage::create(std::shared_ptr<contract::UniterMessage> message) {
    std::shared_ptr<EventMessage> event{new EventMessage(std::move(message))};
    MessageManager::instance().sendMessage(event);
    if (!event->message()) {
        return {};
    }
    return event;
}

EventMessage::EventMessage(std::shared_ptr<contract::UniterMessage> message)
    : QObject(nullptr),
      message_(std::move(message)) {
}

} // namespace messaging
