#ifndef UNITER_MESSAGING_MESSAGEMANAGER_H
#define UNITER_MESSAGING_MESSAGEMANAGER_H

#include "messageobserver.h"

#include <cstdint>
#include <map>
#include <memory>

namespace messaging {

class MessageManager {
public:
    std::shared_ptr<MessageObserver> track(std::shared_ptr<contract::UniterMessage> message);
    bool routeIncomingDirectResponse(std::shared_ptr<contract::UniterMessage> message);

private:
    uint64_t nextSequenceId_ = 1;
    std::map<uint64_t, std::shared_ptr<MessageObserver>> observers_;
};

} // namespace messaging

#endif // UNITER_MESSAGING_MESSAGEMANAGER_H
