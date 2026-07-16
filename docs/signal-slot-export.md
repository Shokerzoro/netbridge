# Signal/Slot Export

## Main singleton

| Class | Accessor | Responsibility |
| --- | --- | --- |
| `tenantbridge::TenantMessager` | `tenantbridge::TenantMessager::instance()` | Owns tenant token state, outgoing validation, sequence assignment, dispatch, and response correlation. |

## Exported slots

| Slot | Input | Effect |
| --- | --- | --- |
| `setToken(const std::string& token)` | Initial/replacement session token, or empty to clear | Replaces the session token and clears pending queries. |
| `refreshToken(const std::string& token)` | Non-empty refreshed access token | Replaces the token while preserving pending queries; ignores empty input. |
| `onRecvUniterMessage(std::shared_ptr<sharedmodel::UniterMessage>)` | Incoming tenant/CRUD terminal response | Routes a matching response to its pending query. |

## Exported signal

| Signal | Payload | Expected receiver |
| --- | --- | --- |
| `signalSendMessage(std::shared_ptr<sharedmodel::UniterMessage>)` | Validated tenant or CRUD message with injected token | Tenant TLS connector or app-level protocol router. |

The application owns cross-module signal/slot wiring. Tenantbridge does not wire
itself directly to public, Kafka, or file-transfer connectors.
