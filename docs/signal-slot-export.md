# Signal/Slot Export

`netbridge::NetBridge::instance()` owns request validation, sequence assignment,
token injection, dispatch, and response correlation.

## Session slots

- `setToken` replaces or clears the tenant session token and clears pending
  queries.
- `refreshToken` installs a non-empty replacement token while preserving pending
  queries.

## Endpoint transport pairs

| Endpoint | Outgoing signal | Incoming slot |
| --- | --- | --- |
| PUBLIC | `signalSendPublicMessage` | `onReceivePublicMessage` |
| TENANT | `signalSendTenantMessage` | `onReceiveTenantMessage` |
| CRUD | `signalSendCrudMessage` | `onReceiveCrudMessage` |

Each incoming slot accepts terminal `SUCCESS` or `ERROR` messages only for its
own endpoint. Matching messages are delivered through the pending
`QueryMessage::signalReceived` signal.

KAFKA and FILESTORAGE messages are not accepted by NetBridge.
