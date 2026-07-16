#ifndef UNITER_TENANTBRIDGE_SPECIALIZEDMESSAGES_H
#define UNITER_TENANTBRIDGE_SPECIALIZEDMESSAGES_H

#include "eventmessage.h"
#include "querymessage.h"

#include <common/resourceabstract.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace tenantbridge {

enum class TransactionAction {
    COMMIT,
    ROLLBACK
};

enum class FileAccessMode {
    READ,
    WRITE
};

class GetUserQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<GetUserQueryMessage> create();

private:
    explicit GetUserQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class GetKafkaQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<GetKafkaQueryMessage> create(uint64_t kafkaOffset);

private:
    explicit GetKafkaQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class FullSyncQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<FullSyncQueryMessage> create();

private:
    explicit FullSyncQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class BeginTransactionQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<BeginTransactionQueryMessage> create();

private:
    explicit BeginTransactionQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class EndTransactionQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<EndTransactionQueryMessage> create(
        const std::string& transactionId,
        TransactionAction action);

private:
    explicit EndTransactionQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class FileAccessQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<FileAccessQueryMessage> create(
        const std::string& objectKey,
        FileAccessMode mode);

private:
    explicit FileAccessQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class CrudQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<CrudQueryMessage> create(
        sharedmodel::CrudAction action,
        std::shared_ptr<sharedmodel::ResourceAbstract> resource,
        std::optional<std::string> transactionId = std::nullopt);

private:
    explicit CrudQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class CrudEventMessage : public EventMessage {
public:
    static std::shared_ptr<CrudEventMessage> create(
        sharedmodel::CrudAction action,
        std::shared_ptr<sharedmodel::ResourceAbstract> resource,
        std::optional<std::string> transactionId = std::nullopt);

private:
    explicit CrudEventMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace tenantbridge

#endif // UNITER_TENANTBRIDGE_SPECIALIZEDMESSAGES_H
