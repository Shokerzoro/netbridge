#include "specializedmessages.h"

#include "tenantmessager.h"

#include <uniterprotocol.h>

#include <stdexcept>
#include <utility>

namespace tenantbridge {

namespace {

std::shared_ptr<sharedmodel::UniterMessage> makeTenantMessage(sharedmodel::ProtocolAction action) {
    auto message = std::make_shared<sharedmodel::UniterMessage>();
    message->endpoint = sharedmodel::Endpoint::TENANT;
    message->subsystem = sharedmodel::Subsystem::PROTOCOL;
    message->crudact = sharedmodel::CrudAction::NOTCRUD;
    message->action = action;
    message->status = sharedmodel::MessageStatus::REQUEST;
    return message;
}

std::shared_ptr<sharedmodel::UniterMessage> makeCrudMessage(
    sharedmodel::CrudAction action,
    sharedmodel::MessageStatus status,
    std::shared_ptr<sharedmodel::ResourceAbstract> resource,
    const std::optional<std::string>& transactionId) {
    if (action == sharedmodel::CrudAction::NOTCRUD) {
        throw std::invalid_argument("CRUD action must be specified");
    }
    if (!resource) {
        throw std::invalid_argument("CRUD message requires a resource");
    }
    if (resource->key.subsystem.subsystem == sharedmodel::Subsystem::LOCAL) {
        throw std::invalid_argument("Local resources cannot be sent through tenantbridge");
    }

    auto message = std::make_shared<sharedmodel::UniterMessage>();
    message->endpoint = sharedmodel::Endpoint::CRUD;
    message->subsystem = resource->key.subsystem.subsystem;
    message->gensubsystemid = resource->key.subsystem.has_genid()
        ? std::optional<uint64_t>{resource->key.subsystem.get_genid()}
        : std::nullopt;
    message->crudact = action;
    message->action = sharedmodel::ProtocolAction::NONE;
    message->status = status;
    message->resource = std::move(resource);
    if (transactionId && !transactionId->empty()) {
        message->add_data[sharedmodel::AddDataTransactionId] = *transactionId;
    }
    return message;
}

template <typename QueryT>
std::shared_ptr<QueryT> submitQuery(std::shared_ptr<QueryT> query) {
    return TenantMessager::instance().query(query) ? query : nullptr;
}

} // namespace

std::shared_ptr<GetUserQueryMessage> GetUserQueryMessage::create() {
    return submitQuery(std::shared_ptr<GetUserQueryMessage>{
        new GetUserQueryMessage(makeTenantMessage(sharedmodel::ProtocolAction::GET_USER))});
}

GetUserQueryMessage::GetUserQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<GetKafkaQueryMessage> GetKafkaQueryMessage::create(uint64_t kafkaOffset) {
    auto message = makeTenantMessage(sharedmodel::ProtocolAction::GET_KAFKA);
    message->add_data[sharedmodel::AddDataKafkaOffset] = std::to_string(kafkaOffset);
    return submitQuery(std::shared_ptr<GetKafkaQueryMessage>{new GetKafkaQueryMessage(std::move(message))});
}

GetKafkaQueryMessage::GetKafkaQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<FullSyncQueryMessage> FullSyncQueryMessage::create() {
    return submitQuery(std::shared_ptr<FullSyncQueryMessage>{
        new FullSyncQueryMessage(makeTenantMessage(sharedmodel::ProtocolAction::FULL_SYNC))});
}

FullSyncQueryMessage::FullSyncQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<BeginTransactionQueryMessage> BeginTransactionQueryMessage::create() {
    return submitQuery(std::shared_ptr<BeginTransactionQueryMessage>{
        new BeginTransactionQueryMessage(makeTenantMessage(sharedmodel::ProtocolAction::BEGIN_TRANSACTION))});
}

BeginTransactionQueryMessage::BeginTransactionQueryMessage(
    std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<EndTransactionQueryMessage> EndTransactionQueryMessage::create(
    const std::string& transactionId,
    TransactionAction action) {
    if (transactionId.empty()) {
        throw std::invalid_argument("END_TRANSACTION requires a transaction ID");
    }
    auto message = makeTenantMessage(sharedmodel::ProtocolAction::END_TRANSACTION);
    message->add_data[sharedmodel::AddDataTransactionId] = transactionId;
    message->add_data[sharedmodel::AddDataTransactionAction] =
        action == TransactionAction::COMMIT
            ? sharedmodel::AddDataTransactionCommit
            : sharedmodel::AddDataTransactionRollback;
    return submitQuery(std::shared_ptr<EndTransactionQueryMessage>{
        new EndTransactionQueryMessage(std::move(message))});
}

EndTransactionQueryMessage::EndTransactionQueryMessage(
    std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<FileAccessQueryMessage> FileAccessQueryMessage::create(
    const std::string& objectKey,
    FileAccessMode mode) {
    if (objectKey.empty()) {
        throw std::invalid_argument("FILE_ACCESS requires an object key");
    }
    auto message = makeTenantMessage(sharedmodel::ProtocolAction::FILE_ACCESS);
    message->add_data[sharedmodel::AddDataObjectKey] = objectKey;
    message->add_data[sharedmodel::AddDataFileAccessMode] =
        mode == FileAccessMode::READ
            ? sharedmodel::AddDataFileAccessRead
            : sharedmodel::AddDataFileAccessWrite;
    return submitQuery(std::shared_ptr<FileAccessQueryMessage>{
        new FileAccessQueryMessage(std::move(message))});
}

FileAccessQueryMessage::FileAccessQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<CrudQueryMessage> CrudQueryMessage::create(
    sharedmodel::CrudAction action,
    std::shared_ptr<sharedmodel::ResourceAbstract> resource,
    std::optional<std::string> transactionId) {
    auto message = makeCrudMessage(
        action,
        sharedmodel::MessageStatus::REQUEST,
        std::move(resource),
        transactionId);
    return submitQuery(std::shared_ptr<CrudQueryMessage>{new CrudQueryMessage(std::move(message))});
}

CrudQueryMessage::CrudQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<CrudEventMessage> CrudEventMessage::create(
    sharedmodel::CrudAction action,
    std::shared_ptr<sharedmodel::ResourceAbstract> resource,
    std::optional<std::string> transactionId) {
    if (action == sharedmodel::CrudAction::READ) {
        throw std::invalid_argument("READ requires a tracked CRUD query");
    }
    auto message = makeCrudMessage(
        action,
        sharedmodel::MessageStatus::NOTIFICATION,
        std::move(resource),
        transactionId);
    std::shared_ptr<CrudEventMessage> event{new CrudEventMessage(std::move(message))};
    return TenantMessager::instance().sendMessage(event) ? event : nullptr;
}

CrudEventMessage::CrudEventMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : EventMessage(std::move(message)) {
}

} // namespace tenantbridge
