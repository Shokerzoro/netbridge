# CrudHandler

`netbridge::CrudHandler` stages explicit resource mutations and submits them
as one tenant transaction through the surrounding `netbridge` module.

## Usage

```cpp
auto employee = std::make_shared<sharedmodel::Employee>(); // UUIDv7 generated
employee->login = "engineer";

auto assignment = std::make_shared<sharedmodel::EmployeeAssignment>();
employee->assignments.push_back(
    sharedmodel::EmployeeAssignmentUuid{assignment->id});

auto* transaction = new netbridge::CrudHandler(parent);
connect(transaction, &netbridge::CrudHandler::signalFinished,
        parent, [transaction](bool ok, sharedmodel::ErrorCode error,
                              const QString& text) {
            // Inspect ok/error/text and dispose or retain the handler as needed.
            transaction->deleteLater();
        });

transaction->add(employee);
transaction->add(assignment);
transaction->commit();
```

`add()`, `update()`, and `remove()` take a `ResourceAbstract` pointer, validate
that its concrete type is registered and its identity is UUIDv7, and clone the
resource immediately. Later caller changes do not alter the staged request.

## Transaction behavior

`commit()` performs these asynchronous steps:

1. submit `BEGIN_TRANSACTION` and accept the transaction ID created by the
   tenant server;
2. submit one CRUD message per staged resource, in staging order, with that ID;
3. submit `END_TRANSACTION/commit` after every mutation succeeds;
4. submit `END_TRANSACTION/rollback` after a CRUD or commit failure.

The handler is one-shot. `signalFinished(success, error, errorText)` is emitted
exactly once for a started operation. `state()` exposes progress and
`setRequestTimeout()` adjusts the per-request timeout while the handler is still
in `Draft` state.

Repeated operations for the same resource are coalesced: create followed by
update remains create with the newest snapshot; update followed by delete
becomes delete; create followed by delete removes the staged operation.
