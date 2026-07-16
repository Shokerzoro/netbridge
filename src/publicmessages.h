#ifndef UNITER_NETBRIDGE_PUBLICMESSAGES_H
#define UNITER_NETBRIDGE_PUBLICMESSAGES_H

#include "querymessage.h"

#include <memory>
#include <string>

namespace netbridge {

enum class PublicFileAccessMode { READ, WRITE };

class GetTokenQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<GetTokenQueryMessage> create(
        const std::string& login,
        const std::string& password);
private:
    explicit GetTokenQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class RefreshTokenQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<RefreshTokenQueryMessage> create(const std::string& refreshToken);
private:
    explicit RefreshTokenQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class CreateEmployeeQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<CreateEmployeeQueryMessage> create(
        const std::string& email,
        const std::string& name);
private:
    explicit CreateEmployeeQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class RegisterCompanyQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<RegisterCompanyQueryMessage> create(
        const std::string& companyName,
        const std::string& adminLogin,
        const std::string& adminEmail,
        const std::string& adminPassword);
private:
    explicit RegisterCompanyQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class GetUpdateQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<GetUpdateQueryMessage> create(const std::string& appVersion);
private:
    explicit GetUpdateQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class GetMigrationsQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<GetMigrationsQueryMessage> create();
private:
    explicit GetMigrationsQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

class PublicFileAccessQueryMessage : public QueryMessage {
public:
    static std::shared_ptr<PublicFileAccessQueryMessage> create(
        const std::string& objectKey,
        PublicFileAccessMode mode);
private:
    explicit PublicFileAccessQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message);
};

} // namespace netbridge

#endif // UNITER_NETBRIDGE_PUBLICMESSAGES_H
