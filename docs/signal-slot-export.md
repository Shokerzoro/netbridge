# Signal/Slot Export

## Purpose

This document lists the Qt signal/slot surface exported by `serverbrige` for app-level soft wiring.

`app` owns cross-module connections. Module singleton classes expose signals and slots, but must not directly wire unrelated application components together.

## Main Singleton

| Class | Accessor | Responsibility |
| --- | --- | --- |
| `serverbrige::MessageManager` | `serverbrige::MessageManager::instance()` | Owns outgoing protocol message dispatch, query `sequence_id` assignment, and routing incoming responses back to pending `QueryMessage` objects. |

## Exported Slots

| Slot | Input | Expected sender | Effect |
| --- | --- | --- | --- |
| `onRecvUniterMessage(std::shared_ptr<sharedmodel::UniterMessage> message)` | Incoming `UniterMessage` from server transport/connector. | Server connector or app-level protocol router. | Ignores messages without `sequence_id`; resolves the matching pending query; stores the response on the `QueryMessage`; emits the query object's receive notification. |

## Exported Signals

| Signal | Payload | Expected receivers | Meaning |
| --- | --- | --- | --- |
| `signalSendMessage(std::shared_ptr<sharedmodel::UniterMessage> message)` | Outgoing `UniterMessage`. | Server connector send slot or app-level protocol router. | Emitted when `MessageManager` sends an event or query request outward. |

## App Wiring Notes

| Direction | Connection |
| --- | --- |
| Connector/app router to serverbrige | Connect incoming server messages to `MessageManager::onRecvUniterMessage`. |
| serverbrige to connector/app router | Connect `MessageManager::signalSendMessage` to the connector send slot. |

Higher-level modules normally create specialized `QueryMessage` / `EventMessage` wrappers instead of calling connector APIs directly. The app still owns the final runtime wiring between serverbrige and the actual connector component.
