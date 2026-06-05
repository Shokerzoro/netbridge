#include "messagemanager.h"

#include <utility>

namespace messaging {

std::shared_ptr<MessageObserver> MessageManager::track(std::shared_ptr<contract::UniterMessage> message) {
    if (!message) {
        return nullptr;
    }

    const auto sequenceId = nextSequenceId_++;
    message->sequence_id = sequenceId;

    auto observer = std::make_shared<MessageObserver>(sequenceId);
    observers_.emplace(sequenceId, observer);
    return observer;
}

bool MessageManager::routeIncomingDirectResponse(std::shared_ptr<contract::UniterMessage> message) {
    if (!message || !message->sequence_id) {
        return false;
    }

    auto it = observers_.find(*message->sequence_id);
    if (it == observers_.end()) {
        return false;
    }

    auto observer = it->second;
    observers_.erase(it);

    if (message->status == contract::MessageStatus::ERROR ||
        message->error != contract::ErrorCode::SUCCESS) {
        observer->recordError(std::move(message));
    } else {
        observer->recordResponse(std::move(message));
    }

    return true;
}

} // namespace messaging
