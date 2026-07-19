#ifndef UNITER_NETBRIDGE_CRUDHANDLER_H
#define UNITER_NETBRIDGE_CRUDHANDLER_H

#include <resourcefabric.h>
#include <unitermessage.h>
#include <uniterprotocol.h>

#include <QObject>
#include <QString>
#include <QTimer>

#include <memory>
#include <string>
#include <vector>

namespace netbridge {
class QueryMessage;
}

namespace netbridge {

class CrudHandler final : public QObject {
    Q_OBJECT
public:
    enum class State {
        Draft,
        Beginning,
        Sending,
        Committing,
        RollingBack,
        Finished
    };
    Q_ENUM(State)

    explicit CrudHandler(QObject* parent = nullptr);

    bool add(std::shared_ptr<sharedmodel::ResourceAbstract> resource);
    bool update(std::shared_ptr<sharedmodel::ResourceAbstract> resource);
    bool remove(std::shared_ptr<sharedmodel::ResourceAbstract> resource);
    bool commit();

    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] QString lastError() const { return last_error_; }
    void setRequestTimeout(int milliseconds);

signals:
    void signalFinished(bool success,
                        sharedmodel::ErrorCode error,
                        QString errorText);

private:
    struct Operation {
        sharedmodel::CrudAction action{sharedmodel::CrudAction::NOTCRUD};
        std::shared_ptr<sharedmodel::ResourceAbstract> resource;
    };

    bool stage(sharedmodel::CrudAction action,
               const std::shared_ptr<sharedmodel::ResourceAbstract>& resource);
    void begin();
    void sendNext();
    void commitTransaction();
    void rollback(sharedmodel::ErrorCode error, QString text);
    void finish(bool success, sharedmodel::ErrorCode error, QString text);
    void track(const std::shared_ptr<netbridge::QueryMessage>& query);
    void startTimeout();
    void handleTimeout();

    static bool succeeded(const std::shared_ptr<sharedmodel::UniterMessage>& response) noexcept;
    static QString errorText(const std::shared_ptr<sharedmodel::UniterMessage>& response);

    sharedmodel::ResourceFabric resource_fabric_;
    std::vector<Operation> operations_;
    std::shared_ptr<netbridge::QueryMessage> current_query_;
    std::string transaction_id_;
    std::size_t operation_index_{0};
    State state_{State::Draft};
    QString last_error_;
    QTimer request_timer_;
    int request_timeout_ms_{30000};
    sharedmodel::ErrorCode rollback_error_{sharedmodel::ErrorCode::SUCCESS};
    QString rollback_text_;
};

} // namespace netbridge

Q_DECLARE_METATYPE(sharedmodel::ErrorCode)

#endif // UNITER_NETBRIDGE_CRUDHANDLER_H
