#include <gtest/gtest.h>

#include <specializedmessages.h>
#include <tenantmessager.h>

#include <common/resourceabstract.h>
#include <uniterprotocol.h>

#include <QObject>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace tenantbridge {
namespace {

class TestQuery final : public QueryMessage {
public:
    explicit TestQuery(std::shared_ptr<sharedmodel::UniterMessage> message)
        : QueryMessage(std::move(message)) {
    }
};

class TenantBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& messager = TenantMessager::instance();
        messager.setToken("session-token");
        connection_ = QObject::connect(
            &messager,
            &TenantMessager::signalSendMessage,
            [&] (std::shared_ptr<sharedmodel::UniterMessage> message) {
                emitted_ = std::move(message);
                ++emission_count_;
            });
    }

    void TearDown() override {
        QObject::disconnect(connection_);
        TenantMessager::instance().setToken({});
    }

    std::shared_ptr<sharedmodel::ResourceAbstract> employeeResource() const {
        return std::make_shared<sharedmodel::ResourceAbstract>(
            sharedmodel::Subsystem::MANAGER,
            std::nullopt,
            sharedmodel::Resource::EMPLOYEES);
    }

    QMetaObject::Connection connection_;
    std::shared_ptr<sharedmodel::UniterMessage> emitted_;
    int emission_count_{0};
};

void expectTenantRequest(
    const std::shared_ptr<QueryMessage>& query,
    sharedmodel::ProtocolAction action) {
    ASSERT_NE(query, nullptr);
    ASSERT_NE(query->message(), nullptr);
    EXPECT_EQ(query->message()->endpoint, sharedmodel::Endpoint::TENANT);
    EXPECT_EQ(query->message()->subsystem, sharedmodel::Subsystem::PROTOCOL);
    EXPECT_EQ(query->message()->crudact, sharedmodel::CrudAction::NOTCRUD);
    EXPECT_EQ(query->message()->action, action);
    EXPECT_EQ(query->message()->status, sharedmodel::MessageStatus::REQUEST);
    EXPECT_TRUE(query->message()->sequence_id.has_value());
    EXPECT_EQ(query->message()->add_data.at(sharedmodel::AddDataToken), "session-token");
}

TEST_F(TenantBridgeTest, CreatesAllSimpleTenantQueries) {
    auto getUser = GetUserQueryMessage::create();
    expectTenantRequest(getUser, sharedmodel::ProtocolAction::GET_USER);

    auto getKafka = GetKafkaQueryMessage::create(42);
    expectTenantRequest(getKafka, sharedmodel::ProtocolAction::GET_KAFKA);
    EXPECT_EQ(getKafka->message()->add_data.at(sharedmodel::AddDataKafkaOffset), "42");

    auto fullSync = FullSyncQueryMessage::create();
    expectTenantRequest(fullSync, sharedmodel::ProtocolAction::FULL_SYNC);

    auto begin = BeginTransactionQueryMessage::create();
    expectTenantRequest(begin, sharedmodel::ProtocolAction::BEGIN_TRANSACTION);
    EXPECT_EQ(begin->message()->add_data.count(sharedmodel::AddDataTransactionId), 0U);
}

TEST_F(TenantBridgeTest, CreatesEndTransactionAndFileAccessQueries) {
    auto commit = EndTransactionQueryMessage::create("txn-1", TransactionAction::COMMIT);
    expectTenantRequest(commit, sharedmodel::ProtocolAction::END_TRANSACTION);
    EXPECT_EQ(commit->message()->add_data.at(sharedmodel::AddDataTransactionId), "txn-1");
    EXPECT_EQ(commit->message()->add_data.at(sharedmodel::AddDataTransactionAction), "commit");

    auto rollback = EndTransactionQueryMessage::create("txn-2", TransactionAction::ROLLBACK);
    EXPECT_EQ(rollback->message()->add_data.at(sharedmodel::AddDataTransactionAction), "rollback");

    auto read = FileAccessQueryMessage::create("object", FileAccessMode::READ);
    expectTenantRequest(read, sharedmodel::ProtocolAction::FILE_ACCESS);
    EXPECT_EQ(read->message()->add_data.at(sharedmodel::AddDataObjectKey), "object");
    EXPECT_EQ(read->message()->add_data.at(sharedmodel::AddDataFileAccessMode), "read");

    auto write = FileAccessQueryMessage::create("object", FileAccessMode::WRITE);
    EXPECT_EQ(write->message()->add_data.at(sharedmodel::AddDataFileAccessMode), "write");
}

TEST_F(TenantBridgeTest, CreatesTrackedCrudQueries) {
    auto query = CrudQueryMessage::create(
        sharedmodel::CrudAction::READ,
        employeeResource(),
        "txn-1");

    ASSERT_NE(query, nullptr);
    const auto& message = query->message();
    EXPECT_EQ(message->endpoint, sharedmodel::Endpoint::CRUD);
    EXPECT_EQ(message->subsystem, sharedmodel::Subsystem::MANAGER);
    EXPECT_EQ(message->action, sharedmodel::ProtocolAction::NONE);
    EXPECT_EQ(message->crudact, sharedmodel::CrudAction::READ);
    EXPECT_EQ(message->status, sharedmodel::MessageStatus::REQUEST);
    EXPECT_TRUE(message->sequence_id.has_value());
    EXPECT_EQ(message->add_data.at(sharedmodel::AddDataTransactionId), "txn-1");
    EXPECT_EQ(message->add_data.at(sharedmodel::AddDataToken), "session-token");
}

TEST_F(TenantBridgeTest, CreatesOneWayCrudEvents) {
    auto event = CrudEventMessage::create(
        sharedmodel::CrudAction::CREATE,
        employeeResource(),
        "txn-1");

    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->message()->endpoint, sharedmodel::Endpoint::CRUD);
    EXPECT_EQ(event->message()->crudact, sharedmodel::CrudAction::CREATE);
    EXPECT_EQ(event->message()->status, sharedmodel::MessageStatus::NOTIFICATION);
    EXPECT_FALSE(event->message()->sequence_id.has_value());
    EXPECT_EQ(event->message()->add_data.at(sharedmodel::AddDataTransactionId), "txn-1");
    EXPECT_EQ(event->message()->add_data.at(sharedmodel::AddDataToken), "session-token");
}

TEST_F(TenantBridgeTest, RejectsInvalidCrudMessages) {
    EXPECT_THROW(
        CrudQueryMessage::create(sharedmodel::CrudAction::NOTCRUD, employeeResource()),
        std::invalid_argument);
    EXPECT_THROW(
        CrudQueryMessage::create(sharedmodel::CrudAction::CREATE, nullptr),
        std::invalid_argument);
    EXPECT_THROW(
        CrudEventMessage::create(sharedmodel::CrudAction::READ, employeeResource()),
        std::invalid_argument);

    auto local = std::make_shared<sharedmodel::ResourceAbstract>(
        sharedmodel::Subsystem::LOCAL,
        std::nullopt,
        sharedmodel::Resource::LOCAL_SETTINGS);
    EXPECT_THROW(
        CrudQueryMessage::create(sharedmodel::CrudAction::UPDATE, local),
        std::invalid_argument);
    EXPECT_THROW(
        EndTransactionQueryMessage::create({}, TransactionAction::COMMIT),
        std::invalid_argument);
    EXPECT_THROW(
        FileAccessQueryMessage::create({}, FileAccessMode::READ),
        std::invalid_argument);
}

TEST_F(TenantBridgeTest, RequiresTokenAndOverwritesExistingToken) {
    auto& messager = TenantMessager::instance();
    messager.setToken({});
    EXPECT_EQ(GetUserQueryMessage::create(), nullptr);
    EXPECT_EQ(emission_count_, 0);

    messager.setToken("authoritative-token");
    auto message = std::make_shared<sharedmodel::UniterMessage>();
    message->endpoint = sharedmodel::Endpoint::TENANT;
    message->action = sharedmodel::ProtocolAction::GET_USER;
    message->add_data[sharedmodel::AddDataToken] = "caller-token";
    auto query = std::make_shared<TestQuery>(message);

    EXPECT_TRUE(messager.query(query));
    EXPECT_EQ(message->add_data.at(sharedmodel::AddDataToken), "authoritative-token");
}

TEST_F(TenantBridgeTest, RejectsUnsupportedEndpoints) {
    auto message = std::make_shared<sharedmodel::UniterMessage>();
    message->endpoint = sharedmodel::Endpoint::PUBLIC;
    message->action = sharedmodel::ProtocolAction::GET_TOKEN;
    auto query = std::make_shared<TestQuery>(message);

    EXPECT_FALSE(TenantMessager::instance().query(query));
    EXPECT_FALSE(message->sequence_id.has_value());
    EXPECT_EQ(emission_count_, 0);
}

TEST_F(TenantBridgeTest, RefreshPreservesPendingQueriesAndSetClearsThem) {
    auto& messager = TenantMessager::instance();
    auto preserved = GetUserQueryMessage::create();
    ASSERT_NE(preserved, nullptr);
    auto preservedResponse = std::make_shared<sharedmodel::UniterMessage>();
    preservedResponse->endpoint = sharedmodel::Endpoint::TENANT;
    preservedResponse->action = sharedmodel::ProtocolAction::GET_USER;
    preservedResponse->status = sharedmodel::MessageStatus::SUCCESS;
    preservedResponse->sequence_id = preserved->message()->sequence_id;

    messager.refreshToken("refreshed-token");
    messager.onRecvUniterMessage(preservedResponse);
    EXPECT_EQ(preserved->response(), preservedResponse);

    auto cleared = GetUserQueryMessage::create();
    ASSERT_NE(cleared, nullptr);
    auto clearedResponse = std::make_shared<sharedmodel::UniterMessage>();
    clearedResponse->endpoint = sharedmodel::Endpoint::TENANT;
    clearedResponse->action = sharedmodel::ProtocolAction::GET_USER;
    clearedResponse->status = sharedmodel::MessageStatus::SUCCESS;
    clearedResponse->sequence_id = cleared->message()->sequence_id;

    messager.setToken("new-session-token");
    messager.onRecvUniterMessage(clearedResponse);
    EXPECT_EQ(cleared->response(), nullptr);
}

TEST_F(TenantBridgeTest, IgnoresMismatchedResponsesUntilCorrectResponseArrives) {
    auto query = GetUserQueryMessage::create();
    ASSERT_NE(query, nullptr);

    auto mismatch = std::make_shared<sharedmodel::UniterMessage>();
    mismatch->endpoint = sharedmodel::Endpoint::TENANT;
    mismatch->action = sharedmodel::ProtocolAction::FULL_SYNC;
    mismatch->status = sharedmodel::MessageStatus::SUCCESS;
    mismatch->sequence_id = query->message()->sequence_id;
    TenantMessager::instance().onRecvUniterMessage(mismatch);
    EXPECT_EQ(query->response(), nullptr);

    auto match = std::make_shared<sharedmodel::UniterMessage>();
    match->endpoint = sharedmodel::Endpoint::TENANT;
    match->action = sharedmodel::ProtocolAction::GET_USER;
    match->status = sharedmodel::MessageStatus::ERROR;
    match->error = sharedmodel::ErrorCode::TOKEN_EXPIRED;
    match->sequence_id = query->message()->sequence_id;
    TenantMessager::instance().onRecvUniterMessage(match);
    EXPECT_EQ(query->response(), match);
}

} // namespace
} // namespace tenantbridge
