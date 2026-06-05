#ifndef UNITER_MESSAGING_MESSAGEOBSERVER_H
#define UNITER_MESSAGING_MESSAGEOBSERVER_H

#include <contract/unitermessage.h>

#include <cstdint>
#include <memory>

namespace messaging {

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
    const std::shared_ptr<contract::UniterMessage>& response() const;

    void recordResponse(std::shared_ptr<contract::UniterMessage> message);
    void recordError(std::shared_ptr<contract::UniterMessage> message);
    void recordTimeout();

private:
    uint64_t expectedSequenceId_;
    State state_ = State::Pending;
    std::shared_ptr<contract::UniterMessage> response_;
};

} // namespace messaging

#endif // UNITER_MESSAGING_MESSAGEOBSERVER_H
