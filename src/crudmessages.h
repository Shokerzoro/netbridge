#ifndef UNITER_NETBRIDGE_CRUDMESSAGES_H
#define UNITER_NETBRIDGE_CRUDMESSAGES_H

#include "eventmessage.h"
#include "querymessage.h"

#include <common/resourceabstract.h>

#include <memory>
#include <optional>
#include <string>

namespace netbridge {

class CrudQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<CrudQueryMessage> create(
        sharedmodel::CrudAction action,
        std::shared_ptr<sharedmodel::ResourceAbstract> resource,
        std::optional<std::string> transactionId = std::nullopt);
private:
    explicit CrudQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class CrudEventMessage : public EventMessage {
public:
    static std::shared_ptr<CrudEventMessage> create(
        sharedmodel::CrudAction action,
        std::shared_ptr<sharedmodel::ResourceAbstract> resource,
        std::optional<std::string> transactionId = std::nullopt);
private:
    explicit CrudEventMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace netbridge

#endif // UNITER_NETBRIDGE_CRUDMESSAGES_H
