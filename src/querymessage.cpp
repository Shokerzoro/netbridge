#include "querymessage.h"

#include <utility>

namespace tenantbridge {

QueryMessage::QueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QObject(nullptr),
      message_(std::move(message)) {
}

void QueryMessage::setSequenceId(uint64_t id) {
    sequence_id_ = id;
    message_->sequence_id = id;
}

void QueryMessage::setResponse(std::shared_ptr<sharedmodel::UniterMessage> message) {
    response_ = std::move(message);
}

} // namespace tenantbridge
