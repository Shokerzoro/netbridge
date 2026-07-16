# NetBridge Communication

## Public requests

Public requests require no tenant session token. Their specialized factories
populate the exact `add_data` fields from the sharedmodel protocol contract and
emit through `signalSendPublicMessage`.

## Tenant and CRUD requests

The application installs the tenant access token through `NetBridge::setToken`.
NetBridge overwrites `add_data[token]` immediately before dispatching every
TENANT or CRUD message. Such messages are rejected while no token is installed.

`refreshToken` changes a non-empty token without cancelling in-flight requests.

## Query correlation

1. A specialized factory creates and submits a request.
2. NetBridge validates the endpoint-specific shape and assigns a sequence ID.
3. The matching PUBLIC, TENANT, or CRUD send signal emits the message.
4. The corresponding receive slot accepts a terminal response with the same
   sequence ID, endpoint, and action or CRUD action.
5. The response is stored on the query and emitted through its `signalReceived`.

CRUD create/update/delete events remain one-way notifications without sequence
IDs. Transactional CRUD uses TENANT begin/end transaction requests and optional
CRUD `transaction_id` metadata.

`Endpoint::KAFKA` and `Endpoint::FILESTORAGE` are outside this module.
