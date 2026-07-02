#include "specializedmessages.h"

#include "messagemanager.h"

#include <uniterprotocol.h>

#include <stdexcept>
#include <optional>
#include <utility>

namespace serverbrige {

namespace {

std::shared_ptr<sharedmodel::UniterMessage> makeCrudMessage(
    sharedmodel::CrudAction action,
    sharedmodel::MessageStatus status,
    std::shared_ptr<sharedmodel::ResourceAbstract> resource) {
    if (resource && resource->key.subsystem.subsystem == sharedmodel::Subsystem::LOCAL) {
        throw std::invalid_argument("Local resources cannot be sent through CRUD serverbrige");
    }

    auto message = std::make_shared<sharedmodel::UniterMessage>();
    message->subsystem = resource ? resource->key.subsystem.subsystem : sharedmodel::Subsystem::PROTOCOL;
    message->gensubsystemid = resource && resource->key.subsystem.has_genid()
        ? std::optional<uint64_t>{resource->key.subsystem.get_genid()}
        : std::nullopt;
    message->crudact = action;
    message->protact = sharedmodel::ProtocolAction::NOTPROTOCOL;
    message->status = status;
    message->resource = std::move(resource);
    return message;
}

std::shared_ptr<sharedmodel::UniterMessage> makeProtocolMessage(sharedmodel::ProtocolAction action) {
    auto message = std::make_shared<sharedmodel::UniterMessage>();
    message->subsystem = sharedmodel::Subsystem::PROTOCOL;
    message->protact = action;
    message->status = sharedmodel::MessageStatus::REQUEST;
    return message;
}

} // namespace

std::shared_ptr<CrudEventMessage> CrudEventMessage::create(
    sharedmodel::CrudAction action,
    std::shared_ptr<sharedmodel::ResourceAbstract> resource) {
    std::shared_ptr<CrudEventMessage> event{
        new CrudEventMessage(makeCrudMessage(action, sharedmodel::MessageStatus::NOTIFICATION, std::move(resource)))
    };
    MessageManager::instance().sendMessage(event);
    return event->message() ? event : nullptr;
}

CrudEventMessage::CrudEventMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : EventMessage(std::move(message)) {
}

std::shared_ptr<CrudQueryMessage> CrudQueryMessage::create(
    sharedmodel::CrudAction action,
    std::shared_ptr<sharedmodel::ResourceAbstract> resource) {
    std::shared_ptr<CrudQueryMessage> query{
        new CrudQueryMessage(makeCrudMessage(action, sharedmodel::MessageStatus::REQUEST, std::move(resource)))
    };
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

CrudQueryMessage::CrudQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<TransactionQueryMessage> TransactionQueryMessage::createBegin(const std::string& transactionId) {
    auto message = makeProtocolMessage(sharedmodel::ProtocolAction::BEGIN_TRANSACTION);
    message->add_data[sharedmodel::AddDataTransactionId] = transactionId;

    std::shared_ptr<TransactionQueryMessage> query{new TransactionQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

std::shared_ptr<TransactionQueryMessage> TransactionQueryMessage::createFinish(
    const std::string& transactionId,
    TransactionAction action) {
    auto message = makeProtocolMessage(sharedmodel::ProtocolAction::COMMIT_ROLLBACK_TXN);
    message->add_data[sharedmodel::AddDataTransactionId] = transactionId;
    message->add_data[sharedmodel::AddDataTransactionAction] =
        action == TransactionAction::COMMIT
            ? sharedmodel::AddDataTransactionCommit
            : sharedmodel::AddDataTransactionRollback;

    std::shared_ptr<TransactionQueryMessage> query{new TransactionQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

TransactionQueryMessage::TransactionQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<PresignedUrlQueryMessage> PresignedUrlQueryMessage::create(const std::string& object) {
    auto message = makeProtocolMessage(sharedmodel::ProtocolAction::GET_MINIO_PRESIGNED_URL);
    message->add_data[sharedmodel::AddDataMinioObject] = object;

    std::shared_ptr<PresignedUrlQueryMessage> query{new PresignedUrlQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

PresignedUrlQueryMessage::PresignedUrlQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<DownloadFileQueryMessage> DownloadFileQueryMessage::create(
    const std::string& object,
    const std::string& presignedUrl) {
    auto message = makeProtocolMessage(sharedmodel::ProtocolAction::GET_MINIO_FILE);
    message->add_data[sharedmodel::AddDataMinioObject] = object;
    message->add_data[sharedmodel::AddDataPresignedUrl] = presignedUrl;

    std::shared_ptr<DownloadFileQueryMessage> query{new DownloadFileQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

DownloadFileQueryMessage::DownloadFileQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<UploadFileQueryMessage> UploadFileQueryMessage::create(
    const std::string& object,
    const std::string& presignedUrl,
    const std::filesystem::path& localPath) {
    auto message = makeProtocolMessage(sharedmodel::ProtocolAction::PUT_MINIO_FILE);
    message->add_data[sharedmodel::AddDataMinioObject] = object;
    message->add_data[sharedmodel::AddDataPresignedUrl] = presignedUrl;
    message->add_data[sharedmodel::AddDataLocalPath] = localPath.string();

    std::shared_ptr<UploadFileQueryMessage> query{new UploadFileQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

UploadFileQueryMessage::UploadFileQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

} // namespace serverbrige
