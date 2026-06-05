# uniter-messaging

Messaging helpers for Uniter request creation, one-way events, and local direct-response tracking.

`MessageFactory` creates protocol and CRUD `UniterMessage` instances without assigning `sequence_id`. `MessageManager` is a QObject singleton that owns local monotonic `sequence_id` generation for `QueryMessage` and emits outgoing `UniterMessage` instances through `signalSendMessage`.

Use `QueryMessage` for direct request/response traffic. Use `EventMessage` for one-way messages that should be sent without response tracking.
