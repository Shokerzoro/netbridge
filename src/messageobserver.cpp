#include "messageobserver.h"

#include <utility>

namespace serverbrige {

MessageObserver::MessageObserver(uint64_t expectedSequenceId)
    : expectedSequenceId_(expectedSequenceId) {
}

uint64_t MessageObserver::expectedSequenceId() const {
    return expectedSequenceId_;
}

MessageObserver::State MessageObserver::state() const {
    return state_;
}

const std::shared_ptr<sharedmodel::UniterMessage>& MessageObserver::response() const {
    return response_;
}

void MessageObserver::recordResponse(std::shared_ptr<sharedmodel::UniterMessage> message) {
    response_ = std::move(message);
    state_ = State::Success;
}

void MessageObserver::recordError(std::shared_ptr<sharedmodel::UniterMessage> message) {
    response_ = std::move(message);
    state_ = State::Error;
}

void MessageObserver::recordTimeout() {
    state_ = State::Timeout;
}

} // namespace serverbrige
