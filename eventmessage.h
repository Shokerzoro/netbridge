#ifndef UNITER_MESSAGING_EVENTMESSAGE_H
#define UNITER_MESSAGING_EVENTMESSAGE_H

#include <contract/unitermessage.h>

#include <QObject>

#include <memory>

namespace messaging {

class MessageManager;

class EventMessage : public QObject {
    Q_OBJECT
public:
    static std::shared_ptr<EventMessage> create(std::shared_ptr<contract::UniterMessage> message);
    ~EventMessage() override = default;

    [[nodiscard]] const std::shared_ptr<contract::UniterMessage>& message() const noexcept { return message_; }

protected:
    explicit EventMessage(std::shared_ptr<contract::UniterMessage> message);

private:
    friend class MessageManager;

    void notify_sent() { emit signalSent(message_); }

    std::shared_ptr<contract::UniterMessage> message_;

signals:
    void signalSent(std::shared_ptr<contract::UniterMessage> message);
};

} // namespace messaging

#endif // UNITER_MESSAGING_EVENTMESSAGE_H
