# Docs and sources for Enfusion / Arma Reforger

Use these in lookup order. Citations matter - when you tell a user a replication rule, point
at the source so they can verify and learn.

## 1. Arma Reforger Script API reference (doxygen)

- Index: https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/annotated.html
- Class pages look like: .../interfaceSCR__TaskSystem.html (note the double underscore for
  namespace separation in the URL).
- Use for: official method signatures, parameter names, enum values, inheritance diagrams,
  the list of protected Rpc_ / RplSave / RplLoad members (tells you what is replicated).
- Limitation: signatures only, no bodies. You cannot tell from the doxygen alone whether a
  setter RPCs (broadcast, lost on late clients) or writes RplSave (snapshot, JIP-safe). Use
  Arma Reforger Explorer (below) for the body.

Search tips:
- Replication primitives (RplComponent, RplRpc, RplProp, RplChannel, RplRcver, RplRole,
  RplCondition): see the Replication group page
  .../group__Replication.html and Page_Replication_LoadtimeAndRuntime.html.

## 2. Arma Reforger Explorer (vanilla source)

- Root: https://arexplorer.zeroy.com/
- Versioned per game build (e.g. v1.1.0.42, v1.7.0.54). Pick the build closest to the user's
  project; APIs drift between builds.
- Source file URLs look like: .../_s_c_r___task_system_8c_source.html
  (lowercase, underscores, _8c_source).
- Use for: the actual bodies of vanilla classes. ALWAYS read this for any class that has
  RplSave/RplLoad or Rpc_ methods - it is the only way to know how state actually moves.
- Good examples worth knowing:
  - SCR_TaskSystem.c - AssignTask, CreateTask, RegisterTask, IsTaskVisibleFor,
    CanTaskBeAssignedTo.
  - SCR_Task.c - RplSave/RplLoad, WriteExecutor/ReadExecutor, AddTaskAssignee_Proxy /
    Rpc_AddTaskAssigneePlayer (broadcast).
  - SCR_TaskSystemNetworkComponent.c - the vanilla client->server request pattern (attached
    to PlayerController).
  - SCR_BaseTaskExecutor.c / SCR_TaskExecutor.c - FromPlayerID/FromLocalPlayer.

## 3. BI Community Wiki - conceptual rules

- Multiplayer Scripting: https://community.bistudio.com/wiki/Arma_Reforger:Multiplayer_Scripting
  (network model, entity roles, RPC, JIP, streaming, relevance).
- Replication - Loadtime and Runtime:
  https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/Page_Replication_LoadtimeAndRuntime.html
  (the loadtime vs runtime vs local distinction, streaming rules, the "broadcast before
  streamed in is lost" warning).
- Replication group (enums/types):
  https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/group__Replication.html
  (RplChannel, RplRcver, RplRole, RplCondition, and the full RPC routing table for
  RplRcver.Server/Owner/Broadcast by caller mode and ownership).
- Use for: citing the rules. The RPC routing table is the authoritative answer to "why does
  this RPC work for the host but not joiners."

## 4. The user's own repo

- grep for the API in the project's Scripts/ to see local usage and conventions.
- Prefab (.et) files: a child prefab lists only added/modified components; inherited
  components are implicit. To know a prefab's full component set, open the PARENT chain.
- resourceDatabase.rdb / addon.gprojects / Missions/*.conf describe the addon and mission.

## How to use these together (typical flow)

1. User asks about a vanilla class or a replication symptom.
2. Get the signature from the doxygen (source 1).
3. Get the body from Arma Reforger Explorer (source 2) - especially RplSave/RplLoad/Rpc.
4. If a rule is in question, cite the BI wiki (source 3).
5. Match against the user's actual code (source 4).

Do not guess API behavior. If you cannot find the body, say so and reason from the rules in
references/multiplayer-replication.md rather than inventing semantics.
