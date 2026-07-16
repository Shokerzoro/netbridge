#ifndef UNITER_TENANTBRIDGE_TENANTMESSAGER_H
#define UNITER_TENANTBRIDGE_TENANTMESSAGER_H

#include <unitermessage.h>

#include <QObject>

#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace tenantbridge {

class EventMessage;
class QueryMessage;

class TenantMessager : public QObject {
    Q_OBJECT
public:
    static TenantMessager& instance() {
        static TenantMessager instance;
        return instance;
    }

    TenantMessager(const TenantMessager&) = delete;
    TenantMessager& operator=(const TenantMessager&) = delete;
    TenantMessager(TenantMessager&&) = delete;
    TenantMessager& operator=(TenantMessager&&) = delete;
    ~TenantMessager() override = default;

    bool query(const std::shared_ptr<QueryMessage>& message);
    bool sendMessage(const std::shared_ptr<EventMessage>& message);

private:
    TenantMessager();

    bool prepareMessage(
        const std::shared_ptr<sharedmodel::UniterMessage>& message,
        sharedmodel::MessageStatus expectedStatus);

    uint64_t sequence_id{0};
    std::map<uint64_t, std::weak_ptr<QueryMessage>> queries;
    std::string token_;

public slots:
    void setToken(const std::string& token);
    void refreshToken(const std::string& token);
    void onRecvUniterMessage(std::shared_ptr<sharedmodel::UniterMessage> message);

signals:
    void signalSendMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace tenantbridge

#endif // UNITER_TENANTBRIDGE_TENANTMESSAGER_H
