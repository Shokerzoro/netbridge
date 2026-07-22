#include <gtest/gtest.h>

#include <crudhandler.h>
#include <netbridge.h>

#include <manager/employee.h>

#include <QObject>

#include <memory>
#include <string>
#include <vector>

namespace {

std::shared_ptr<sharedmodel::UniterMessage> responseFor(
    const std::shared_ptr<sharedmodel::UniterMessage>& request,
    sharedmodel::MessageStatus status,
    sharedmodel::ErrorCode error = sharedmodel::ErrorCode::SUCCESS)
{
    auto response = std::make_shared<sharedmodel::UniterMessage>();
    response->endpoint = request->endpoint;
    response->subsystem = request->subsystem;
    response->crudact = request->crudact;
    response->publicact = request->publicact;
    response->tenantact = request->tenantact;
    response->filestorageact = request->filestorageact;
    response->status = status;
    response->error = error;
    response->sequence_id = request->sequence_id;
    return response;
}

class CrudHandlerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto& bridge = netbridge::NetBridge::instance();
        bridge.setToken("crud-handler-test-token");
        tenant_connection = QObject::connect(
            &bridge, &netbridge::NetBridge::signalSendTenantMessage,
            [&](const std::shared_ptr<sharedmodel::UniterMessage>& message) {
                tenant_messages.push_back(message);
            });
        crud_connection = QObject::connect(
            &bridge, &netbridge::NetBridge::signalSendCrudMessage,
            [&](const std::shared_ptr<sharedmodel::UniterMessage>& message) {
                crud_messages.push_back(message);
            });
    }

    void TearDown() override
    {
        QObject::disconnect(tenant_connection);
        QObject::disconnect(crud_connection);
        netbridge::NetBridge::instance().setToken({});
    }

    void acceptBegin()
    {
        ASSERT_FALSE(tenant_messages.empty());
        auto response = responseFor(tenant_messages.back(), sharedmodel::MessageStatus::SUCCESS);
        response->add_data[sharedmodel::AddDataTransactionId] = "transaction-uuid-test";
        netbridge::NetBridge::instance().onReceiveTenantMessage(response);
    }

    void acceptCrud(std::size_t index)
    {
        ASSERT_GT(crud_messages.size(), index);
        netbridge::NetBridge::instance().onReceiveCrudMessage(
            responseFor(crud_messages[index], sharedmodel::MessageStatus::SUCCESS));
    }

    QMetaObject::Connection tenant_connection;
    QMetaObject::Connection crud_connection;
    std::vector<std::shared_ptr<sharedmodel::UniterMessage>> tenant_messages;
    std::vector<std::shared_ptr<sharedmodel::UniterMessage>> crud_messages;
};

TEST_F(CrudHandlerTest, CommitsSeveralExplicitResourcesInOneServerTransaction)
{
    netbridge::CrudHandler handler;
    auto first = std::make_shared<sharedmodel::Employee>();
    auto second = std::make_shared<sharedmodel::Employee>();

    ASSERT_TRUE(handler.add(first));
    ASSERT_TRUE(handler.update(second));

    bool finished = false;
    bool success = false;
    QObject::connect(&handler, &netbridge::CrudHandler::signalFinished,
                     [&](bool value, sharedmodel::ErrorCode, const QString&) {
                         finished = true;
                         success = value;
                     });

    ASSERT_TRUE(handler.commit());
    ASSERT_EQ(tenant_messages.size(), 1U);
    EXPECT_EQ(tenant_messages.front()->tenantact,
              sharedmodel::TenantAction::BEGIN_TRANSACTION);

    acceptBegin();
    ASSERT_EQ(crud_messages.size(), 1U);
    EXPECT_EQ(crud_messages[0]->crudact, sharedmodel::CrudAction::CREATE);
    EXPECT_EQ(crud_messages[0]->resource->id, first->id);
    EXPECT_EQ(crud_messages[0]->add_data.at(sharedmodel::AddDataTransactionId),
              "transaction-uuid-test");

    acceptCrud(0);
    ASSERT_EQ(crud_messages.size(), 2U);
    EXPECT_EQ(crud_messages[1]->crudact, sharedmodel::CrudAction::UPDATE);
    EXPECT_EQ(crud_messages[1]->resource->id, second->id);

    acceptCrud(1);
    ASSERT_EQ(tenant_messages.size(), 2U);
    EXPECT_EQ(tenant_messages.back()->tenantact,
              sharedmodel::TenantAction::END_TRANSACTION);
    EXPECT_EQ(tenant_messages.back()->add_data.at(sharedmodel::AddDataTransactionAction),
              sharedmodel::AddDataTransactionCommit);

    netbridge::NetBridge::instance().onReceiveTenantMessage(
        responseFor(tenant_messages.back(), sharedmodel::MessageStatus::SUCCESS));
    EXPECT_TRUE(finished);
    EXPECT_TRUE(success);
    EXPECT_EQ(handler.state(), netbridge::CrudHandler::State::Finished);
}

TEST_F(CrudHandlerTest, CrudFailureRequestsRollbackAndReportsOriginalError)
{
    netbridge::CrudHandler handler;
    ASSERT_TRUE(handler.remove(std::make_shared<sharedmodel::Employee>()));

    bool finished = false;
    sharedmodel::ErrorCode result = sharedmodel::ErrorCode::SUCCESS;
    QObject::connect(&handler, &netbridge::CrudHandler::signalFinished,
                     [&](bool, sharedmodel::ErrorCode error, const QString&) {
                         finished = true;
                         result = error;
                     });

    ASSERT_TRUE(handler.commit());
    acceptBegin();
    ASSERT_EQ(crud_messages.size(), 1U);

    auto failure = responseFor(
        crud_messages.front(), sharedmodel::MessageStatus::ERROR,
        sharedmodel::ErrorCode::PERMISSION_DENIED);
    failure->add_data[sharedmodel::AddDataErrorText] = "denied";
    netbridge::NetBridge::instance().onReceiveCrudMessage(failure);

    ASSERT_EQ(tenant_messages.size(), 2U);
    EXPECT_EQ(tenant_messages.back()->add_data.at(sharedmodel::AddDataTransactionAction),
              sharedmodel::AddDataTransactionRollback);
    netbridge::NetBridge::instance().onReceiveTenantMessage(
        responseFor(tenant_messages.back(), sharedmodel::MessageStatus::SUCCESS));

    EXPECT_TRUE(finished);
    EXPECT_EQ(result, sharedmodel::ErrorCode::PERMISSION_DENIED);
}

TEST(CrudHandlerSynchronousTransport, DoesNotLoseImmediateResponses)
{
    auto& bridge = netbridge::NetBridge::instance();
    bridge.setToken("crud-handler-sync-token");
    QObject context;
    QObject::connect(&bridge, &netbridge::NetBridge::signalSendTenantMessage,
                     &context, [&bridge](const auto& request) {
        auto response = responseFor(request, sharedmodel::MessageStatus::SUCCESS);
        if (request->tenantact == sharedmodel::TenantAction::BEGIN_TRANSACTION) {
            response->add_data[sharedmodel::AddDataTransactionId] = "sync-transaction";
        }
        bridge.onReceiveTenantMessage(response);
    });
    QObject::connect(&bridge, &netbridge::NetBridge::signalSendCrudMessage,
                     &context, [&bridge](const auto& request) {
        bridge.onReceiveCrudMessage(
            responseFor(request, sharedmodel::MessageStatus::SUCCESS));
    });

    netbridge::CrudHandler handler;
    ASSERT_TRUE(handler.add(std::make_shared<sharedmodel::Employee>()));
    bool finished = false;
    QObject::connect(&handler, &netbridge::CrudHandler::signalFinished,
                     [&](bool success, sharedmodel::ErrorCode, const QString&) {
        finished = success;
    });

    EXPECT_TRUE(handler.commit());
    EXPECT_TRUE(finished);
    EXPECT_EQ(handler.state(), netbridge::CrudHandler::State::Finished);
    bridge.setToken({});
}

TEST(CrudHandlerStaging, CoalescesCreationUpdateAndDeletion)
{
    netbridge::CrudHandler handler;
    auto resource = std::make_shared<sharedmodel::Employee>();
    ASSERT_TRUE(handler.add(resource));
    resource->login = "latest";
    ASSERT_TRUE(handler.update(resource));
    ASSERT_TRUE(handler.remove(resource));
    EXPECT_TRUE(handler.commit());
    EXPECT_EQ(handler.state(), netbridge::CrudHandler::State::Finished);
}

} // namespace
