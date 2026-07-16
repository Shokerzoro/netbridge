#include "tenantmessager.h"

#include "eventmessage.h"
#include "querymessage.h"

#include <utility>

namespace tenantbridge {

namespace {

bool isTenantAction(sharedmodel::ProtocolAction action) {
    using sharedmodel::ProtocolAction;
    switch (action) {
        case ProtocolAction::GET_USER:
        case ProtocolAction::GET_KAFKA:
        case ProtocolAction::FULL_SYNC:
        case ProtocolAction::BEGIN_TRANSACTION:
        case ProtocolAction::END_TRANSACTION:
        case ProtocolAction::FILE_ACCESS:
            return true;
        default:
            return false;
    }
}

bool isCrudEventAction(sharedmodel::CrudAction action) {
    return action == sharedmodel::CrudAction::CREATE
        || action == sharedmodel::CrudAction::UPDATE
        || action == sharedmodel::CrudAction::DELETE;
}

bool matchesResponse(
    const sharedmodel::UniterMessage& request,
    const sharedmodel::UniterMessage& response) {
    if (request.endpoint != response.endpoint) {
        return false;
    }

    if (request.endpoint == sharedmodel::Endpoint::TENANT) {
        return request.action == response.action;
    }

    return request.endpoint == sharedmodel::Endpoint::CRUD
        && request.action == sharedmodel::ProtocolAction::NONE
        && response.action == sharedmodel::ProtocolAction::NONE
        && request.crudact == response.crudact;
}

} // namespace

TenantMessager::TenantMessager()
    : QObject(nullptr) {
}

void TenantMessager::setToken(const std::string& token) {
    token_ = token;
    queries.clear();
}

void TenantMessager::refreshToken(const std::string& token) {
    if (!token.empty()) {
        token_ = token;
    }
}

bool TenantMessager::prepareMessage(
    const std::shared_ptr<sharedmodel::UniterMessage>& message,
    sharedmodel::MessageStatus expectedStatus) {
    if (!message || token_.empty() || message->status != expectedStatus) {
        return false;
    }

    if (message->endpoint == sharedmodel::Endpoint::TENANT) {
        if (expectedStatus != sharedmodel::MessageStatus::REQUEST
            || message->subsystem != sharedmodel::Subsystem::PROTOCOL
            || message->crudact != sharedmodel::CrudAction::NOTCRUD
            || !isTenantAction(message->action)) {
            return false;
        }
    } else if (message->endpoint == sharedmodel::Endpoint::CRUD) {
        if (message->action != sharedmodel::ProtocolAction::NONE
            || message->crudact == sharedmodel::CrudAction::NOTCRUD
            || !message->resource) {
            return false;
        }
        if (expectedStatus == sharedmodel::MessageStatus::NOTIFICATION
            && !isCrudEventAction(message->crudact)) {
            return false;
        }
    } else {
        return false;
    }

    message->add_data[sharedmodel::AddDataToken] = token_;
    return true;
}

bool TenantMessager::query(const std::shared_ptr<QueryMessage>& message) {
    if (!message || !prepareMessage(message->message(), sharedmodel::MessageStatus::REQUEST)) {
        return false;
    }

    const auto id = ++sequence_id;
    message->setSequenceId(id);
    queries[id] = message;
    emit signalSendMessage(message->message());
    return true;
}

bool TenantMessager::sendMessage(const std::shared_ptr<EventMessage>& message) {
    if (!message || !prepareMessage(message->message(), sharedmodel::MessageStatus::NOTIFICATION)) {
        return false;
    }

    emit signalSendMessage(message->message());
    message->notify_sent();
    return true;
}

void TenantMessager::onRecvUniterMessage(std::shared_ptr<sharedmodel::UniterMessage> message) {
    if (!message || !message->sequence_id
        || (message->status != sharedmodel::MessageStatus::SUCCESS
            && message->status != sharedmodel::MessageStatus::ERROR)
        || (message->endpoint != sharedmodel::Endpoint::TENANT
            && message->endpoint != sharedmodel::Endpoint::CRUD)) {
        return;
    }

    auto it = queries.find(*message->sequence_id);
    if (it == queries.end()) {
        return;
    }

    auto query = it->second.lock();
    if (!query || !query->message() || !matchesResponse(*query->message(), *message)) {
        if (!query) {
            queries.erase(it);
        }
        return;
    }

    queries.erase(it);
    query->setResponse(std::move(message));
    query->notify_received();
}

} // namespace tenantbridge
