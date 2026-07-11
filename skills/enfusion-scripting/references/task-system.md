# The task system (SCR_TaskSystem / SCR_Task / SCR_TaskExecutor)

Use when working with tasks, task assignment, task visibility, or the SCR_TaskSystem world
system. The task system is a frequent source of "works in SP, breaks on dedicated" bugs
because tasks combine runtime spawning, RPC timing, streaming, and faction filters.

## The cast of characters

- SCR_TaskSystem - a World system (not an entity). Get it via
  `SCR_TaskSystem.GetInstance()` (it does `world.FindSystem(SCR_TaskSystem)`). Holds the
  static `s_aTasks` list and the `s_OnTaskCreated`/`s_OnTaskAdded`/`s_OnTaskRemoved` invokers.
  Methods: AssignTask, UnassignTask, CreateTask, MoveTask, SetTaskState, IsTaskVisibleFor,
  CanTaskBeAssignedTo.
- SCR_Task (and SCR_ExtendedTask, SCR_EditorTask, SCR_TriggerTask) - the task ENTITY. It has
  RplSave/RplLoad and broadcast RPCs (Rpc_AddTaskAssigneePlayer, Rpc_SetTaskState, etc.).
  Assignees are written into RplSave via WriteExecutor/ReadExecutor, so they are JIP-safe.
- SCR_TaskExecutor - a lightweight, non-entity handle (SCR_TaskExecutorPlayer / Entity /
  Group). `SCR_TaskExecutor.FromPlayerID(id)` makes one. It is created locally; it is NOT
  itself replicated, it just describes who to assign to.
- SCR_TaskSystemNetworkComponent - vanilla component on the PlayerController that handles
  client->server task requests. This is the template for any client-initiated task action.

## The two ways tasks come into existence

1. Pre-placed in the world (loadtime): the task entity is in the scene. Every client has a
   proxy from world load. EOnInit -> ConnectToTaskSystem runs on every client, so the task is
   in s_aTasks everywhere. This is the ROBUST option for any objective every player must see.
2. Runtime spawn (SpawnEntityPrefab) or SCR_TaskSystem.CreateTask: the task exists on the
   server and streams to clients by relevance. Far clients have no proxy => task not in their
   s_aTasks => invisible. Also subject to the unregistered-item RPC timing trap if you assign
   in the same frame.

Rule: an objective that must be visible to all players at all distances MUST be loadtime. If
you need randomization, pre-place one task per candidate location and activate/move only the
chosen one.

## The runtime-spawn trap (the classic bug)

```enforce
m_extractionTask = SCR_Task.Cast(ESC_Utils.SpawnEntityPrefab(taskPrefab, extractionPoint));
// ... same frame:
taskSystem.AssignTask(m_extractionTask, executor);   // "unregistered item" RPC error
taskSystem.SetTaskState(m_extractionTask, SCR_ETaskState.CREATED); // also dropped
```
Two problems:
1. The RPC goes through an entity not yet registered in the replication session
   (SpawnEntityPrefab returns before registration) => "unregistered item", dropped.
2. Even once registered, far clients don't stream the runtime task in => invisible there.

Fix (loadtime, preferred): pre-place a task at each extraction point. At StartEscape, pick the
chosen one and MoveTask/SetTaskState/AssignTask through the long-registered entity. No
unregistered-item error, every client has it, JIP-safe.

Fix (if you must runtime-spawn): defer the task operations off the spawn frame and accept that
far clients won't see it until they approach:
```enforce
m_extractionTask = SCR_Task.Cast(ESC_Utils.SpawnEntityPrefab(taskPrefab, startPos));
GetGame().GetCallqueue().CallLater(FinalizeTask, 200, false);
void FinalizeTask()
{
    taskSystem.MoveTask(m_extractionTask, m_extractionPoint.GetOrigin());
    foreach (ref ESC_Player p : ESC_Utils.GetPlayers())
        taskSystem.AssignTask(m_extractionTask, p.GetTaskExecutor());
}
```
This kills the unregistered-item error but does NOT solve far-visibility. For a global
objective, prefer loadtime.

## JIP-safe assignment

AssignTask -> AddTaskAssignee -> Rpc_AddTaskAssigneePlayer is a BROADCAST RPC on the task
entity. It reaches only clients that currently have the task streamed in. A client that
streams in later (or JIP) misses the broadcast - BUT assignees are also in RplSave
(WriteExecutor/ReadExecutor), so when a client streams the task in it reads the current
assignees from the snapshot. So assignees ARE JIP-safe as long as the task entity reaches the
client eventually.

The risk is only when you broadcast BEFORE the client has the entity AND the entity never
reaches the client (runtime + far). Loadtime tasks remove that risk entirely.

For fully client-driven, JIP-safe assignment (the vanilla pattern), have each client request
assignment when it sees the task created:
```enforce
// client-side, once on startup:
SCR_TaskSystem.GetOnTaskCreated().Insert(OnTaskCreated);
void OnTaskCreated(SCR_Task task)
{
    if (task.GetTaskID() != "extraction_move") return;
    m_NetComp.RequestAssignTask(task.GetTaskID()); // RPC via PlayerController component
}
// server-side handler:
[RplRpc(RplChannel.Reliable, RplRcver.Server)]
protected void Rpc_RequestAssignTask_S(string taskID)
{
    SCR_Task task = SCR_TaskSystem.GetTaskFromTaskID(taskID);
    if (!task) return;
    SCR_TaskSystem.GetInstance().AssignTask(task,
        SCR_TaskExecutor.FromPlayerID(m_PlayerController.GetPlayerId()), true);
}
```
Because the client fires this only after the task exists locally (streamed in), the resulting
broadcast lands on a client that already has the task. JIP clients handle themselves.

## Visibility and ownership filters (silent hiding)

SCR_Task has:
- m_eTaskOwnership (NONE/EXECUTOR/ASSIGNEES/GROUP/FACTION/EVERYONE) - who CAN be assigned.
- m_eTaskVisibility (NONE/EXECUTOR/ASSIGNEES/GROUP/FACTION/EVERYONE) - who CAN see it.
- m_aOwnerFactionKeys / m_aOwnerGroupIDs / m_aOwnerExecutors - the allowed sets.

SCR_TaskSystem.IsTaskVisibleFor / CanTaskBeAssignedTo check these against the executor's
faction key / group id. If they don't match, the task is HIDDEN or assignment is SILENTLY
rejected (AddTaskAssignee returns false, no broadcast).

Common gotcha: a task with `m_eTaskVisibility FACTION` and `m_aOwnerFactionKeys { "US" }` is
invisible to any executor whose faction is not "US" (or whose faction is not resolved yet on
the client). If players aren't in the US faction at the moment the task list refreshes, the
task vanishes even though the entity is present.

Debug: temporarily set `m_eTaskVisibility EVERYONE` and `m_eTaskOwnership EVERYONE`. If the
task then shows/assigns, it was the filter; ensure players are in the right faction/group
before/when the task appears.

## Useful runtime checks

Count tasks a client knows about (great for diagnosing "invisible task"):
```enforce
array<SCR_Task> tasks = {};
SCR_TaskSystem.GetInstance().GetTasks(tasks);
Print(string.Format("[CLIENT] task count = %1", tasks.Count()), LogLevel.ERROR);
```
- 0 on a remote client that should see the task => not streamed in (runtime + far) or missing
  RplComponent. Use loadtime for global objectives.
- >0 but still invisible => visibility filter (faction/group/executor) mismatch.

## When to use each approach

| Need | Approach |
| --- | --- |
| Global objective every player must always see | Pre-place the task (loadtime); activate/move the chosen one. |
| Randomized objective among N locations | Pre-place one task per location; activate only the chosen one. |
| Task created near players, short-lived | Runtime spawn + CallLater for assign/state; accept streaming. |
| Client-initiated assignment | PlayerController network component + GetOnTaskCreated hook. |
| Task hidden from wrong players | Check m_eTaskVisibility/m_eTaskOwnership + owner faction/group sets. |
