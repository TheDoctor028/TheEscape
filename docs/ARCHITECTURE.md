# Architecture

High-level architecture of TheEscape's Escape game mode. The diagram below shows
the canonical ownership between the runtime systems, prefabs, and the player.

```
+-----------------------------+        +-----------------------------+
| Player (ScriptedUserAction) |        | World                     |
|  - ESC_StartEscapeUserAction|        |  - ESC_EntityRegisterComp |
|  - ESC_ReviveUserAction     |        |  - ESC_StartingBackpackComp|
+--------------+--------------+        |  - ESC_SmokeTriggerComp   |
               |                       +-------------+---------------+
               | call StartEscapeRcp                 | on init
               v                                     v
      +--------+--------+               +------------+------------+
      |RplRpc (server)  |               |ESC_EscapeManagerComp   |
      |ESC_EscapeManager|-------------->| * startPoints          |
      +--------+--------+   register    | * extractionPoints     |
               |                        | * extractionTask       |
               | inits / drives         | * per-player map       |
               v                        | * ESC_PatrolController |
      +--------+--------+               +------------+------------+
      |ESC_EscapeSpawnMgr| spawn prison |            |
      | * GenerateStartPt| spawn bagpack|  StartEscape()
      +--------+--------+               v
               |                  +-----+--------+
               | uses             |ESC_Theatre   |
               v                  | * towns/villages
      +--------+--------+         | * patrol spawning
      |ESC_Utils       |         | * civilian traffic
      | * LoadResource |         +-----+--------+
      | * SpawnEntity  |               |
      | * Players()    |               | spawns
      | ...            |               v
      +----------------+         +-----+--------+
                                 |ESC_Patrol    |
                                 | patrol flows |
                                 +--------------+
```

## Round lifecycle

The round state lives on `ESC_EscapeManagerComponent.m_escapeStatus`, exposed as
`ESC_EscapeStatus`:

| State        | Entry condition                                                  | Exit condition                                |
| ------------ | ---------------------------------------------------------------- | --------------------------------------------- |
| `READY`      | Default value when the manager spawns.                           | `StartEscape` RPC arrives and succeeds.       |
| `INPROGRESS` | `StartEscape` has placed players, tasks, and patrols.            | Player steps into the smoke trigger.          |
| `EXTRACTION` | `ESC_SmokeTriggerComponent.HandleActivate` flips the state.      | `ESC_Utils.EndGame` ends the round (5 s delay). |

## Self-registration

`ESC_EntityRegisterComponent` forwards its owner entity to
`ESC_EscapeManagerComponent.Register` based on the editor-selected
`ESC_ControlledEntityType` (`START_POINT` or `EXTRATION_POINT`). This is the
canonical way to add new candidates without modifying the manager.

## Procedural starting point

`ESC_EscapeSpawnManager.GenerateStartPoint` is a rejection-sampling loop:

1. Pick a random spawn in a hard-coded 11500 x 12300 m square.
2. Reject if the surface elevation is below `YLimitForSpawn`.
3. Reject if the slope (via `ESC_Utils.GetSteepness`) is above `SteepnessLimitForSpawn`.
4. Reject if the distance to the chosen extraction point is below
   `MinimumExtractionDistance`.
5. Otherwise spawn the prison prop + starting backpack at the spot and return.

The starting round then drops:
- `ESC_StartingBackpackComponent` fills the backpack with one pistol + two
  magazines per connected player.
- A small guard `ESC_Patrol` placed 10 m east of the prison.
- All current players are teleported to the spot and assigned the extraction
  task.

## World population

`ESC_TheatreComponent` runs on server init:

1. `InitTowns` queries the world around `(4000, 4000)` within 10 km, bucketing
   `SCR_MapDescriptorComponent` hits into cities/villages/military bases.
2. `SpawnActorsToTowns` seeds OPFOR patrols around every town using prefab
   pools from the editor attributes.
3. `SpawnCivilianTraffic` spawns a configurable number of civilian vehicles
   driving between random villages.

## Runtime patrols

`ESC_PatrolController.StartPatrolling` loops every `CHECK_INTERVAL_MS`. Each
tick:

1. Prunes `m_activePatrols2` of patrols whose AI group has died.
2. For each connected player, counts patrols in `m_checkRadius`.
3. Spawns patrols (via `ESC_EscapeSpawnManager.SpawnPatrolAroundCoordinate2`)
   in random angular sectors around the player until the desired count is met.

## Damage / revive flow

`ESC_PlayerDeathPrefenter.c` mods `SCR_CharacterDamageManagerComponent`:

- When a player's hit-zone falls below 1% health, clamps it to exactly 1% and
  flips the character controller into the unconscious state.

`ESC_ReviveUserAction` exposes an interaction prompt that is only valid while
the target is unconscious; performing it calls `FullHeal()` and refreshes the
consciousness state.

## Triggering extraction

`ESC_SmokeTriggerComponent` listens on `SCR_BaseTriggerEntity.GetOnActivate()`
and calls `ESC_EscapeManagerComponent.StartExtraction`. The trigger is latched
(`m_activated`) so subsequent activations are no-ops.
