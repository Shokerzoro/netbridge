#include <gtest/gtest.h>

#include <crudmessages.h>
#include <netbridge.h>
#include <publicmessages.h>
#include <tenantmessages.h>

#include <common/resourceabstract.h>
#include <uniterprotocol.h>

#include <QObject>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace netbridge {
namespace {

class TestQuery final : public QueryMessage {
public:
    explicit TestQuery(std::shared_ptr<sharedmodel::UniterMessage> message)
        : QueryMessage(std::move(message)) {}
};

class NetBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& bridge = NetBridge::instance();
        bridge.setToken("session-token");
        public_connection_ = QObject::connect(
            &bridge,
            &NetBridge::signalSendPublicMessage,
            [&] (std::shared_ptr<sharedmodel::UniterMessage> message) {
                public_message_ = std::move(message);
                ++public_count_;
            });
        tenant_connection_ = QObject::connect(
            &bridge,
            &NetBridge::signalSendTenantMessage,
            [&] (std::shared_ptr<sharedmodel::UniterMessage> message) {
                tenant_message_ = std::move(message);
                ++tenant_count_;
            });
        crud_connection_ = QObject::connect(
            &bridge,
            &NetBridge::signalSendCrudMessage,
            [&] (std::shared_ptr<sharedmodel::UniterMessage> message) {
                crud_message_ = std::move(message);
                ++crud_count_;
            });
    }

    void TearDown() override {
        QObject::disconnect(public_connection_);
        QObject::disconnect(tenant_connection_);
        QObject::disconnect(crud_connection_);
        NetBridge::instance().setToken({});
    }

    std::shared_ptr<sharedmodel::ResourceAbstract> employeeResource() const {
        return std::make_shared<sharedmodel::ResourceAbstract>(
            sharedmodel::Subsystem::MANAGER,
            std::nullopt,
            sharedmodel::Resource::EMPLOYEES);
    }

    QMetaObject::Connection public_connection_;
    QMetaObject::Connection tenant_connection_;
    QMetaObject::Connection crud_connection_;
    std::shared_ptr<sharedmodel::UniterMessage> public_message_;
    std::shared_ptr<sharedmodel::UniterMessage> tenant_message_;
    std::shared_ptr<sharedmodel::UniterMessage> crud_message_;
    int public_count_{0};
    int tenant_count_{0};
    int crud_count_{0};
};

void expectProtocolRequest(
    const std::shared_ptr<QueryMessage>& query,
    sharedmodel::Endpoint endpoint,
    sharedmodel::ProtocolAction action) {
    ASSERT_NE(query, nullptr);
    ASSERT_NE(query->message(), nullptr);
    EXPECT_EQ(query->message()->endpoint, endpoint);
    EXPECT_EQ(query->message()->subsystem, sharedmodel::Subsystem::PROTOCOL);
    EXPECT_EQ(query->message()->crudact, sharedmodel::CrudAction::NOTCRUD);
    EXPECT_EQ(query->message()->action, action);
    EXPECT_EQ(query->message()->status, sharedmodel::MessageStatus::REQUEST);
    EXPECT_TRUE(query->message()->sequence_id.has_value());
}

TEST_F(NetBridgeTest, CreatesEveryPublicAuthenticationAndAccountRequest) {
    auto login = GetTokenQueryMessage::create("user", "secret");
    expectProtocolRequest(login, sharedmodel::Endpoint::PUBLIC, sharedmodel::ProtocolAction::GET_TOKEN);
    EXPECT_EQ(login->message()->add_data.at(sharedmodel::AddDataLogin), "user");
    EXPECT_EQ(login->message()->add_data.at(sharedmodel::AddDataPassword), "secret");

    auto refresh = RefreshTokenQueryMessage::create("refresh-token");
    expectProtocolRequest(refresh, sharedmodel::Endpoint::PUBLIC, sharedmodel::ProtocolAction::REFRESH_TOKEN);
    EXPECT_EQ(refresh->message()->add_data.at(sharedmodel::AddDataRefreshToken), "refresh-token");

    auto employee = CreateEmployeeQueryMessage::create("user@example.com", "User");
    expectProtocolRequest(employee, sharedmodel::Endpoint::PUBLIC, sharedmodel::ProtocolAction::CREATE_EMPLOYEE);
    EXPECT_EQ(employee->message()->add_data.at(sharedmodel::AddDataEmail), "user@example.com");
    EXPECT_EQ(employee->message()->add_data.at(sharedmodel::AddDataName), "User");

    auto company = RegisterCompanyQueryMessage::create(
        "Company", "admin", "admin@example.com", "admin-secret");
    expectProtocolRequest(company, sharedmodel::Endpoint::PUBLIC, sharedmodel::ProtocolAction::REGISTER_COMPANY);
    EXPECT_EQ(company->message()->add_data.at(sharedmodel::AddDataCompanyName), "Company");
    EXPECT_EQ(company->message()->add_data.at(sharedmodel::AddDataAdminLogin), "admin");
    EXPECT_EQ(company->message()->add_data.at(sharedmodel::AddDataAdminEmail), "admin@example.com");
    EXPECT_EQ(company->message()->add_data.at(sharedmodel::AddDataAdminPassword), "admin-secret");

    EXPECT_EQ(public_count_, 4);
    EXPECT_EQ(tenant_count_, 0);
    EXPECT_EQ(crud_count_, 0);
}

TEST_F(NetBridgeTest, CreatesEveryPublicUpdateAndFileAccessRequest) {
    auto update = GetUpdateQueryMessage::create("1.2.3");
    expectProtocolRequest(update, sharedmodel::Endpoint::PUBLIC, sharedmodel::ProtocolAction::GET_UPDATE);
    EXPECT_EQ(update->message()->add_data.at(sharedmodel::AddDataAppVersion), "1.2.3");

    auto migrations = GetMigrationsQueryMessage::create();
    expectProtocolRequest(
        migrations, sharedmodel::Endpoint::PUBLIC, sharedmodel::ProtocolAction::GET_MIGRATIONS);
    EXPECT_EQ(
        migrations->message()->add_data.at(sharedmodel::AddDataDataModelVersion),
        sharedmodel::DataModelVersion);

    auto file = PublicFileAccessQueryMessage::create("manifest.xml", PublicFileAccessMode::READ);
    expectProtocolRequest(file, sharedmodel::Endpoint::PUBLIC, sharedmodel::ProtocolAction::FILE_ACCESS);
    EXPECT_EQ(file->message()->add_data.at(sharedmodel::AddDataObjectKey), "manifest.xml");
    EXPECT_EQ(file->message()->add_data.at(sharedmodel::AddDataFileAccessMode), "read");
    EXPECT_EQ(file->message()->add_data.count(sharedmodel::AddDataToken), 0U);
}

TEST_F(NetBridgeTest, CreatesEveryTenantRequestAndInjectsToken) {
    auto getUser = GetUserQueryMessage::create();
    expectProtocolRequest(getUser, sharedmodel::Endpoint::TENANT, sharedmodel::ProtocolAction::GET_USER);

    auto getKafka = GetKafkaQueryMessage::create(42);
    EXPECT_EQ(getKafka->message()->add_data.at(sharedmodel::AddDataKafkaOffset), "42");

    auto fullSync = FullSyncQueryMessage::create();
    expectProtocolRequest(fullSync, sharedmodel::Endpoint::TENANT, sharedmodel::ProtocolAction::FULL_SYNC);

    auto begin = BeginTransactionQueryMessage::create();
    expectProtocolRequest(
        begin, sharedmodel::Endpoint::TENANT, sharedmodel::ProtocolAction::BEGIN_TRANSACTION);

    auto end = EndTransactionQueryMessage::create("txn-1", TransactionAction::COMMIT);
    EXPECT_EQ(end->message()->add_data.at(sharedmodel::AddDataTransactionId), "txn-1");
    EXPECT_EQ(end->message()->add_data.at(sharedmodel::AddDataTransactionAction), "commit");

    auto file = TenantFileAccessQueryMessage::create("snapshot", TenantFileAccessMode::WRITE);
    EXPECT_EQ(file->message()->add_data.at(sharedmodel::AddDataObjectKey), "snapshot");
    EXPECT_EQ(file->message()->add_data.at(sharedmodel::AddDataFileAccessMode), "write");

    EXPECT_EQ(tenant_count_, 6);
    EXPECT_EQ(public_count_, 0);
    EXPECT_EQ(crud_count_, 0);
    EXPECT_EQ(file->message()->add_data.at(sharedmodel::AddDataToken), "session-token");
}

TEST_F(NetBridgeTest, CreatesCrudQueriesAndOneWayEvents) {
    auto query = CrudQueryMessage::create(
        sharedmodel::CrudAction::READ, employeeResource(), "txn-1");
    ASSERT_NE(query, nullptr);
    EXPECT_EQ(query->message()->endpoint, sharedmodel::Endpoint::CRUD);
    EXPECT_EQ(query->message()->action, sharedmodel::ProtocolAction::NONE);
    EXPECT_EQ(query->message()->crudact, sharedmodel::CrudAction::READ);
    EXPECT_EQ(query->message()->status, sharedmodel::MessageStatus::REQUEST);
    EXPECT_EQ(query->message()->add_data.at(sharedmodel::AddDataToken), "session-token");

    auto event = CrudEventMessage::create(
        sharedmodel::CrudAction::CREATE, employeeResource(), "txn-1");
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->message()->status, sharedmodel::MessageStatus::NOTIFICATION);
    EXPECT_FALSE(event->message()->sequence_id.has_value());
    EXPECT_EQ(crud_count_, 2);
    EXPECT_EQ(public_count_, 0);
    EXPECT_EQ(tenant_count_, 0);
}

TEST_F(NetBridgeTest, PublicRequestsWorkWithoutTenantToken) {
    NetBridge::instance().setToken({});
    auto query = GetTokenQueryMessage::create("user", "secret");
    ASSERT_NE(query, nullptr);
    EXPECT_EQ(query->message()->add_data.count(sharedmodel::AddDataToken), 0U);
    EXPECT_EQ(GetUserQueryMessage::create(), nullptr);
}

TEST_F(NetBridgeTest, RefreshTokenPreservesPendingQueryAndSetTokenClearsIt) {
    auto preserved = GetUserQueryMessage::create();
    auto response = std::make_shared<sharedmodel::UniterMessage>();
    response->endpoint = sharedmodel::Endpoint::TENANT;
    response->action = sharedmodel::ProtocolAction::GET_USER;
    response->status = sharedmodel::MessageStatus::SUCCESS;
    response->sequence_id = preserved->message()->sequence_id;
    NetBridge::instance().refreshToken("refreshed-token");
    NetBridge::instance().onReceiveTenantMessage(response);
    EXPECT_EQ(preserved->response(), response);

    auto cleared = GetUserQueryMessage::create();
    response = std::make_shared<sharedmodel::UniterMessage>();
    response->endpoint = sharedmodel::Endpoint::TENANT;
    response->action = sharedmodel::ProtocolAction::GET_USER;
    response->status = sharedmodel::MessageStatus::SUCCESS;
    response->sequence_id = cleared->message()->sequence_id;
    NetBridge::instance().setToken("new-session-token");
    NetBridge::instance().onReceiveTenantMessage(response);
    EXPECT_EQ(cleared->response(), nullptr);
}

TEST_F(NetBridgeTest, ReceiveSlotsRequireTheirOwnEndpointAndMatchingAction) {
    auto query = GetUpdateQueryMessage::create("1.0.0");
    auto response = std::make_shared<sharedmodel::UniterMessage>();
    response->endpoint = sharedmodel::Endpoint::PUBLIC;
    response->action = sharedmodel::ProtocolAction::GET_UPDATE;
    response->status = sharedmodel::MessageStatus::SUCCESS;
    response->sequence_id = query->message()->sequence_id;

    NetBridge::instance().onReceiveTenantMessage(response);
    EXPECT_EQ(query->response(), nullptr);

    auto mismatch = std::make_shared<sharedmodel::UniterMessage>(*response);
    mismatch->action = sharedmodel::ProtocolAction::GET_TOKEN;
    NetBridge::instance().onReceivePublicMessage(mismatch);
    EXPECT_EQ(query->response(), nullptr);

    NetBridge::instance().onReceivePublicMessage(response);
    EXPECT_EQ(query->response(), response);
}

TEST_F(NetBridgeTest, RejectsKafkaFileStorageNoneAndMalformedRequests) {
    for (const auto endpoint : {
             sharedmodel::Endpoint::NONE,
             sharedmodel::Endpoint::KAFKA,
             sharedmodel::Endpoint::FILESTORAGE}) {
        auto message = std::make_shared<sharedmodel::UniterMessage>();
        message->endpoint = endpoint;
        auto query = std::make_shared<TestQuery>(message);
        EXPECT_FALSE(NetBridge::instance().query(query));
        EXPECT_FALSE(message->sequence_id.has_value());
    }

    auto malformed = std::make_shared<sharedmodel::UniterMessage>();
    malformed->endpoint = sharedmodel::Endpoint::PUBLIC;
    malformed->action = sharedmodel::ProtocolAction::GET_TOKEN;
    EXPECT_FALSE(NetBridge::instance().query(std::make_shared<TestQuery>(malformed)));
    EXPECT_EQ(public_count_, 0);
    EXPECT_EQ(tenant_count_, 0);
    EXPECT_EQ(crud_count_, 0);
}

TEST_F(NetBridgeTest, SpecializedFactoriesRejectInvalidArguments) {
    EXPECT_THROW(GetTokenQueryMessage::create({}, "password"), std::invalid_argument);
    EXPECT_THROW(RefreshTokenQueryMessage::create({}), std::invalid_argument);
    EXPECT_THROW(CreateEmployeeQueryMessage::create({}, "name"), std::invalid_argument);
    EXPECT_THROW(GetUpdateQueryMessage::create({}), std::invalid_argument);
    EXPECT_THROW(
        PublicFileAccessQueryMessage::create({}, PublicFileAccessMode::READ),
        std::invalid_argument);
    EXPECT_THROW(
        TenantFileAccessQueryMessage::create({}, TenantFileAccessMode::READ),
        std::invalid_argument);
    EXPECT_THROW(
        EndTransactionQueryMessage::create({}, TransactionAction::COMMIT),
        std::invalid_argument);
    EXPECT_THROW(
        CrudQueryMessage::create(sharedmodel::CrudAction::NOTCRUD, employeeResource()),
        std::invalid_argument);
    EXPECT_THROW(
        CrudEventMessage::create(sharedmodel::CrudAction::READ, employeeResource()),
        std::invalid_argument);
}

} // namespace
} // namespace netbridge
