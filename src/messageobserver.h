#ifndef UNITER_SERVERBRIGE_MESSAGEOBSERVER_H
#define UNITER_SERVERBRIGE_MESSAGEOBSERVER_H

#include <sharedmodel/unitermessage.h>

#include <cstdint>
#include <memory>

namespace serverbrige {

class MessageObserver {
public:
    enum class State {
        Pending,
        Success,
        Error,
        Timeout
    };

    explicit MessageObserver(uint64_t expectedSequenceId);

    uint64_t expectedSequenceId() const;
    State state() const;
    const std::shared_ptr<sharedmodel::UniterMessage>& response() const;

    void recordResponse(std::shared_ptr<sharedmodel::UniterMessage> message);
    void recordError(std::shared_ptr<sharedmodel::UniterMessage> message);
    void recordTimeout();

private:
    uint64_t expectedSequenceId_;
    State state_ = State::Pending;
    std::shared_ptr<sharedmodel::UniterMessage> response_;
};

} // namespace serverbrige

#endif // UNITER_SERVERBRIGE_MESSAGEOBSERVER_H
