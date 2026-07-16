#ifndef UNITER_NETBRIDGE_NETBRIDGE_H
#define UNITER_NETBRIDGE_NETBRIDGE_H

#include <unitermessage.h>

#include <QObject>

#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace netbridge {

class EventMessage;
class QueryMessage;

class NetBridge : public QObject {
    Q_OBJECT
public:
    static NetBridge& instance() {
        static NetBridge instance;
        return instance;
    }

    NetBridge(const NetBridge&) = delete;
    NetBridge& operator=(const NetBridge&) = delete;
    NetBridge(NetBridge&&) = delete;
    NetBridge& operator=(NetBridge&&) = delete;
    ~NetBridge() override = default;

    bool query(const std::shared_ptr<QueryMessage>& message);
    bool sendMessage(const std::shared_ptr<EventMessage>& message);

private:
    NetBridge();

    bool prepareMessage(
        const std::shared_ptr<sharedmodel::UniterMessage>& message,
        sharedmodel::MessageStatus expectedStatus);
    void receiveMessage(
        std::shared_ptr<sharedmodel::UniterMessage> message,
        sharedmodel::Endpoint expectedEndpoint);

    uint64_t sequence_id_{0};
    std::map<uint64_t, std::weak_ptr<QueryMessage>> queries_;
    std::string token_;

public slots:
    void setToken(const std::string& token);
    void refreshToken(const std::string& token);
    void onReceivePublicMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
    void onReceiveTenantMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
    void onReceiveCrudMessage(std::shared_ptr<sharedmodel::UniterMessage> message);

signals:
    void signalSendPublicMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
    void signalSendTenantMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
    void signalSendCrudMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace netbridge

#endif // UNITER_NETBRIDGE_NETBRIDGE_H
