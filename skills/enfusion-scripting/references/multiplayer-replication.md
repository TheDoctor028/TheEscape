# Multiplayer and replication deep dive

This is the reference for any "works in SP / self-hosted, breaks on dedicated" bug and any
Rpc/RplProp/RplSave/streaming question. Read it when the diagnostic checklist in SKILL.md
points at replication.

## Roles and the network model

- One server (the authority). N clients (proxies). Clients never talk to each other; the
  server redistributes.
- Single player == a player-hosted (listen) server. The local machine is authority AND owner
  of basically everything. This is why SP/listen hide replication bugs.
- A dedicated server is authority but NOT a player, and owns no player-controlled entities.

Entity roles:
- Authority: the reference entity. Set in stone; never moves.
- Proxy: a client's local representation of an authority. Receives state; cannot send state
  about it (unless it is also the owner).
- Owner: a special proxy with extra rights (owner-only RPCs, prediction). Only one machine
  owns an entity; ownership can transfer; authority cannot.

## The RPC routing table (cite this)

From the BI wiki Replication group page. This is THE answer to most "RPC works for host, not
joiners" bugs.

caller mode = SERVER:
| caller is owner | receiver=Server | receiver=Owner | receiver=Broadcast |
| --- | --- | --- | --- |
| yes  | direct call | self (direct) | routed to all relevant clients |
| no   | direct call | routed to owner client | routed to all relevant clients |

caller mode = CLIENT:
| caller is owner | receiver=Server | receiver=Owner | receiver=Broadcast |
| --- | --- | --- | --- |
| yes  | routed to server | direct call | self (direct) |
| no   | DROPPED | DROPPED | self (direct) |

Key consequences:
- A client sending RplRcver.Server through an entity it does NOT own => DROPPED. No log, no
  error, just silence. This is the #1 dedicated-server bug.
- The host on a listen server IS the owner of the GameMode, so its Server RPCs work. A joiner
  is not the owner of the GameMode, so its Server RPC through the GameMode is dropped.
- Therefore: client->server requests must go through an entity the client owns - the
  PlayerController or the controlled character. Vanilla does exactly this
  (SCR_TaskNetworkComponent, SCR_CampaignNetworkComponent live on PlayerController).

Pattern (client request -> server):
```enforce
// On a component attached to the PlayerController (client owns it):
void RequestStartEscape() { Rpc(Rpc_RequestStartEscape_S); }

[RplRpc(RplChannel.Reliable, RplRcver.Server)]
protected void Rpc_RequestStartEscape_S()
{
    // runs on the server; call the authority-side logic directly (no further RplRpc needed)
    ESC_EscapeManagerComponent mgr = ESC_Utils.GetManager();
    if (mgr) mgr.StartEscape();
}
```
The authority-side method (StartEscape here) should NOT have its own [RplRpc]; it is now just
a normal server method invoked by the network component.

## Loadtime vs runtime vs local items

- Loadtime: placed in the editor. Proxy exists on every client regardless of distance.
  Required for: anything every client must always see (objectives, task markers, GameMode,
  task manager, the world's fixed structures).
- Runtime: created with SpawnEntityPrefab during play, on the server. Proxy exists only on
  clients that have streamed it in (relevance/proximity). Correct for AI, loot, vehicles,
  patrols - things that should only exist near players. Streaming out destroys the proxy;
  streaming back in re-runs EOnInit.
- Local: created on a client. Server is unaware. No proxies elsewhere. Use only for local
  prediction/visuals. By default the engine only allows prefab spawning on the server to
  prevent accidental local-only entities that users mistake for replicated ones.

Rule of thumb: if it must be visible to everyone at all distances, it must be loadtime. There
is no clean public "make this runtime entity exist on all clients" switch; the only robust
mechanism for global visibility is loadtime placement.

## RplComponent requirement

- No RplComponent => not replicated. Server-spawned => server-only. Client-spawned =>
  local-only.
- Verify at runtime on the authority (do NOT trust the child .et file, which hides inherited
  components):
```enforce
RplComponent rpl = RplComponent.Cast(myEntity.FindComponent(RplComponent));
Print(string.Format("rpl=%1 role=%2", rpl, rpl ? rpl.Role() : "NONE"), LogLevel.ERROR);
```
- If NONE: the prefab is missing the RplComponent. Fix by inheriting from a vanilla base that
  carries one (e.g. extend a vanilla task prefab), not by hand-adding a bare RplComponent to a
  root that lost it, and not by spawning per-client copies (which destroys replication).

## RPC timing: "unregistered item"

Symptom in logs:
```
RPL (E): Attempting to send an RPC through unregistered item. itemType='...SCR_TriggerTask', rpc='Rpc_AddTaskAssigneePlayer'
```
Cause: SpawnEntityPrefab returns an IEntity before its RplComponent is inserted into the
replication session. Any Rpc(...) through it in the same frame is dropped. Registration is
asynchronous.

Registration is NOT distance-dependent. Spawning the entity near the player does not make it
register faster. The fix is timing, not location.

Fix A (preferred for initial values) - replicated state instead of RPC:
```enforce
// initial values set as RplProp travel in the first snapshot (produced by registration)
[RplProp()]
protected SCR_ETaskState m_eInitialState = SCR_ETaskState.CREATED;
```
Or write them in RplSave. The snapshot has no registration-timing problem and is JIP-safe.

Fix B - defer the RPC off the spawn frame:
```enforce
IEntity ent = ESC_Utils.SpawnEntityPrefab(prefab, pos);
GetGame().GetCallqueue().CallLater(SetupAfterRegistered, 100, false, ent);
void SetupAfterRegistered(IEntity ent)
{
    // now registered; RPCs go through
    SCR_Task.Cast(ent).SetTaskState(SCR_ETaskState.CREATED);
}
```
There is no public "registered" callback; a short CallLater is the standard workaround.

## Broadcast RPCs and streaming/JIP

- RplRcver.Broadcast on an entity reaches only clients that currently have that entity's
  RplComponent streamed in.
- A broadcast fired BEFORE a client streams the entity in is lost for that client. JIP clients
  miss it entirely. (Cite: BI wiki, Loadtime and Runtime page.)
- Distance: a broadcast on a runtime entity 1500m from a player reaches no one there.
- Therefore, for state that must survive JIP and streaming (assignees, ownership, visibility,
  position), put it in RplSave/RplLoad (the snapshot), not in a broadcast RPC. Vanilla
  SCR_Task writes assignees via WriteExecutor/ReadExecutor in RplSave for this reason;
  AddTaskAssignee ALSO broadcasts, but the snapshot is what makes late clients catch up.

## Forcing streaming (last resort, with warnings)

RplComponent exposes:
- `EnableStreamingConNode(identity, bool)` - force-stream this node to ONE player (respects
  node rules; needs Network Dynamic Simulation).
- `static EnableStreamingForConnection(identity, bool)` - force-stream to one player,
  ignoring connection rules. The API doc warns: "Use with caution. This could cause
  performance issues if used by many players."

There is no clean "stream to all clients" toggle. To fake it you would loop every player and
re-run on every JIP - fragile and warned against. For a globally-visible objective, use
loadtime placement instead.

## Replicated state patterns

[RplProp] - field replicated from authority to proxies. Good for values that change
infrequently (status, faction key, a target id).
```enforce
[RplProp()]
protected int m_someValue;
```
RplSave/RplLoad - custom binary snapshot. Good for collections/complex data and for JIP-safe
initial state. Bump with `m_RplComponent.SetBumping()` (or equivalent) when you change the
data so proxies get an update.
[RplRpc] - remote procedure call. Use for EVENTS/changes after the entity is live, not for
initial state.

Choosing:
- Initial value on a freshly spawned entity => RplProp default or RplSave (snapshot). Avoid
  same-frame RPC.
- Later change that all proxies need => RplProp (auto-replicates) OR RplSave bump.
- One-off event/request => RplRpc (Server for client->server requests through owned entity;
  Broadcast for authority->all, but remember the streaming/JIP caveat).

## Common bug -> cause map

| Symptom | Likely cause |
| --- | --- |
| Works for host, nothing happens for joiners on dedicated | client->server RPC sent through a non-owned entity (GameMode); dropped. |
| "unregistered item" RPC errors | Rpc in the same frame as SpawnEntityPrefab; defer or use RplProp/RplSave. |
| Task/marker invisible on remote clients, fine in SP/listen | runtime-spawned far entity not streamed to clients; use loadtime, or it is missing RplComponent. |
| Assignee/state present on server but missing on a late/JIP client | state travels by broadcast RPC only; move it to RplSave snapshot. |
| Task hidden from some players | SCR_ETaskVisibility/Ownership faction/group filter does not match the client's executor faction/group yet. |
| Per-client spawned copies never update | local-only entities; server unaware. Stop spawning per-client; fix the prefab's RplComponent. |
