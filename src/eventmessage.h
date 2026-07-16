#ifndef UNITER_TENANTBRIDGE_EVENTMESSAGE_H
#define UNITER_TENANTBRIDGE_EVENTMESSAGE_H

#include <unitermessage.h>

#include <QObject>

#include <memory>

namespace tenantbridge {

class TenantMessager;

class EventMessage : public QObject {
    Q_OBJECT
public:
    ~EventMessage() override = default;

    [[nodiscard]] const std::shared_ptr<sharedmodel::UniterMessage>& message() const noexcept { return message_; }

protected:
    explicit EventMessage(std::shared_ptr<sharedmodel::UniterMessage> message);

private:
    friend class TenantMessager;

    void notify_sent() { emit signalSent(message_); }

    std::shared_ptr<sharedmodel::UniterMessage> message_;

signals:
    void signalSent(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace tenantbridge

#endif // UNITER_TENANTBRIDGE_EVENTMESSAGE_H
