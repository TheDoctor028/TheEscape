# TheEscape

A small Arma Reforger (Enfusion Script / Enforce) mod that runs an "Escape" game
mode loop:

1. Players press a control-pole action to start the round.
2. The Escape manager picks an extraction point, generates a random starting
   point, drops the prison prop + starting backpack, and spawns a guard patrol.
3. Players are teleported to the prison, receive the extraction task, and have
   to fight through OPFOR patrols spawned around them until they reach the
   smoke-trigged extraction zone.

This repository contains:

- The Enfusion Script sources under `Scripts/Game/...` that implement the
  behavior above.
- Workbench prefabs and world layers used by the script code (referenced by
  prefab GUIDs hard-coded inside the `.c` files).
- Curated docs under `docs/` and project metadata for the Arma Reforger
  Workbench.

## Project layout

```
.
├── README.md                       # This file
├── addon.gproj                     # Workbench project metadata
├── ESC_ExtractionTask.et           # The extraction SCR task prefab
├── Prefabs/                        # Authored prefabs referenced by scripts
├── Missions/                       # Mission layer overrides
├── worlds/                         # World layers (terrain data)
├── Scripts/Game/                   # Enfusion Script (Enforce) sources
│   ├── Base/                       # Base project component
│   ├── ESC_*.c                     # Component files (see docs/SCRIPTS.md)
│   ├── GameMode/Escape/            # Escape-mode manager + helpers
│   └── Triggers/UserAction/        # World interactions
├── docs/                           # High-level project documentation
│   ├── ARCHITECTURE.md             # Game-mode architecture
│   └── SCRIPTS.md                  # File-by-file map
└── skills/                         # Enfusion scripting skill metadata
```

The Workbench project id is stored in `addon.gproj`. Open that project in the
Arma Reforger Workbench to edit scripts, prefabs, and worlds.

## Game mode at a glance

The authoritative conductor is `ESC_EscapeManagerComponent`, attached to the
game mode entity. It drives the round state machine
(`ESC_EscapeStatus`: `READY -> INPROGRESS -> EXTRACTION`).

Key collaborators:

- `ESC_EntityRegisterComponent` self-registers start and extraction objects.
- `ESC_TheatreComponent` populates the world with OPFOR patrols and civilian
  traffic based on map descriptors (`SCR_MapDescriptorComponent`).
- `ESC_EscapeSpawnManager` provides the procedural-start utilities (terrain
  filtering, slope rejection, prison + backpack drop, patrol spawn).
- `ESC_PatrolController` keeps a configurable number of OPFOR patrols around
  every player throughout the round.
- `ESC_StartingBackpackComponent` fills the prison's backpack with a pistol +
  magazines scaled to the connected-player count.
- `ESC_SmokeTriggerComponent` listens for a player reaching the extraction smoke
  trigger and transitions the round to `EXTRACTION`.
- `ESC_ReviveUserAction` keeps gameplay readable by letting players revive
  downed teammates instead of dying outright.

The `modded class SCR_CharacterDamageManagerComponent` in
`Scripts/Game/ESC_PlayerDeathPrefenter.c` enforces the "downed but not dead"
behavior on top of the engine damage pipeline.

## How to extend

### Add a new component

1. Subclass `ESC_ScriptComponent` (see `Scripts/Game/Base/ESC_ScriptComponent.c`)
   to inherit the project's `EOnInit` / `OnPostInit` / `OnInitServer` / `OnInitClient`
   pattern.
2. Place the script under `Scripts/Game/` (or a subfolder following the
   `Escape/...` convention for category-specific components).
3. Document the class with `//!` blocks and a `//----` separator following the
   Enfusion convention. The `docs-updater` skill is loaded into the project and
   outlines the standard.

### Add a new scripted user action

1. Subclass `ScriptedUserAction`.
2. Use `ESC_Utils.GetManager()` to reach the manager from any action.
3. Forward any "start round" intent via `ESC_EscapeManagerComponent.StartEscapeRcp()`
   so the work happens on the server.

### Tweak the round

Editor-tunable knobs live on `ESC_EscapeManagerComponent`:

- `m_randomStartingPointFromTheMap` (bool) - procedural vs. registered start.
- `m_patrolPrefabs` - prefab pool for the runtime `ESC_PatrolController`.
- `m_escapingFaction` - faction that receives notifications and the victory.

Editor-tunable knobs on `ESC_TheatreComponent`:

- `m_cityPatrolRscs`, `m_villagePatrolRscs` - OPFOR pools by settlement size.
- `m_cityWeight`, `m_villageWeight` - probability per town.
- `m_civilTrafficVehicles`, `m_civilCharacter` - civilian traffic defaults.
- `m_civTrafficIntensity` - number of civilian vehicles spawned at round start.

The patrol controller has static tunables in `ESC_PatrolController`:

- `SPAWN_RANGE_MIN` / `SPAWN_RANGE_MAX` - spawn ring around each player.
- `CHECK_INTERVAL_MS` - density check cadence.
- `WAYPOINT_COUNT`, `PATROL_RADIUS` - per-patrol shape.

## Documentation

- `docs/ARCHITECTURE.md` - high-level round flow and ownership.
- `docs/SCRIPTS.md` - file/role map for every `.c` file under `Scripts/Game/`.
- Inline Doxygen blocks (`//!`) describe every class and method following the
  Bohemia Interactive Enfusion convention.

## Notes / known limitations

- `ESC_HunterGroupController` is a placeholder class reserved for upcoming work.
- `ESC_Utils.GetRandomRscName` reads `items[Math.RandomInt(0, items.Count())]`,
  which can return out-of-range when the array is empty - callers must guard.
- The map center hard-coded in `ESC_TheatreComponent.InitTowns` is tuned for a
  generic 8 km Reforger map; adjust `mapCenter` for larger terrains.
