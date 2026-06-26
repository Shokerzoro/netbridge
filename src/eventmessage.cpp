#include "eventmessage.h"

#include "messagemanager.h"

#include <utility>

namespace serverbrige {

std::shared_ptr<EventMessage> EventMessage::create(std::shared_ptr<sharedmodel::UniterMessage> message) {
    std::shared_ptr<EventMessage> event{new EventMessage(std::move(message))};
    MessageManager::instance().sendMessage(event);
    if (!event->message()) {
        return {};
    }
    return event;
}

EventMessage::EventMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QObject(nullptr),
      message_(std::move(message)) {
}

} // namespace serverbrige
