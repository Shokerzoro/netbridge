#include "crudhandler.h"

#include <crudmessages.h>
#include <querymessage.h>
#include <tenantmessages.h>
#include <unitermessage.h>

#include <QMetaObject>

#include <algorithm>
#include <utility>

namespace netbridge {

CrudHandler::CrudHandler(QObject* parent)
    : QObject(parent)
{
    request_timer_.setSingleShot(true);
    connect(&request_timer_, &QTimer::timeout, this, &CrudHandler::handleTimeout);
}

bool CrudHandler::add(std::shared_ptr<sharedmodel::ResourceAbstract> resource)
{
    return stage(sharedmodel::CrudAction::CREATE, resource);
}

bool CrudHandler::update(std::shared_ptr<sharedmodel::ResourceAbstract> resource)
{
    return stage(sharedmodel::CrudAction::UPDATE, resource);
}

bool CrudHandler::remove(std::shared_ptr<sharedmodel::ResourceAbstract> resource)
{
    return stage(sharedmodel::CrudAction::DELETE, resource);
}

void CrudHandler::setRequestTimeout(int milliseconds)
{
    if (state_ == State::Draft && milliseconds > 0) {
        request_timeout_ms_ = milliseconds;
    }
}

bool CrudHandler::stage(
    sharedmodel::CrudAction action,
    const std::shared_ptr<sharedmodel::ResourceAbstract>& resource)
{
    if (state_ != State::Draft) {
        last_error_ = "The CRUD transaction has already started";
        return false;
    }

    const auto info = resource_fabric_.resourceInfo(resource);
    if (!info || !sharedmodel::isUuidV7(info->id.id)) {
        last_error_ = "A registered resource with a UUIDv7 identity is required";
        return false;
    }

    auto snapshot = resource_fabric_.clone(resource);
    if (!snapshot) {
        last_error_ = "Unable to copy the staged resource";
        return false;
    }

    const auto existing = std::find_if(
        operations_.begin(), operations_.end(), [&](const Operation& operation) {
            return operation.resource->key == snapshot->key &&
                   operation.resource->id == snapshot->id;
        });

    if (existing == operations_.end()) {
        operations_.push_back({action, std::move(snapshot)});
        last_error_.clear();
        return true;
    }

    if (existing->action == sharedmodel::CrudAction::DELETE) {
        last_error_ = "A deleted resource cannot be staged again";
        return false;
    }
    if (existing->action == sharedmodel::CrudAction::UPDATE &&
        action == sharedmodel::CrudAction::CREATE) {
        last_error_ = "An updated resource cannot be staged as newly created";
        return false;
    }
    if (existing->action == sharedmodel::CrudAction::CREATE &&
        action == sharedmodel::CrudAction::DELETE) {
        operations_.erase(existing);
        last_error_.clear();
        return true;
    }

    if (existing->action != sharedmodel::CrudAction::CREATE) {
        existing->action = action;
    }
    existing->resource = std::move(snapshot);
    last_error_.clear();
    return true;
}

bool CrudHandler::commit()
{
    if (state_ != State::Draft) {
        last_error_ = "The CRUD transaction is one-shot";
        return false;
    }
    if (operations_.empty()) {
        state_ = State::Finished;
        QMetaObject::invokeMethod(this, [this] {
            emit signalFinished(true, sharedmodel::ErrorCode::SUCCESS, {});
        }, Qt::QueuedConnection);
        return true;
    }

    begin();
    return state_ != State::Finished || last_error_.isEmpty();
}

void CrudHandler::begin()
{
    state_ = State::Beginning;
    auto query = netbridge::BeginTransactionQueryMessage::create();
    if (!query) {
        finish(false, sharedmodel::ErrorCode::SERVICE_UNAVAILABLE,
               "Unable to submit BEGIN_TRANSACTION");
        return;
    }
    track(query);
    const auto handleResponse =
        [this](const std::shared_ptr<sharedmodel::UniterMessage>& response) {
        request_timer_.stop();
        if (!succeeded(response)) {
            finish(false, response ? response->error : sharedmodel::ErrorCode::SERVICE_UNAVAILABLE,
                   errorText(response));
            return;
        }
        const auto transaction = response->add_data.find(sharedmodel::AddDataTransactionId);
        if (transaction == response->add_data.end() || transaction->second.empty()) {
            finish(false, sharedmodel::ErrorCode::BAD_REQUEST,
                   "BEGIN_TRANSACTION response has no transaction ID");
            return;
        }
        transaction_id_ = transaction->second;
        operation_index_ = 0;
        sendNext();
    };
    connect(query.get(), &netbridge::QueryMessage::signalReceived, this,
            handleResponse);
    if (query->response()) handleResponse(query->response());
}

void CrudHandler::sendNext()
{
    if (operation_index_ == operations_.size()) {
        commitTransaction();
        return;
    }

    state_ = State::Sending;
    const auto& operation = operations_[operation_index_];
    auto query = netbridge::CrudQueryMessage::create(
        operation.action, operation.resource, transaction_id_);
    if (!query) {
        rollback(sharedmodel::ErrorCode::SERVICE_UNAVAILABLE,
                 "Unable to submit a CRUD request");
        return;
    }
    track(query);
    const auto handleResponse =
        [this](const std::shared_ptr<sharedmodel::UniterMessage>& response) {
        request_timer_.stop();
        if (!succeeded(response)) {
            rollback(response ? response->error : sharedmodel::ErrorCode::SERVICE_UNAVAILABLE,
                     errorText(response));
            return;
        }
        ++operation_index_;
        sendNext();
    };
    connect(query.get(), &netbridge::QueryMessage::signalReceived, this,
            handleResponse);
    if (query->response()) handleResponse(query->response());
}

void CrudHandler::commitTransaction()
{
    state_ = State::Committing;
    auto query = netbridge::EndTransactionQueryMessage::create(
        transaction_id_, netbridge::TransactionAction::COMMIT);
    if (!query) {
        rollback(sharedmodel::ErrorCode::SERVICE_UNAVAILABLE,
                 "Unable to submit COMMIT");
        return;
    }
    track(query);
    const auto handleResponse =
        [this](const std::shared_ptr<sharedmodel::UniterMessage>& response) {
        request_timer_.stop();
        if (succeeded(response)) {
            finish(true, sharedmodel::ErrorCode::SUCCESS, {});
        } else {
            rollback(response ? response->error : sharedmodel::ErrorCode::SERVICE_UNAVAILABLE,
                     errorText(response));
        }
    };
    connect(query.get(), &netbridge::QueryMessage::signalReceived, this,
            handleResponse);
    if (query->response()) handleResponse(query->response());
}

void CrudHandler::rollback(sharedmodel::ErrorCode error, QString text)
{
    rollback_error_ = error;
    rollback_text_ = std::move(text);
    if (transaction_id_.empty()) {
        finish(false, rollback_error_, rollback_text_);
        return;
    }

    state_ = State::RollingBack;
    auto query = netbridge::EndTransactionQueryMessage::create(
        transaction_id_, netbridge::TransactionAction::ROLLBACK);
    if (!query) {
        finish(false, rollback_error_, rollback_text_ + "; rollback submission failed");
        return;
    }
    track(query);
    const auto handleResponse =
        [this](const std::shared_ptr<sharedmodel::UniterMessage>& response) {
        request_timer_.stop();
        QString text = rollback_text_;
        if (!succeeded(response)) {
            text += "; rollback was not confirmed";
        }
        finish(false, rollback_error_, std::move(text));
    };
    connect(query.get(), &netbridge::QueryMessage::signalReceived, this,
            handleResponse);
    if (query->response()) handleResponse(query->response());
}

void CrudHandler::finish(bool success, sharedmodel::ErrorCode error, QString text)
{
    if (state_ == State::Finished) return;
    request_timer_.stop();
    current_query_.reset();
    state_ = State::Finished;
    last_error_ = text;
    emit signalFinished(success, error, std::move(text));
}

void CrudHandler::track(const std::shared_ptr<netbridge::QueryMessage>& query)
{
    current_query_ = query;
    startTimeout();
}

void CrudHandler::startTimeout()
{
    request_timer_.start(request_timeout_ms_);
}

void CrudHandler::handleTimeout()
{
    current_query_.reset();
    if (state_ == State::RollingBack) {
        finish(false, rollback_error_, rollback_text_ + "; rollback timed out");
    } else if (state_ == State::Beginning) {
        finish(false, sharedmodel::ErrorCode::SERVICE_UNAVAILABLE,
               "BEGIN_TRANSACTION timed out");
    } else {
        rollback(sharedmodel::ErrorCode::SERVICE_UNAVAILABLE,
                 "CRUD transaction request timed out");
    }
}

bool CrudHandler::succeeded(
    const std::shared_ptr<sharedmodel::UniterMessage>& response) noexcept
{
    return response && response->status == sharedmodel::MessageStatus::SUCCESS;
}

QString CrudHandler::errorText(
    const std::shared_ptr<sharedmodel::UniterMessage>& response)
{
    if (!response) return "No response was received";
    const auto value = response->add_data.find(sharedmodel::AddDataErrorText);
    return value != response->add_data.end() && !value->second.empty()
        ? QString::fromStdString(value->second)
        : QString{"The server rejected the CRUD transaction request"};
}

} // namespace netbridge
