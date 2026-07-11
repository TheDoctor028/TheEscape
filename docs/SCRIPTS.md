# Scripts map

File-by-file map of every `.c` file under `Scripts/Game/`. Use this as the
quick lookup table; the Doxygen inside each file is the source of truth.

## Base

| File                                 | Role                                                                |
| ------------------------------------ | ------------------------------------------------------------------- |
| `Base/ESC_ScriptComponent.c`         | Project base `ScriptComponent`. Provides shared `EOnInit` / `OnPostInit` / `OnInitServer` / `OnInitClient` and caches `m_owner`. New project components should extend this. |

## Top-level components

| File                                       | Role                                                                                                            |
| ------------------------------------------ | --------------------------------------------------------------------------------------------------------------- |
| `ESC_EscapeManagerComponent.c`             | Authoritative game-mode conductor. Owns the round state, start/extraction registries, extraction task, per-player map, and the runtime patrol controller. |
| `ESC_EscapeSpawnManager.c`                 | Static helper class. Spawn rules, terrain rejection, prison+backpack placement, patrol prefab spawn helpers.    |
| `ESC_TheatreComponent.c`                   | World population. Discovers settlements via map descriptors, seeds OPFOR patrols, and spawns civilian traffic. |
| `ESC_PatrolController.c`                   | Runtime OPFOR patrol density manager. Maintains a configurable number of patrols around every connected player. |
| `ESC_Patrol.c`                             | High-level patrol wrapper around `SCR_AIGroup`. Builds circular and random-disk patrols with waypoint cycles.   |
| `ESC_EntityRegisterComponent.c`            | Self-registration component. Forwards its owner to the manager with a chosen `ESC_ControlledEntityType`.        |
| `ESC_EscapeTaskComponent.c`                | Marker component placed on the extraction `SCR_TriggerTask` prefab so non-task systems can reach it.            |
| `ESC_PlayerDeathPrefenter.c`               | Modded `SCR_CharacterDamageManagerComponent` that keeps players in the unconscious state instead of dying.     |
| `ESC_StartingBackpack.c`                   | Backpack component that fills itself with one pistol and two magazines per connected player.                    |
| `ESC_HunterGroupController.c`              | Placeholder reserved for future Hunter-faction AI.                                                              |

## Triggers / user actions

| File                                       | Role                                                                                                            |
| ------------------------------------------ | --------------------------------------------------------------------------------------------------------------- |
| `Triggers/ESC_SmokeTrigger.c`              | Listener for the extraction smoke trigger. Latches once and flips the manager into `EXTRACTION`.                |
| `UserAction/ESC_ControlePoleActions.c`     | Control-pole interactions. `ESC_ReadyUserAction` (TODO stub) and `ESC_StartEscapeUserAction` (kicks the round).  |
| `UserAction/ESC_ReviveUserAction.c`        | Revive interaction. Only available while the target is unconscious. Calls `FullHeal` and refreshes consciousness. |

## Utilities

| File                | Role                                                                                                                |
| ------------------- | ------------------------------------------------------------------------------------------------------------------- |
| `ESC_Utils.c`       | Project-wide static helpers: `LoadResource`, `SpawnEntity`, `SpawnVehicleWithDriver`, player lookup, AI group factory, ground-snap, end-game, manager lookup, plus the `ESC_Player`, `ESC_Waypoints`, `ESC_WaypointType`, `ESC_SCR_MapDescriptorComponent_Types` types. |
