#ifndef UNITER_SERVERBRIGE_SPECIALIZEDMESSAGES_H
#define UNITER_SERVERBRIGE_SPECIALIZEDMESSAGES_H

#include "eventmessage.h"
#include "querymessage.h"

#include <common/resourceabstract.h>

#include <filesystem>
#include <memory>
#include <string>

namespace serverbrige {

enum class TransactionAction {
    COMMIT,
    ROLLBACK
};

class CrudEventMessage : public EventMessage {
public:
    static std::shared_ptr<CrudEventMessage> create(
        sharedmodel::CrudAction action,
        std::shared_ptr<sharedmodel::ResourceAbstract> resource);

private:
    explicit CrudEventMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class CrudQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<CrudQueryMessage> create(
        sharedmodel::CrudAction action,
        std::shared_ptr<sharedmodel::ResourceAbstract> resource);

private:
    explicit CrudQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class TransactionQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<TransactionQueryMessage> createBegin(const std::string& transactionId);
    static std::shared_ptr<TransactionQueryMessage> createFinish(
        const std::string& transactionId,
        TransactionAction action);

private:
    explicit TransactionQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class FileAccessQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<FileAccessQueryMessage> create(
        const std::string& object,
        const std::string& accessMode);

private:
    explicit FileAccessQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class GetFileQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<GetFileQueryMessage> create(
        const std::string& object,
        const std::string& fileAccessUrl);

private:
    explicit GetFileQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class PutFileQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<PutFileQueryMessage> create(
        const std::string& object,
        const std::string& fileAccessUrl,
        const std::filesystem::path& localPath);

private:
    explicit PutFileQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class UpdateCheckQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<UpdateCheckQueryMessage> create(const std::string& currentVersion = sharedmodel::DataModelVersion);

private:
    explicit UpdateCheckQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class GetMigrationsQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<GetMigrationsQueryMessage> create();

private:
    explicit GetMigrationsQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace serverbrige

#endif // UNITER_SERVERBRIGE_SPECIALIZEDMESSAGES_H
