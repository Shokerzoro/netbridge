#include "specializedmessages.h"

#include "messagemanager.h"

#include <contract/uniterprotocol.h>

#include <stdexcept>
#include <optional>
#include <utility>

namespace messaging {

namespace {

std::shared_ptr<contract::UniterMessage> makeCrudMessage(
    contract::CrudAction action,
    contract::MessageStatus status,
    std::shared_ptr<contract::ResourceAbstract> resource) {
    if (resource && resource->key.subsystem.subsystem == contract::Subsystem::LOCAL) {
        throw std::invalid_argument("Local resources cannot be sent through CRUD messaging");
    }

    auto message = std::make_shared<contract::UniterMessage>();
    message->subsystem = resource ? resource->key.subsystem.subsystem : contract::Subsystem::PROTOCOL;
    message->gensubsystemid = resource && resource->key.subsystem.has_genid()
        ? std::optional<uint64_t>{resource->key.subsystem.get_genid()}
        : std::nullopt;
    message->crudact = action;
    message->protact = contract::ProtocolAction::NOTPROTOCOL;
    message->status = status;
    message->resource = std::move(resource);
    return message;
}

std::shared_ptr<contract::UniterMessage> makeProtocolMessage(contract::ProtocolAction action) {
    auto message = std::make_shared<contract::UniterMessage>();
    message->subsystem = contract::Subsystem::PROTOCOL;
    message->protact = action;
    message->status = contract::MessageStatus::REQUEST;
    return message;
}

} // namespace

std::shared_ptr<CrudEventMessage> CrudEventMessage::create(
    contract::CrudAction action,
    std::shared_ptr<contract::ResourceAbstract> resource) {
    std::shared_ptr<CrudEventMessage> event{
        new CrudEventMessage(makeCrudMessage(action, contract::MessageStatus::NOTIFICATION, std::move(resource)))
    };
    MessageManager::instance().sendMessage(event);
    return event->message() ? event : nullptr;
}

CrudEventMessage::CrudEventMessage(std::shared_ptr<contract::UniterMessage> message)
    : EventMessage(std::move(message)) {
}

std::shared_ptr<CrudQueryMessage> CrudQueryMessage::create(
    contract::CrudAction action,
    std::shared_ptr<contract::ResourceAbstract> resource) {
    std::shared_ptr<CrudQueryMessage> query{
        new CrudQueryMessage(makeCrudMessage(action, contract::MessageStatus::REQUEST, std::move(resource)))
    };
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

CrudQueryMessage::CrudQueryMessage(std::shared_ptr<contract::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<TransactionQueryMessage> TransactionQueryMessage::createBegin(const std::string& transactionId) {
    auto message = makeProtocolMessage(contract::ProtocolAction::BEGIN_TRANSACTION);
    message->add_data[contract::AddDataTransactionId] = transactionId;

    std::shared_ptr<TransactionQueryMessage> query{new TransactionQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

std::shared_ptr<TransactionQueryMessage> TransactionQueryMessage::createFinish(
    const std::string& transactionId,
    TransactionAction action) {
    auto message = makeProtocolMessage(contract::ProtocolAction::COMMIT_ROLLBACK_TXN);
    message->add_data[contract::AddDataTransactionId] = transactionId;
    message->add_data[contract::AddDataTransactionAction] =
        action == TransactionAction::COMMIT
            ? contract::AddDataTransactionCommit
            : contract::AddDataTransactionRollback;

    std::shared_ptr<TransactionQueryMessage> query{new TransactionQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

TransactionQueryMessage::TransactionQueryMessage(std::shared_ptr<contract::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<PresignedUrlQueryMessage> PresignedUrlQueryMessage::create(const std::string& object) {
    auto message = makeProtocolMessage(contract::ProtocolAction::GET_MINIO_PRESIGNED_URL);
    message->add_data[contract::AddDataMinioObject] = object;

    std::shared_ptr<PresignedUrlQueryMessage> query{new PresignedUrlQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

PresignedUrlQueryMessage::PresignedUrlQueryMessage(std::shared_ptr<contract::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<DownloadFileQueryMessage> DownloadFileQueryMessage::create(
    const std::string& object,
    const std::string& presignedUrl) {
    auto message = makeProtocolMessage(contract::ProtocolAction::GET_MINIO_FILE);
    message->add_data[contract::AddDataMinioObject] = object;
    message->add_data[contract::AddDataPresignedUrl] = presignedUrl;

    std::shared_ptr<DownloadFileQueryMessage> query{new DownloadFileQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

DownloadFileQueryMessage::DownloadFileQueryMessage(std::shared_ptr<contract::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<UploadFileQueryMessage> UploadFileQueryMessage::create(
    const std::string& object,
    const std::string& presignedUrl,
    const std::filesystem::path& localPath) {
    auto message = makeProtocolMessage(contract::ProtocolAction::PUT_MINIO_FILE);
    message->add_data[contract::AddDataMinioObject] = object;
    message->add_data[contract::AddDataPresignedUrl] = presignedUrl;
    message->add_data[contract::AddDataLocalPath] = localPath.string();

    std::shared_ptr<UploadFileQueryMessage> query{new UploadFileQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

UploadFileQueryMessage::UploadFileQueryMessage(std::shared_ptr<contract::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

} // namespace messaging
