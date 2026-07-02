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

class PresignedUrlQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<PresignedUrlQueryMessage> create(const std::string& object);

private:
    explicit PresignedUrlQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class DownloadFileQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<DownloadFileQueryMessage> create(
        const std::string& object,
        const std::string& presignedUrl);

private:
    explicit DownloadFileQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class UploadFileQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<UploadFileQueryMessage> create(
        const std::string& object,
        const std::string& presignedUrl,
        const std::filesystem::path& localPath);

private:
    explicit UploadFileQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace serverbrige

#endif // UNITER_SERVERBRIGE_SPECIALIZEDMESSAGES_H
