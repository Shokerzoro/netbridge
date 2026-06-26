#include "messagemanager.h"

#include "eventmessage.h"
#include "querymessage.h"

namespace serverbrige {

MessageManager::MessageManager()
    : QObject(nullptr) {
}

void MessageManager::query(std::shared_ptr<QueryMessage> message) {
    if (!message || !message->message()) {
        return;
    }

    const auto id = ++sequence_id;
    message->setSequenceId(id);
    queries[id] = message;
    emit signalSendMessage(message->message());
}

void MessageManager::sendMessage(std::shared_ptr<EventMessage> message) {
    if (!message || !message->message()) {
        return;
    }

    emit signalSendMessage(message->message());
    message->notify_sent();
}

void MessageManager::onRecvUniterMessage(std::shared_ptr<sharedmodel::UniterMessage> message) {
    if (!message || !message->sequence_id) {
        return;
    }

    auto it = queries.find(*message->sequence_id);
    if (it == queries.end()) {
        return;
    }

    auto query = it->second.lock();
    queries.erase(it);
    if (!query) {
        return;
    }

    query->setResponse(std::move(message));
    query->notify_received();
}

} // namespace serverbrige
