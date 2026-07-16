# uniter-netbridge

`netbridge` owns construction, dispatch, and correlation for the three network
request domains used by the desktop client: `Endpoint::PUBLIC`,
`Endpoint::TENANT`, and `Endpoint::CRUD`.

`NetBridge` is the QObject singleton exported for application wiring. It assigns
monotonic sequence IDs, tracks pending queries, owns the current tenant access
token, and routes outgoing messages through endpoint-specific signals.

The upper-layer specialized API is split by endpoint:

- `publicmessages.h` covers authentication, company/employee registration,
  update and migration lookup, and public file-access requests.
- `tenantmessages.h` covers user authentication, Kafka credential lookup, full
  synchronization, transactions, and tenant file-access requests.
- `crudmessages.h` covers tracked CRUD requests and one-way CUD events.

The module does not support `Endpoint::KAFKA` or `Endpoint::FILESTORAGE`.
Kafka consumption and presigned-URL file transfer are owned by other modules.
