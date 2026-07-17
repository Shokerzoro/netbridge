#include "crudmessages.h"

#include "netbridge.h"

#include <stdexcept>
#include <utility>

namespace netbridge {
namespace {

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
    if (transactionId && transactionId->empty()) {
        throw std::invalid_argument("Transaction ID cannot be empty");
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
    if (transactionId) {
        message->add_data[sharedmodel::AddDataTransactionId] = *transactionId;
    }
    return message;
}

} // namespace

std::shared_ptr<CrudQueryMessage> CrudQueryMessage::create(
    sharedmodel::CrudAction action,
    std::shared_ptr<sharedmodel::ResourceAbstract> resource,
    std::optional<std::string> transactionId) {
    auto message = makeCrudMessage(
        action,
        sharedmodel::MessageStatus::REQUEST,
        std::move(resource),
        transactionId);
    std::shared_ptr<CrudQueryMessage> query{new CrudQueryMessage(std::move(message))};
    return NetBridge::instance().query(query) ? query : nullptr;
}

CrudQueryMessage::CrudQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

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
    return NetBridge::instance().sendMessage(event) ? event : nullptr;
}

CrudEventMessage::CrudEventMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : EventMessage(std::move(message)) {}

} // namespace netbridge
