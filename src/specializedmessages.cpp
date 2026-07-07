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
    message->protact = sharedmodel::CoreAction::NOTCORE;
    message->status = status;
    message->resource = std::move(resource);
    return message;
}

std::shared_ptr<sharedmodel::UniterMessage> makeProtocolMessage(sharedmodel::CoreAction action) {
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
    auto message = makeProtocolMessage(sharedmodel::CoreAction::BEGIN_TRANSACTION);
    message->add_data[sharedmodel::AddDataTransactionId] = transactionId;

    std::shared_ptr<TransactionQueryMessage> query{new TransactionQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

std::shared_ptr<TransactionQueryMessage> TransactionQueryMessage::createFinish(
    const std::string& transactionId,
    TransactionAction action) {
    auto message = makeProtocolMessage(sharedmodel::CoreAction::COMMIT_ROLLBACK_TXN);
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

std::shared_ptr<FileAccessQueryMessage> FileAccessQueryMessage::create(
    const std::string& object,
    const std::string& accessMode) {
    auto message = makeProtocolMessage(sharedmodel::CoreAction::FILE_ACCESS);
    message->add_data[sharedmodel::AddDataFileObject] = object;
    message->add_data[sharedmodel::AddDataFileAccessMode] = accessMode;

    std::shared_ptr<FileAccessQueryMessage> query{new FileAccessQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

FileAccessQueryMessage::FileAccessQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<GetFileQueryMessage> GetFileQueryMessage::create(
    const std::string& object,
    const std::string& fileAccessUrl) {
    auto message = makeProtocolMessage(sharedmodel::CoreAction::GET_FILE);
    message->add_data[sharedmodel::AddDataFileObject] = object;
    message->add_data[sharedmodel::AddDataFileAccessUrl] = fileAccessUrl;

    std::shared_ptr<GetFileQueryMessage> query{new GetFileQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

GetFileQueryMessage::GetFileQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<PutFileQueryMessage> PutFileQueryMessage::create(
    const std::string& object,
    const std::string& fileAccessUrl,
    const std::filesystem::path& localPath) {
    auto message = makeProtocolMessage(sharedmodel::CoreAction::PUT_FILE);
    message->add_data[sharedmodel::AddDataFileObject] = object;
    message->add_data[sharedmodel::AddDataFileAccessUrl] = fileAccessUrl;
    message->add_data[sharedmodel::AddDataLocalPath] = localPath.string();

    std::shared_ptr<PutFileQueryMessage> query{new PutFileQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

PutFileQueryMessage::PutFileQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<UpdateCheckQueryMessage> UpdateCheckQueryMessage::create(const std::string& currentVersion) {
    auto message = makeProtocolMessage(sharedmodel::ProtocolAction::UPDATE_CHECK);
    message->add_data[sharedmodel::AddDataUpdateVersion] = currentVersion;

    std::shared_ptr<UpdateCheckQueryMessage> query{new UpdateCheckQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

UpdateCheckQueryMessage::UpdateCheckQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

std::shared_ptr<GetMigrationsQueryMessage> GetMigrationsQueryMessage::create() {
    auto message = makeProtocolMessage(sharedmodel::ProtocolAction::GET_MIGRATIONS);
    message->add_data[sharedmodel::AddDataDataModelVersion] = sharedmodel::DataModelVersion;

    std::shared_ptr<GetMigrationsQueryMessage> query{new GetMigrationsQueryMessage(std::move(message))};
    MessageManager::instance().query(query);
    return query->message() && query->message()->sequence_id ? query : nullptr;
}

GetMigrationsQueryMessage::GetMigrationsQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {
}

} // namespace serverbrige
