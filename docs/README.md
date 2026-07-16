# uniter-tenantbridge

`tenantbridge` owns construction and correlation of direct tenant TLS messages
for `Endpoint::TENANT` and `Endpoint::CRUD`. It does not create public web,
Kafka-consumer, or local file-storage messages.

`TenantMessager` is the QObject singleton exported for app-level wiring. It owns
the current tenant access token, assigns local monotonic `sequence_id` values,
injects the token into every outgoing message, emits messages through
`signalSendMessage`, and routes matching success/error responses back to pending
queries.

The specialized API covers every tenant action:

- `GetUserQueryMessage`
- `GetKafkaQueryMessage`
- `FullSyncQueryMessage`
- `BeginTransactionQueryMessage`
- `EndTransactionQueryMessage`
- `FileAccessQueryMessage`

`CrudQueryMessage` provides tracked CREATE/READ/UPDATE/DELETE requests.
`CrudEventMessage` provides one-way CREATE/UPDATE/DELETE operations, including
optional transaction association. READ always requires a query.

Public authentication/update requests belong to a future public bridge.
Presigned URL GET/PUT operations belong to `filetransfer`. Kafka notifications
are received through the Kafka connector and are not emitted by tenantbridge.
