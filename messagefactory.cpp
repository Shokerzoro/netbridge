#include "messagefactory.h"

#include <contract/uniterprotocol.h>

#include <cstdint>
#include <optional>
#include <utility>

namespace messaging {

std::shared_ptr<contract::UniterMessage> MessageFactory::makeCrudMessage(
    contract::CrudAction action,
    std::shared_ptr<contract::ResourceAbstract> resource) {
    auto message = std::make_shared<contract::UniterMessage>();
    message->subsystem = resource ? resource->key.subsystem.subsystem : contract::Subsystem::PROTOCOL;
    message->gensubsystemid = resource && resource->key.subsystem.has_genid()
        ? std::optional<uint64_t>{resource->key.subsystem.get_genid()}
        : std::nullopt;
    message->crudact = action;
    message->protact = contract::ProtocolAction::NOTPROTOCOL;
    message->status = contract::MessageStatus::NOTIFICATION;
    message->resource = std::move(resource);
    return message;
}

std::shared_ptr<contract::UniterMessage> MessageFactory::makeCreateMessage(
    std::shared_ptr<contract::ResourceAbstract> resource) {
    return makeCrudMessage(contract::CrudAction::CREATE, std::move(resource));
}

std::shared_ptr<contract::UniterMessage> MessageFactory::makeUpdateMessage(
    std::shared_ptr<contract::ResourceAbstract> resource) {
    return makeCrudMessage(contract::CrudAction::UPDATE, std::move(resource));
}

std::shared_ptr<contract::UniterMessage> MessageFactory::makeDeleteMessage(
    std::shared_ptr<contract::ResourceAbstract> resource) {
    return makeCrudMessage(contract::CrudAction::DELETE, std::move(resource));
}

std::shared_ptr<contract::UniterMessage> MessageFactory::makeBeginTransactionMessage(
    const std::string& transactionId) {
    auto message = std::make_shared<contract::UniterMessage>();
    message->subsystem = contract::Subsystem::PROTOCOL;
    message->protact = contract::ProtocolAction::BEGIN_TRANSACTION;
    message->status = contract::MessageStatus::REQUEST;
    message->add_data[contract::AddDataTransactionId] = transactionId;
    return message;
}

std::shared_ptr<contract::UniterMessage> MessageFactory::makeFinishTransactionMessage(
    const std::string& transactionId,
    TransactionAction action) {
    auto message = std::make_shared<contract::UniterMessage>();
    message->subsystem = contract::Subsystem::PROTOCOL;
    message->protact = contract::ProtocolAction::COMMIT_ROLLBACK_TXN;
    message->status = contract::MessageStatus::REQUEST;
    message->add_data[contract::AddDataTransactionId] = transactionId;
    message->add_data[contract::AddDataTransactionAction] =
        action == TransactionAction::COMMIT
            ? contract::AddDataTransactionCommit
            : contract::AddDataTransactionRollback;
    return message;
}

std::shared_ptr<contract::UniterMessage> MessageFactory::makePresignedUrlRequest(
    const std::string& object) {
    auto message = std::make_shared<contract::UniterMessage>();
    message->subsystem = contract::Subsystem::PROTOCOL;
    message->protact = contract::ProtocolAction::GET_MINIO_PRESIGNED_URL;
    message->status = contract::MessageStatus::REQUEST;
    message->add_data[contract::AddDataMinioObject] = object;
    return message;
}

std::shared_ptr<contract::UniterMessage> MessageFactory::makeFileRequest(
    const std::string& object,
    const std::string& presignedUrl) {
    auto message = std::make_shared<contract::UniterMessage>();
    message->subsystem = contract::Subsystem::PROTOCOL;
    message->protact = contract::ProtocolAction::GET_MINIO_FILE;
    message->status = contract::MessageStatus::REQUEST;
    message->add_data[contract::AddDataMinioObject] = object;
    message->add_data[contract::AddDataPresignedUrl] = presignedUrl;
    return message;
}

std::shared_ptr<contract::UniterMessage> MessageFactory::makeUploadFileRequest(
    const std::string& object,
    const std::string& presignedUrl,
    const std::filesystem::path& localPath) {
    auto message = std::make_shared<contract::UniterMessage>();
    message->subsystem = contract::Subsystem::PROTOCOL;
    message->protact = contract::ProtocolAction::PUT_MINIO_FILE;
    message->status = contract::MessageStatus::REQUEST;
    message->add_data[contract::AddDataMinioObject] = object;
    message->add_data[contract::AddDataPresignedUrl] = presignedUrl;
    message->add_data[contract::AddDataLocalPath] = localPath.string();
    return message;
}

} // namespace messaging
