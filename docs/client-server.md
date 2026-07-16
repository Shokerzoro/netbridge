# Tenant And CRUD Communication

The desktop uses tenantbridge only for the assigned tenant's direct TLS channel.
All outgoing messages are specialized `TENANT` queries or `CRUD` queries/events.

## Token lifecycle

The application supplies the initial tenant token through
`TenantMessager::setToken`. This starts a new session and clears pending requests
from the previous session. Passing an empty token clears the session.

`TenantMessager::refreshToken` replaces a non-empty token without clearing
pending requests, allowing in-flight responses during token overlap. An empty
refresh value is ignored. The messager overwrites `add_data[token]` immediately
before dispatch; messages are not emitted while no token is installed.

## Query flow

1. A caller creates a specialized query.
2. `TenantMessager` validates that it is a supported `TENANT` or `CRUD` request.
3. The active token and a new `sequence_id` are assigned.
4. `signalSendMessage` emits the request to the tenant connector.
5. A terminal `SUCCESS` or `ERROR` response must repeat the sequence ID,
   endpoint, and action/CRUD action.
6. Matching responses are stored on the query and emit its `signalReceived`.

Responses from public, Kafka, or file-storage endpoints and mismatched responses
are ignored.

## Transactional CRUD

`BeginTransactionQueryMessage` starts a transaction and the response supplies
its ID. CRUD queries or one-way CUD events may carry that ID. The transaction is
finished using `EndTransactionQueryMessage` with commit or rollback.

One-way CRUD events deliberately have no sequence ID. They are intended for
operations whose outcome is determined by the surrounding transaction. A READ
operation always uses `CrudQueryMessage` because it requires a response.
