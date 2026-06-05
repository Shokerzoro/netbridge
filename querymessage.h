#ifndef UNITER_MESSAGING_QUERYMESSAGE_H
#define UNITER_MESSAGING_QUERYMESSAGE_H

#include <contract/unitermessage.h>

#include <QObject>

#include <cstdint>
#include <memory>

namespace messaging {

class MessageManager;

class QueryMessage : public QObject {
    Q_OBJECT
public:
    static std::shared_ptr<QueryMessage> create(std::shared_ptr<contract::UniterMessage> message);
    ~QueryMessage() override = default;

    [[nodiscard]] const std::shared_ptr<contract::UniterMessage>& message() const noexcept { return message_; }
    [[nodiscard]] const std::shared_ptr<contract::UniterMessage>& response() const noexcept { return response_; }
    [[nodiscard]] uint64_t sequenceId() const noexcept { return sequence_id_; }

protected:
    explicit QueryMessage(std::shared_ptr<contract::UniterMessage> message);

private:
    friend class MessageManager;

    void setSequenceId(uint64_t id);
    void setResponse(std::shared_ptr<contract::UniterMessage> message);
    void notify_received() { emit signalReceived(response_); }

    std::shared_ptr<contract::UniterMessage> message_;
    std::shared_ptr<contract::UniterMessage> response_;
    uint64_t sequence_id_{0};

signals:
    void signalReceived(std::shared_ptr<contract::UniterMessage> message);
};

} // namespace messaging

#endif // UNITER_MESSAGING_QUERYMESSAGE_H
