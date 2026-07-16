#include "publicmessages.h"

#include "netbridge.h"

#include <uniterprotocol.h>

#include <stdexcept>
#include <utility>

namespace netbridge {
namespace {

void requireValue(const std::string& value, const char* description) {
    if (value.empty()) {
        throw std::invalid_argument(description);
    }
}

std::shared_ptr<sharedmodel::UniterMessage> makePublicMessage(sharedmodel::ProtocolAction action) {
    auto message = std::make_shared<sharedmodel::UniterMessage>();
    message->endpoint = sharedmodel::Endpoint::PUBLIC;
    message->subsystem = sharedmodel::Subsystem::PROTOCOL;
    message->crudact = sharedmodel::CrudAction::NOTCRUD;
    message->action = action;
    message->status = sharedmodel::MessageStatus::REQUEST;
    return message;
}

template <typename QueryT>
std::shared_ptr<QueryT> submitQuery(std::shared_ptr<QueryT> query) {
    return NetBridge::instance().query(query) ? query : nullptr;
}

} // namespace

std::shared_ptr<GetTokenQueryMessage> GetTokenQueryMessage::create(
    const std::string& login,
    const std::string& password) {
    requireValue(login, "GET_TOKEN requires a login");
    requireValue(password, "GET_TOKEN requires a password");
    auto message = makePublicMessage(sharedmodel::ProtocolAction::GET_TOKEN);
    message->add_data[sharedmodel::AddDataLogin] = login;
    message->add_data[sharedmodel::AddDataPassword] = password;
    return submitQuery(std::shared_ptr<GetTokenQueryMessage>{new GetTokenQueryMessage(std::move(message))});
}

GetTokenQueryMessage::GetTokenQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

std::shared_ptr<RefreshTokenQueryMessage> RefreshTokenQueryMessage::create(
    const std::string& refreshToken) {
    requireValue(refreshToken, "REFRESH_TOKEN requires a refresh token");
    auto message = makePublicMessage(sharedmodel::ProtocolAction::REFRESH_TOKEN);
    message->add_data[sharedmodel::AddDataRefreshToken] = refreshToken;
    return submitQuery(std::shared_ptr<RefreshTokenQueryMessage>{
        new RefreshTokenQueryMessage(std::move(message))});
}

RefreshTokenQueryMessage::RefreshTokenQueryMessage(
    std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

std::shared_ptr<CreateEmployeeQueryMessage> CreateEmployeeQueryMessage::create(
    const std::string& email,
    const std::string& name) {
    requireValue(email, "CREATE_EMPLOYEE requires an email");
    requireValue(name, "CREATE_EMPLOYEE requires a name");
    auto message = makePublicMessage(sharedmodel::ProtocolAction::CREATE_EMPLOYEE);
    message->add_data[sharedmodel::AddDataEmail] = email;
    message->add_data[sharedmodel::AddDataName] = name;
    return submitQuery(std::shared_ptr<CreateEmployeeQueryMessage>{
        new CreateEmployeeQueryMessage(std::move(message))});
}

CreateEmployeeQueryMessage::CreateEmployeeQueryMessage(
    std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

std::shared_ptr<RegisterCompanyQueryMessage> RegisterCompanyQueryMessage::create(
    const std::string& companyName,
    const std::string& adminLogin,
    const std::string& adminEmail,
    const std::string& adminPassword) {
    requireValue(companyName, "REGISTER_COMPANY requires a company name");
    requireValue(adminLogin, "REGISTER_COMPANY requires an admin login");
    requireValue(adminEmail, "REGISTER_COMPANY requires an admin email");
    requireValue(adminPassword, "REGISTER_COMPANY requires an admin password");
    auto message = makePublicMessage(sharedmodel::ProtocolAction::REGISTER_COMPANY);
    message->add_data[sharedmodel::AddDataCompanyName] = companyName;
    message->add_data[sharedmodel::AddDataAdminLogin] = adminLogin;
    message->add_data[sharedmodel::AddDataAdminEmail] = adminEmail;
    message->add_data[sharedmodel::AddDataAdminPassword] = adminPassword;
    return submitQuery(std::shared_ptr<RegisterCompanyQueryMessage>{
        new RegisterCompanyQueryMessage(std::move(message))});
}

RegisterCompanyQueryMessage::RegisterCompanyQueryMessage(
    std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

std::shared_ptr<GetUpdateQueryMessage> GetUpdateQueryMessage::create(const std::string& appVersion) {
    requireValue(appVersion, "GET_UPDATE requires an application version");
    auto message = makePublicMessage(sharedmodel::ProtocolAction::GET_UPDATE);
    message->add_data[sharedmodel::AddDataAppVersion] = appVersion;
    return submitQuery(std::shared_ptr<GetUpdateQueryMessage>{new GetUpdateQueryMessage(std::move(message))});
}

GetUpdateQueryMessage::GetUpdateQueryMessage(std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

std::shared_ptr<GetMigrationsQueryMessage> GetMigrationsQueryMessage::create() {
    auto message = makePublicMessage(sharedmodel::ProtocolAction::GET_MIGRATIONS);
    message->add_data[sharedmodel::AddDataDataModelVersion] = sharedmodel::DataModelVersion;
    return submitQuery(std::shared_ptr<GetMigrationsQueryMessage>{
        new GetMigrationsQueryMessage(std::move(message))});
}

GetMigrationsQueryMessage::GetMigrationsQueryMessage(
    std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

std::shared_ptr<PublicFileAccessQueryMessage> PublicFileAccessQueryMessage::create(
    const std::string& objectKey,
    PublicFileAccessMode mode) {
    requireValue(objectKey, "PUBLIC FILE_ACCESS requires an object key");
    auto message = makePublicMessage(sharedmodel::ProtocolAction::FILE_ACCESS);
    message->add_data[sharedmodel::AddDataObjectKey] = objectKey;
    message->add_data[sharedmodel::AddDataFileAccessMode] =
        mode == PublicFileAccessMode::READ
            ? sharedmodel::AddDataFileAccessRead
            : sharedmodel::AddDataFileAccessWrite;
    return submitQuery(std::shared_ptr<PublicFileAccessQueryMessage>{
        new PublicFileAccessQueryMessage(std::move(message))});
}

PublicFileAccessQueryMessage::PublicFileAccessQueryMessage(
    std::shared_ptr<sharedmodel::UniterMessage> message)
    : QueryMessage(std::move(message)) {}

} // namespace netbridge
