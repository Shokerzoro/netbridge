#ifndef UNITER_SERVERBRIGE_QUERYMESSAGE_H
#define UNITER_SERVERBRIGE_QUERYMESSAGE_H

#include <unitermessage.h>

#include <QObject>

#include <cstdint>
#include <memory>

namespace serverbrige {

class MessageManager;

class QueryMessage : public QObject {
    Q_OBJECT
public:
    static std::shared_ptr<QueryMessage> create(std::shared_ptr<sharedmodel::UniterMessage> message);
    ~QueryMessage() override = default;

    [[nodiscard]] const std::shared_ptr<sharedmodel::UniterMessage>& message() const noexcept { return message_; }
    [[nodiscard]] const std::shared_ptr<sharedmodel::UniterMessage>& response() const noexcept { return response_; }
    [[nodiscard]] uint64_t sequenceId() const noexcept { return sequence_id_; }

protected:
    explicit QueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);

private:
    friend class MessageManager;

    void setSequenceId(uint64_t id);
    void setResponse(std::shared_ptr<sharedmodel::UniterMessage> message);
    void notify_received() { emit signalReceived(response_); }

    std::shared_ptr<sharedmodel::UniterMessage> message_;
    std::shared_ptr<sharedmodel::UniterMessage> response_;
    uint64_t sequence_id_{0};

signals:
    void signalReceived(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace serverbrige

#endif // UNITER_SERVERBRIGE_QUERYMESSAGE_H
