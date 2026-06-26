# Client And Server Communication

The client and server maintain two stable communication channels:

- A direct SSL/TLS connection for personal request/response traffic.
- A Kafka subscription for broadcast notifications.

The direct SSL/TLS channel carries protocol requests and CUD requests from a single client. The client sends each direct request with a `sequence_id`, and the server replies on the same direct channel with the same `sequence_id`. The `sequence_id` is assigned by `serverbrige::MessageManager` for `serverbrige::QueryMessage` subclasses.

Kafka is the broadcast notification transport. After the server successfully applies a CUD request, it publishes a CUD notification through Kafka so subscribed clients can keep their local databases consistent. Kafka notifications may omit `sequence_id` unless they are causally tied to a direct request.

Files are stored in MinIO. Clients access MinIO only through server-provided presigned URLs, requested over the direct SSL/TLS channel.

The intended tracked send flow is:

1. Caller creates a specialized `QueryMessage`.
2. `MessageManager` assigns a monotonic local `sequence_id`.
3. `MessageManager` emits the request over `signalSendMessage`.
4. Server replies directly with the same `sequence_id`.
5. `MessageManager` routes the response to the matching `QueryMessage`.
6. If a CUD operation succeeded, the server broadcasts a Kafka notification.

Business logic can use `serverbrige::CrudQueryMessage` for tracked CREATE/UPDATE/DELETE requests and `serverbrige::CrudEventMessage` for one-way CRUD notifications. One-way messages use `serverbrige::EventMessage` subclasses and are emitted through `MessageManager::signalSendMessage` without local `sequence_id` tracking.
