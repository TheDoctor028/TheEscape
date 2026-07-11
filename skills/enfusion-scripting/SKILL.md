---
name: enfusion-scripting
description: Use this skill whenever working with Enfusion / Arma Reforger scripting - writing or debugging .c script files, ScriptComponent classes, prefab (.et) files, replicated state, RPCs (RplRpc/RplProp), the task system (SCR_TaskSystem/SCR_Task/SCR_TaskExecutor), AI, or any multiplayer behavior. Trigger on mentions of Arma Reforger, Enfusion, Workbench, addons, .et/.conf/.rdb files, SCR_ prefixed classes, RplComponent, RplRpc, Replication.IsServer/IsClient, SpawnEntityPrefab, or modding tasks for Arma Reforger. ALWAYS trigger when a user reports something that works in single player or self-hosted/listen but fails on a dedicated server, or works for the host but not for joining players - that is almost always an Enfusion replication issue and this skill is built to diagnose it. Also trigger when a user sees "Attempting to send an RPC through unregistered item" in their logs, or asks about authority/proxy/owner, loadtime vs runtime, streaming, or JIP.
---

# Enfusion / Arma Reforger Scripting

Use this skill when writing or debugging scripts for Arma Reforger / Enfusion (the engine).
It covers the .c script system, ScriptComponent, prefab (.et) files, attributes, the task
system, AI, and especially multiplayer replication - the area that causes the most subtle bugs.

## The single most important mental model

Enfusion multiplayer is authority/proxy based. There is one server (the authority) and N
clients (proxies). A huge fraction of bugs that "work in single player / self-hosted but break
on a dedicated server" are replication bugs, not gameplay logic bugs. When a user reports that
symptom, jump straight to the replication checklist in
`references/multiplayer-replication.md` - do not burn time debugging gameplay logic first.

The reverse intuition is also worth internalizing: in single player the local machine is a
player-hosted server, so it is simultaneously authority AND owner. Code that silently relies on
"caller is owner" or "entity exists locally" passes in SP and listen, then fails on dedicated.
That gap is the bug, not a hint.

## Where to look for docs (lookup order)

When you need an API, a vanilla class body, or a replication rule, look in this order.
Full URLs and usage notes are in `references/docs-and-sources.md`.

1. Arma Reforger Script API reference (doxygen on community.bistudio.com) - official
   class/method signatures.
2. Arma Reforger Explorer (arexplorer.zeroy.com) - the actual vanilla .c source, versioned.
   Use this to see HOW a class works internally, not just its signature. Especially for any
   class with RplSave/RplLoad or Rpc_ methods.
3. BI Community Wiki - the Multiplayer Scripting page and the Replication (Loadtime and
   Runtime) pages. These hold the conceptual rules: the RPC routing table, streaming, JIP.
4. The user's own repo - grep for how an API is already used locally; existing usage is often
   the best template for the project's conventions.

Never reason about a replicated class from its method signatures alone. Read the vanilla
source to learn whether state travels in the RplSave snapshot (JIP-safe), via broadcast RPC
(lost on late/streamed-out clients), or both. That distinction is usually the answer.

## Core rules (read once, internalize)

### Authority vs proxy vs owner
- Server == authority. Clients hold proxies. Owner is a special proxy with extra rights.
- Only one machine can own an entity at a time. Ownership can transfer; authority cannot.
- A client may send an `RplRcver.Server` RPC only through an entity it OWNS (typically its
  PlayerController / controlled character). Sending through a server-authoritative entity
  (e.g. the GameMode) from a non-owner client is silently DROPPED. This is the #1 cause of
  "works for host, breaks for joining players" - the host is owner of the GameMode, joiners
  are not.
- This is why vanilla puts client->server request components (e.g. SCR_TaskNetworkComponent,
  SCR_CampaignNetworkComponent) on the PlayerController, not on the GameMode.

### Loadtime vs runtime items
- Loadtime items (placed in the world in the editor) always have a proxy on every client
  regardless of distance. Use these for things every client must always see: objectives, task
  markers, the GameMode, the task manager.
- Runtime items (SpawnEntityPrefab during play) are distance-streamed: a far client has no
  proxy at all. Use these for things that only matter near players: AI, loot, vehicles,
  patrols. Do NOT use a runtime entity for a globally-visible objective.
- Streaming out destroys the proxy on that client; streaming back in re-runs EOnInit.

### RplComponent is mandatory for replication
- No RplComponent = the entity is not replicated. If spawned on the server, it is server-only
  (no client ever sees it). If spawned on a client, it is local-only (the server is unaware
  and it receives no authority updates).
- NEVER "fix" missing visibility by spawning a copy on each client. That throws away
  replication entirely - the copies never receive assignee/state/position updates from the
  authority. Fix the prefab so it has an RplComponent (usually by inheriting from a vanilla
  base that carries one).
- In a child prefab (.et) file, inherited-but-unmodified components are NOT re-listed. "I
  don't see RplComponent in the .et text" does NOT mean it is absent. Verify at runtime on the
  authority: `FindComponent(RplComponent)` and check `Role()` (Authority vs none).

### RPC timing: "unregistered item"
- `SpawnEntityPrefab` returns an IEntity, but its RplComponent registration into the
  replication session is NOT synchronous. Calling `Rpc(...)` through it in the same frame logs
  `Attempting to send an RPC through unregistered item` and the RPC is dropped.
- Fix option A (preferred for initial values): set state as `[RplProp]` fields or write it in
  `RplSave` before first sync. The initial snapshot is produced by registration, so it has no
  timing problem and is JIP-safe.
- Fix option B (for changes after spawn): defer the RPC off the spawn frame with
  `CallLater(...)` (a frame or two, or a small fixed delay). There is no clean public
  "I'm registered" callback, so a short timer is the standard workaround.
- Reserve RPC for changes that happen after the entity is live. Prefer replicated state for
  initial setup.

### Broadcast RPCs only reach streamed-in clients
- An `RplRcver.Broadcast` RPC on an entity only reaches clients that currently have that
  entity's RplComponent streamed in. A broadcast fired before a client streams the entity in
  is lost for that client, and JIP clients miss it entirely.
- For state that must survive JIP and streaming (assignees, ownership, visibility, position),
  prefer data that travels in `RplSave`/`RplLoad` (the snapshot) over broadcast RPCs. Vanilla
  `SCR_Task` writes assignees into RplSave for exactly this reason.
- Distance matters: a broadcast on a runtime entity 1500m from a player reaches no one there.

## Prefabs and ScriptComponent basics

- A prefab (.et) is a tree of components with attributes. Child prefabs inherit parent
  components; only added/modified components appear in the child file text.
- ScriptComponent is the base for custom logic. Override EOnInit/OnPostInit; set EntityEvent
  masks in OnPostInit (e.g. `SetEventMask(owner, EntityEvent.INIT)`).
- `[Attribute(...)]` exposes fields to the editor. `[ComponentEditorProps(...)]` categorizes
  the component in the editor.
- For a replicated component, the entity needs an RplComponent and the component typically
  uses `[RplProp]` / `[RplRpc]` / `RplSave`/`RplLoad`.
- Patterns and a runtime RplComponent-verification snippet:
  `references/prefabs-and-components.md`

## The task system (SCR_TaskSystem / SCR_Task / SCR_TaskExecutor)

A very common trap: spawning an `SCR_Task` at runtime and assigning it in the same frame. It
hits the unregistered-item error AND far clients never stream it in, so the task is invisible
on dedicated servers. For an objective every player must see, pre-place the task (loadtime) at
each candidate location and activate/move the chosen one at runtime. For JIP-safe assignment,
have each client request assignment when it sees the task created
(`SCR_TaskSystem.GetOnTaskCreated()`), rather than broadcasting from the server in the spawn
frame. Full patterns and the faction-filter gotcha: `references/task-system.md`

## Spawning units, patrols, and traffic dynamically

Runtime spawning is the CORRECT pattern for gameplay entities (AI, loot, vehicles) - do not
pre-place them. The replication constraints that bite the objective task do NOT apply here,
because these things should only exist near players (streaming out at distance is desired and
performant). Recommended patterns:
- Near-player sector spawning (spawn N around each player in angular sectors with jitter).
- Pre-place spawn SLOTS/AREAS (loadtime markers), spawn units INTO a random subset at runtime.
  This gives determinism for JIP (slots are loadtime) while keeping units randomized and
  proximity-relevant. This is the vanilla ScenarioFramework pattern.
- Fixed ambient counts with dead-entity replacement for garrisons (spawn on proximity, despawn
  on departure, optionally preserve death state).
Avoid the unregistered-item trap by not RPCing through a freshly spawned entity in the same
frame; set initial state as RplProps or defer.

## When to read each reference

- `references/multiplayer-replication.md` - any "works in SP, breaks on dedicated" report; any
  Rpc/RplProp/RplSave work; any runtime-spawn visibility issue; the full RPC routing table.
- `references/docs-and-sources.md` - before guessing an API; to find vanilla source; to confirm
  replication rules with citations.
- `references/prefabs-and-components.md` - when creating/editing .et prefabs or ScriptComponent
  classes; to verify an inherited RplComponent.
- `references/task-system.md` - when working with SCR_TaskSystem, SCR_Task, SCR_TaskExecutor,
  or task assignment/visibility.

## Quick diagnostic checklist (run this FIRST on a replication bug)

Run these in order; most bugs are caught in the first three:

1. Does the entity prefab actually have an RplComponent? Check at runtime on the authority with
   `FindComponent(RplComponent)` and `Role()`. Inherited components do not show in child .et
   text, so do not trust the file alone.
2. Is the client->server RPC sent through an entity the client OWNS? An RplRcver.Server RPC
   from a non-owner client (e.g. through the GameMode) is silently dropped. Route client
   requests through a component on the PlayerController.
3. Is the RPC called in the same frame as SpawnEntityPrefab? That gives "unregistered item".
   Defer with CallLater, or use RplProp/RplSave for initial state.
4. Is the entity loadtime or runtime? If runtime and far from players, clients have no proxy.
   For a globally-visible objective, pre-place it (loadtime).
5. Is the relevant state in RplSave (snapshot, JIP-safe) or only in a broadcast RPC (lost on
   late/streamed-out clients)?
6. Does visibility depend on faction/group/executor filters (SCR_ETaskVisibility,
  SCR_ETaskOwnership) that may not match on the client yet? e.g. a FACTION-visibility task
   with owner faction "US" is hidden from any executor whose faction is not "US".

## Communication note

Enfusion multiplayer concepts (authority/proxy/owner, loadtime/runtime, streaming) are
unfamiliar to many users. When diagnosing, briefly frame WHY a symptom happens (e.g. "the host
is the owner of the GameMode, joiners are not, so their Server RPC is dropped") rather than
just giving the fix - it helps the user avoid the same trap next time.
