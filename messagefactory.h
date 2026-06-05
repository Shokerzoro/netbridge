#ifndef UNITER_MESSAGING_MESSAGEFACTORY_H
#define UNITER_MESSAGING_MESSAGEFACTORY_H

#include <contract/unitermessage.h>

#include <filesystem>
#include <memory>
#include <string>

namespace messaging {

enum class TransactionAction {
    COMMIT,
    ROLLBACK
};

class MessageFactory {
public:
    static std::shared_ptr<contract::UniterMessage> makeCrudMessage(
        contract::CrudAction action,
        std::shared_ptr<contract::ResourceAbstract> resource);

    static std::shared_ptr<contract::UniterMessage> makeCreateMessage(
        std::shared_ptr<contract::ResourceAbstract> resource);
    static std::shared_ptr<contract::UniterMessage> makeUpdateMessage(
        std::shared_ptr<contract::ResourceAbstract> resource);
    static std::shared_ptr<contract::UniterMessage> makeDeleteMessage(
        std::shared_ptr<contract::ResourceAbstract> resource);

    static std::shared_ptr<contract::UniterMessage> makeBeginTransactionMessage(
        const std::string& transactionId);
    static std::shared_ptr<contract::UniterMessage> makeFinishTransactionMessage(
        const std::string& transactionId,
        TransactionAction action);

    static std::shared_ptr<contract::UniterMessage> makePresignedUrlRequest(
        const std::string& object);
    static std::shared_ptr<contract::UniterMessage> makeFileRequest(
        const std::string& object,
        const std::string& presignedUrl);
    static std::shared_ptr<contract::UniterMessage> makeUploadFileRequest(
        const std::string& object,
        const std::string& presignedUrl,
        const std::filesystem::path& localPath);
};

} // namespace messaging

#endif // UNITER_MESSAGING_MESSAGEFACTORY_H
