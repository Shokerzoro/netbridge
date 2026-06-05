#ifndef UNITER_MESSAGING_MESSAGEMANAGER_H
#define UNITER_MESSAGING_MESSAGEMANAGER_H

#include <contract/unitermessage.h>

#include <QObject>

#include <cstdint>
#include <map>
#include <memory>

namespace messaging {

class EventMessage;
class QueryMessage;

class MessageManager : public QObject {
    Q_OBJECT
public:
    static MessageManager& instance() {
        static MessageManager instance;
        return instance;
    }

    MessageManager(const MessageManager&) = delete;
    MessageManager& operator=(const MessageManager&) = delete;
    MessageManager(MessageManager&&) = delete;
    MessageManager& operator=(MessageManager&&) = delete;
    ~MessageManager() override = default;

    void query(std::shared_ptr<QueryMessage> message);
    void sendMessage(std::shared_ptr<EventMessage> message);

private:
    MessageManager();

    friend class QueryMessage;
    friend class EventMessage;

    uint64_t sequence_id{0};
    std::map<uint64_t, std::weak_ptr<QueryMessage>> queries;

public slots:
    void onRecvUniterMessage(std::shared_ptr<contract::UniterMessage> message);

signals:
    void signalSendMessage(std::shared_ptr<contract::UniterMessage> message);
};

} // namespace messaging

#endif // UNITER_MESSAGING_MESSAGEMANAGER_H
