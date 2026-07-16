#ifndef UNITER_NETBRIDGE_EVENTMESSAGE_H
#define UNITER_NETBRIDGE_EVENTMESSAGE_H

#include <unitermessage.h>

#include <QObject>

#include <memory>

namespace netbridge {

class NetBridge;

class EventMessage : public QObject {
    Q_OBJECT
public:
    ~EventMessage() override = default;

    [[nodiscard]] const std::shared_ptr<sharedmodel::UniterMessage>& message() const noexcept { return message_; }

protected:
    explicit EventMessage(std::shared_ptr<sharedmodel::UniterMessage> message);

private:
    friend class NetBridge;

    void notify_sent() { emit signalSent(message_); }

    std::shared_ptr<sharedmodel::UniterMessage> message_;

signals:
    void signalSent(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace netbridge

#endif // UNITER_NETBRIDGE_EVENTMESSAGE_H
