#ifndef UNITER_SERVERBRIGE_MESSAGEMANAGER_H
#define UNITER_SERVERBRIGE_MESSAGEMANAGER_H

#include <sharedmodel/unitermessage.h>

#include <QObject>

#include <cstdint>
#include <map>
#include <memory>

namespace serverbrige {

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
    void onRecvUniterMessage(std::shared_ptr<sharedmodel::UniterMessage> message);

signals:
    void signalSendMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace serverbrige

#endif // UNITER_SERVERBRIGE_MESSAGEMANAGER_H
