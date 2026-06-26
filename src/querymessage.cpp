#include "querymessage.h"

#include "messagemanager.h"

#include <utility>

namespace serverbrige {

std::shared_ptr<QueryMessage> QueryMessage::create(std::shared_ptr<sharedmodel::UniterMessage> message) {
    std::shared_ptr<QueryMessage> query{new QueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    if (!query->message() || !query->message()->sequence_id) {
        return {};
    }
    return query;
}

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

} // namespace serverbrige
