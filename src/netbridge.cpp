#include "netbridge.h"

#include "eventmessage.h"
#include "querymessage.h"

#include <utility>

namespace netbridge {

namespace {

bool hasValue(const sharedmodel::UniterMessage& message, const char* key) {
    const auto it = message.add_data.find(key);
    return it != message.add_data.end() && !it->second.empty();
}

bool hasAccessMode(const sharedmodel::UniterMessage& message) {
    const auto it = message.add_data.find(sharedmodel::AddDataFileAccessMode);
    return it != message.add_data.end()
        && (it->second == sharedmodel::AddDataFileAccessRead
            || it->second == sharedmodel::AddDataFileAccessWrite);
}

bool hasTransactionAction(const sharedmodel::UniterMessage& message) {
    const auto it = message.add_data.find(sharedmodel::AddDataTransactionAction);
    return it != message.add_data.end()
        && (it->second == sharedmodel::AddDataTransactionCommit
            || it->second == sharedmodel::AddDataTransactionRollback);
}

bool isValidPublicRequest(const sharedmodel::UniterMessage& message) {
    using sharedmodel::ProtocolAction;
    switch (message.action) {
        case ProtocolAction::GET_TOKEN:
            return hasValue(message, sharedmodel::AddDataLogin)
                && hasValue(message, sharedmodel::AddDataPassword);
        case ProtocolAction::REFRESH_TOKEN:
            return hasValue(message, sharedmodel::AddDataRefreshToken);
        case ProtocolAction::CREATE_EMPLOYEE:
            return hasValue(message, sharedmodel::AddDataEmail)
                && hasValue(message, sharedmodel::AddDataName);
        case ProtocolAction::REGISTER_COMPANY:
            return hasValue(message, sharedmodel::AddDataCompanyName)
                && hasValue(message, sharedmodel::AddDataAdminLogin)
                && hasValue(message, sharedmodel::AddDataAdminEmail)
                && hasValue(message, sharedmodel::AddDataAdminPassword);
        case ProtocolAction::GET_UPDATE:
            return hasValue(message, sharedmodel::AddDataAppVersion);
        case ProtocolAction::GET_MIGRATIONS:
            return hasValue(message, sharedmodel::AddDataDataModelVersion)
                && hasValue(message, sharedmodel::AddDataMigrationTarget)
                && (message.add_data.at(sharedmodel::AddDataMigrationTarget)
                        == sharedmodel::AddDataMigrationTargetLocal
                    || message.add_data.at(sharedmodel::AddDataMigrationTarget)
                        == sharedmodel::AddDataMigrationTargetShared);
        case ProtocolAction::FILE_ACCESS:
            return hasValue(message, sharedmodel::AddDataObjectKey) && hasAccessMode(message);
        default:
            return false;
    }
}

bool isValidTenantRequest(const sharedmodel::UniterMessage& message) {
    using sharedmodel::ProtocolAction;
    switch (message.action) {
        case ProtocolAction::GET_USER:
        case ProtocolAction::FULL_SYNC:
        case ProtocolAction::BEGIN_TRANSACTION:
            return true;
        case ProtocolAction::GET_KAFKA:
            return hasValue(message, sharedmodel::AddDataKafkaOffset);
        case ProtocolAction::END_TRANSACTION:
            return hasValue(message, sharedmodel::AddDataTransactionId)
                && hasTransactionAction(message);
        case ProtocolAction::FILE_ACCESS:
            return hasValue(message, sharedmodel::AddDataObjectKey) && hasAccessMode(message);
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
    if (request.endpoint == sharedmodel::Endpoint::CRUD) {
        return request.action == sharedmodel::ProtocolAction::NONE
            && response.action == sharedmodel::ProtocolAction::NONE
            && request.crudact == response.crudact;
    }
    return request.action == response.action;
}

} // namespace

NetBridge::NetBridge()
    : QObject(nullptr) {
}

void NetBridge::setToken(const std::string& token) {
    token_ = token;
    queries_.clear();
}

void NetBridge::refreshToken(const std::string& token) {
    if (!token.empty()) {
        token_ = token;
    }
}

bool NetBridge::prepareMessage(
    const std::shared_ptr<sharedmodel::UniterMessage>& message,
    sharedmodel::MessageStatus expectedStatus) {
    if (!message || message->status != expectedStatus) {
        return false;
    }

    if (message->endpoint == sharedmodel::Endpoint::PUBLIC) {
        if (expectedStatus != sharedmodel::MessageStatus::REQUEST
            || message->subsystem != sharedmodel::Subsystem::PROTOCOL
            || message->crudact != sharedmodel::CrudAction::NOTCRUD
            || !isValidPublicRequest(*message)) {
            return false;
        }
        message->add_data.erase(sharedmodel::AddDataToken);
        return true;
    }

    if (token_.empty()) {
        return false;
    }

    if (message->endpoint == sharedmodel::Endpoint::TENANT) {
        if (expectedStatus != sharedmodel::MessageStatus::REQUEST
            || message->subsystem != sharedmodel::Subsystem::PROTOCOL
            || message->crudact != sharedmodel::CrudAction::NOTCRUD
            || !isValidTenantRequest(*message)) {
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

bool NetBridge::query(const std::shared_ptr<QueryMessage>& message) {
    if (!message || !prepareMessage(message->message(), sharedmodel::MessageStatus::REQUEST)) {
        return false;
    }

    const auto id = ++sequence_id_;
    message->setSequenceId(id);
    queries_[id] = message;

    switch (message->message()->endpoint) {
        case sharedmodel::Endpoint::PUBLIC:
            emit signalSendPublicMessage(message->message());
            break;
        case sharedmodel::Endpoint::TENANT:
            emit signalSendTenantMessage(message->message());
            break;
        case sharedmodel::Endpoint::CRUD:
            emit signalSendCrudMessage(message->message());
            break;
        default:
            queries_.erase(id);
            return false;
    }
    return true;
}

bool NetBridge::sendMessage(const std::shared_ptr<EventMessage>& message) {
    if (!message || !prepareMessage(message->message(), sharedmodel::MessageStatus::NOTIFICATION)
        || message->message()->endpoint != sharedmodel::Endpoint::CRUD) {
        return false;
    }

    emit signalSendCrudMessage(message->message());
    message->notify_sent();
    return true;
}

void NetBridge::receiveMessage(
    std::shared_ptr<sharedmodel::UniterMessage> message,
    sharedmodel::Endpoint expectedEndpoint) {
    if (!message || message->endpoint != expectedEndpoint || !message->sequence_id
        || (message->status != sharedmodel::MessageStatus::SUCCESS
            && message->status != sharedmodel::MessageStatus::ERROR)) {
        return;
    }

    auto it = queries_.find(*message->sequence_id);
    if (it == queries_.end()) {
        return;
    }

    auto query = it->second.lock();
    if (!query || !query->message() || !matchesResponse(*query->message(), *message)) {
        if (!query) {
            queries_.erase(it);
        }
        return;
    }

    queries_.erase(it);
    query->setResponse(std::move(message));
    query->notify_received();
}

void NetBridge::onReceivePublicMessage(std::shared_ptr<sharedmodel::UniterMessage> message) {
    receiveMessage(std::move(message), sharedmodel::Endpoint::PUBLIC);
}

void NetBridge::onReceiveTenantMessage(std::shared_ptr<sharedmodel::UniterMessage> message) {
    receiveMessage(std::move(message), sharedmodel::Endpoint::TENANT);
}

void NetBridge::onReceiveCrudMessage(std::shared_ptr<sharedmodel::UniterMessage> message) {
    receiveMessage(std::move(message), sharedmodel::Endpoint::CRUD);
}

} // namespace netbridge
