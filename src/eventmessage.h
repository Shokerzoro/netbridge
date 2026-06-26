#ifndef UNITER_SERVERBRIGE_EVENTMESSAGE_H
#define UNITER_SERVERBRIGE_EVENTMESSAGE_H

#include <sharedmodel/unitermessage.h>

#include <QObject>

#include <memory>

namespace serverbrige {

class MessageManager;

class EventMessage : public QObject {
    Q_OBJECT
public:
    static std::shared_ptr<EventMessage> create(std::shared_ptr<sharedmodel::UniterMessage> message);
    ~EventMessage() override = default;

    [[nodiscard]] const std::shared_ptr<sharedmodel::UniterMessage>& message() const noexcept { return message_; }

protected:
    explicit EventMessage(std::shared_ptr<sharedmodel::UniterMessage> message);

private:
    friend class MessageManager;

    void notify_sent() { emit signalSent(message_); }

    std::shared_ptr<sharedmodel::UniterMessage> message_;

signals:
    void signalSent(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace serverbrige

#endif // UNITER_SERVERBRIGE_EVENTMESSAGE_H
