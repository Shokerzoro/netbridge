#include "netbridge.h"

#include "eventmessage.h"
#include "querymessage.h"

#include <logging/logger.h>

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
    using sharedmodel::PublicAction;
    switch (message.publicact) {
        case PublicAction::GET_TOKEN:
            return hasValue(message, sharedmodel::AddDataLogin)
                && hasValue(message, sharedmodel::AddDataPassword);
        case PublicAction::REFRESH_TOKEN:
            return hasValue(message, sharedmodel::AddDataRefreshToken);
        case PublicAction::CREATE_EMPLOYEE:
            return hasValue(message, sharedmodel::AddDataEmail)
                && hasValue(message, sharedmodel::AddDataName);
        case PublicAction::REGISTER_COMPANY:
            return hasValue(message, sharedmodel::AddDataCompanyName)
                && hasValue(message, sharedmodel::AddDataAdminLogin)
                && hasValue(message, sharedmodel::AddDataAdminEmail)
                && hasValue(message, sharedmodel::AddDataAdminPassword);
        case PublicAction::GET_UPDATE:
            return hasValue(message, sharedmodel::AddDataAppVersion);
        case PublicAction::GET_MIGRATIONS:
            return hasValue(message, sharedmodel::AddDataDataModelVersion)
                && hasValue(message, sharedmodel::AddDataMigrationTarget)
                && (message.add_data.at(sharedmodel::AddDataMigrationTarget)
                        == sharedmodel::AddDataMigrationTargetLocal
                    || message.add_data.at(sharedmodel::AddDataMigrationTarget)
                        == sharedmodel::AddDataMigrationTargetShared);
        case PublicAction::FILE_ACCESS:
            return hasValue(message, sharedmodel::AddDataObjectKey) && hasAccessMode(message);
        default:
            return false;
    }
}

bool isValidTenantRequest(const sharedmodel::UniterMessage& message) {
    using sharedmodel::TenantAction;
    switch (message.tenantact) {
        case TenantAction::GET_USER:
        case TenantAction::FULL_SYNC:
        case TenantAction::BEGIN_TRANSACTION:
            return true;
        case TenantAction::GET_KAFKA:
            return hasValue(message, sharedmodel::AddDataKafkaOffset);
        case TenantAction::END_TRANSACTION:
            return hasValue(message, sharedmodel::AddDataTransactionId)
                && hasTransactionAction(message);
        case TenantAction::FILE_ACCESS:
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
    using namespace sharedmodel;
    if (request.endpoint != response.endpoint) {
        return false;
    }
    if (response.endpoint == Endpoint::PUBLIC) {
        return response.crudact == CrudAction::NOTCRUD
            && request.publicact == response.publicact
            && response.tenantact == TenantAction::NOTTENANT
            && response.filestorageact == FileStorageAction::NOTFILESTORAGE;
    }
    if (response.endpoint == Endpoint::TENANT) {
        return response.crudact == CrudAction::NOTCRUD
            && request.tenantact == response.tenantact
            && response.publicact == PublicAction::NOTPUBLIC
            && response.filestorageact == FileStorageAction::NOTFILESTORAGE;
    }
    if (response.endpoint == Endpoint::CRUD) {
        return request.crudact == response.crudact
            && response.publicact == PublicAction::NOTPUBLIC
            && response.tenantact == TenantAction::NOTTENANT
            && response.filestorageact == FileStorageAction::NOTFILESTORAGE;
    }
    return false;
}

} // namespace

NetBridge::NetBridge()
    : QObject(nullptr) {
}

void NetBridge::setToken(const std::string& token) {
    const auto pendingCount = queries_.size();
    token_ = token;
    queries_.clear();
    syspilot::Logger::instance().full_log(
        "NetBridge::setToken(): session replaced, authenticated="
        + std::string(token_.empty() ? "false" : "true")
        + ", cleared_queries=" + std::to_string(pendingCount));
}

void NetBridge::refreshToken(const std::string& token) {
    if (!token.empty()) {
        token_ = token;
        syspilot::Logger::instance().full_log(
            "NetBridge::refreshToken(): session token refreshed");
    } else {
        syspilot::Logger::instance().full_log(
            "NetBridge::refreshToken(): rejected empty token");
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
            || message->tenantact != sharedmodel::TenantAction::NOTTENANT
            || message->filestorageact != sharedmodel::FileStorageAction::NOTFILESTORAGE
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
            || message->publicact != sharedmodel::PublicAction::NOTPUBLIC
            || message->filestorageact != sharedmodel::FileStorageAction::NOTFILESTORAGE
            || !isValidTenantRequest(*message)) {
            return false;
        }
    } else if (message->endpoint == sharedmodel::Endpoint::CRUD) {
        if (message->crudact == sharedmodel::CrudAction::NOTCRUD
            || message->publicact != sharedmodel::PublicAction::NOTPUBLIC
            || message->tenantact != sharedmodel::TenantAction::NOTTENANT
            || message->filestorageact != sharedmodel::FileStorageAction::NOTFILESTORAGE
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
        syspilot::Logger::instance().full_log(
            "NetBridge::query(): rejected invalid request");
        return false;
    }

    const auto id = ++sequence_id_;
    message->setSequenceId(id);
    queries_[id] = message;

    syspilot::Logger::instance().full_log(
        "NetBridge::query(): dispatching endpoint="
        + std::to_string(static_cast<int>(message->message()->endpoint))
        + ", public_action=" + std::to_string(static_cast<int>(message->message()->publicact))
        + ", tenant_action=" + std::to_string(static_cast<int>(message->message()->tenantact))
        + ", crud=" + std::to_string(static_cast<int>(message->message()->crudact))
        + ", sequence=" + std::to_string(id));

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
            syspilot::Logger::instance().full_log(
                "NetBridge::query(): rejected unsupported endpoint, sequence="
                + std::to_string(id));
            return false;
    }
    return true;
}

bool NetBridge::sendMessage(const std::shared_ptr<EventMessage>& message) {
    if (!message || !prepareMessage(message->message(), sharedmodel::MessageStatus::NOTIFICATION)
        || message->message()->endpoint != sharedmodel::Endpoint::CRUD) {
        syspilot::Logger::instance().full_log(
            "NetBridge::sendMessage(): rejected invalid event");
        return false;
    }

    syspilot::Logger::instance().full_log(
        "NetBridge::sendMessage(): dispatching CRUD event, subsystem="
        + std::to_string(static_cast<int>(message->message()->subsystem))
        + ", crud=" + std::to_string(static_cast<int>(message->message()->crudact)));
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
        syspilot::Logger::instance().full_log(
            "NetBridge::receiveMessage(): rejected invalid response envelope, expected_endpoint="
            + std::to_string(static_cast<int>(expectedEndpoint)));
        return;
    }

    auto it = queries_.find(*message->sequence_id);
    if (it == queries_.end()) {
        syspilot::Logger::instance().full_log(
            "NetBridge::receiveMessage(): ignored unknown sequence="
            + std::to_string(*message->sequence_id));
        return;
    }

    auto query = it->second.lock();
    if (!query || !query->message() || !matchesResponse(*query->message(), *message)) {
        if (!query) {
            queries_.erase(it);
        }
        syspilot::Logger::instance().full_log(
            "NetBridge::receiveMessage(): rejected mismatched response, sequence="
            + std::to_string(*message->sequence_id));
        return;
    }

    syspilot::Logger::instance().full_log(
        "NetBridge::receiveMessage(): completed sequence="
        + std::to_string(*message->sequence_id)
        + ", status=" + std::to_string(static_cast<int>(message->status)));
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
