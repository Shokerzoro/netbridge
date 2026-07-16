#ifndef UNITER_NETBRIDGE_TENANTMESSAGES_H
#define UNITER_NETBRIDGE_TENANTMESSAGES_H

#include "querymessage.h"

#include <cstdint>
#include <memory>
#include <string>

namespace netbridge {

enum class TransactionAction { COMMIT, ROLLBACK };
enum class TenantFileAccessMode { READ, WRITE };

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

class TenantFileAccessQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<TenantFileAccessQueryMessage> create(
        const std::string& objectKey,
        TenantFileAccessMode mode);
private:
    explicit TenantFileAccessQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace netbridge

#endif // UNITER_NETBRIDGE_TENANTMESSAGES_H
