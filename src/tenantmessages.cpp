#include "tenantmessages.h"

#include "netbridge.h"

#include <uniterprotocol.h>

#include <stdexcept>
#include <utility>

namespace netbridge {
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

template <typename QueryT>
std::shared_ptr<QueryT> submitQuery(std::shared_ptr<QueryT> query) {
    return NetBridge::instance().query(query) ? query : nullptr;
}

} // namespace

std::shared_ptr<GetUserQueryMessage> GetUserQueryMessage::create() {
    return submitQuery(std::shared_ptr<GetUserQueryMessage>{
        new GetUserQueryMessage(makeTenantMessage(sharedmodel::ProtocolAction::GET_USER))});
}

GetUserQueryMessage::GetUserQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

std::shared_ptr<GetKafkaQueryMessage> GetKafkaQueryMessage::create(uint64_t kafkaOffset) {
    auto message = makeTenantMessage(sharedmodel::ProtocolAction::GET_KAFKA);
    message->add_data[sharedmodel::AddDataKafkaOffset] = std::to_string(kafkaOffset);
    return submitQuery(std::shared_ptr<GetKafkaQueryMessage>{new GetKafkaQueryMessage(std::move(message))});
}

GetKafkaQueryMessage::GetKafkaQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

std::shared_ptr<FullSyncQueryMessage> FullSyncQueryMessage::create() {
    return submitQuery(std::shared_ptr<FullSyncQueryMessage>{
        new FullSyncQueryMessage(makeTenantMessage(sharedmodel::ProtocolAction::FULL_SYNC))});
}

FullSyncQueryMessage::FullSyncQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

std::shared_ptr<BeginTransactionQueryMessage> BeginTransactionQueryMessage::create() {
    return submitQuery(std::shared_ptr<BeginTransactionQueryMessage>{
        new BeginTransactionQueryMessage(makeTenantMessage(sharedmodel::ProtocolAction::BEGIN_TRANSACTION))});
}

BeginTransactionQueryMessage::BeginTransactionQueryMessage(
    std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

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
    : QueryMessage(std::move(message)) {}

std::shared_ptr<TenantFileAccessQueryMessage> TenantFileAccessQueryMessage::create(
    const std::string& objectKey,
    TenantFileAccessMode mode) {
    if (objectKey.empty()) {
        throw std::invalid_argument("TENANT FILE_ACCESS requires an object key");
    }
    auto message = makeTenantMessage(sharedmodel::ProtocolAction::FILE_ACCESS);
    message->add_data[sharedmodel::AddDataObjectKey] = objectKey;
    message->add_data[sharedmodel::AddDataFileAccessMode] =
        mode == TenantFileAccessMode::READ
            ? sharedmodel::AddDataFileAccessRead
            : sharedmodel::AddDataFileAccessWrite;
    return submitQuery(std::shared_ptr<TenantFileAccessQueryMessage>{
        new TenantFileAccessQueryMessage(std::move(message))});
}

TenantFileAccessQueryMessage::TenantFileAccessQueryMessage(
    std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

} // namespace netbridge
