# uniter-messaging

Messaging helpers for Uniter request creation and local direct-response tracking.

`MessageFactory` creates protocol and CRUD `UniterMessage` instances without assigning `sequence_id`. `MessageManager` owns local monotonic `sequence_id` generation and stores observers for direct request/response completion tracking.
