# Prefabs and ScriptComponent patterns

Use when creating or editing .et prefabs or ScriptComponent classes, and when verifying
whether an entity is actually replicated.

## Prefab (.et) file format

A prefab is a tree: a parent reference, an ID, optional components, coords, and child entities.
Example (child of a vanilla task):
```
SCR_TriggerTask : "{7259F0B101761BD5}Prefabs/Tasks/MoveTask.et" {
 ID "55D072570E7E8AB8"
 components {
  SCR_MapDescriptorComponent "{55D072570E7E8AB7}" {
   DisplayName "Extraction"
  }
 }
 m_sTaskID "extraction_move"
 m_eTaskOwnership FACTION
 m_aOwnerFactionKeys { "US" }
}
```
- The `: "{guid}Prefabs/...et"` line is the parent. The child INHERITS all parent components
  and attributes. Only components you ADD or MODIFY appear in the child's `components {}` block.
- A `{guid}` prefix is a compiled-resource GUID; it is how the engine references prefabs/assets
  portably. Keep GUIDs intact when editing.
- `.et.meta` siblings describe platform configurations (PC/XBOX/PS/HEADLESS). Usually leave
  alone.

CRITICAL consequence: an inherited RplComponent does NOT appear in the child .et text. Do not
conclude "no RplComponent" from reading the file. Verify at runtime (see below).

## ScriptComponent lifecycle

```enforce
class ESC_MyComponentClass : ScriptComponentClass {}

[ComponentEditorProps(category: "Escape/Components", description: "...")]
class ESC_MyComponent : ScriptComponent
{
    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT); // request EOnInit
    }

    override void EOnInit(IEntity owner)
    {
        if (!GetGame().InPlayMode()) return;
        if (Replication.IsServer()) OnInitServer();
        if (Replication.IsClient()) OnInitClient();
    }

    protected void OnInitServer() { /* authority-only setup */ }
    protected void OnInitClient() { /* proxy-only setup */ }
}
```
- OnPostInit runs once after all components are added; set event masks here.
- EOnInit runs when the INIT event fires (because you asked for it via SetEventMask).
- Split server/client paths with Replication.IsServer()/IsClient(). On a listen server BOTH
  return true; on a dedicated server only IsServer() is true; on a pure client only IsClient().
- For a GameMode component, owner is the GameMode; it is authority-only logic typically.

## Attributes (editor-exposed fields)

```enforce
[Attribute(uiwidget: UIWidgets.CheckBox, defvalue: "true", desc: "Enable random start")]
protected bool m_bRandomStart;

[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Patrol prefabs")]
protected ref array<ResourceName> m_patrolPrefabs;

[Attribute(uiwidget: UIWidgets.ComboBox, desc: "Type",
  enums: ParamEnumArray.FromEnum(ESC_ControlledEntityType))]
protected ESC_ControlledEntityType m_type;
```
- `ref array<ResourceName>` for arrays of resource names; mark with `ref` for ref-counted
  types.
- ParamEnumArray.FromEnum(MyEnum) populates a combo box from an enum.

## Replicated component patterns

Minimal replicated state:
```enforce
class ESC_MyComponent : ScriptComponent
{
    [RplProp()]
    protected ESC_EscapeStatus m_eStatus = ESC_EscapeStatus.READY;

    protected RplComponent m_Rpl;

    override void EOnInit(IEntity owner)
    {
        m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
    }

    // Call on the server to change state; RplProp auto-replicates to proxies.
    void SetStatus(ESC_EscapeStatus s)
    {
        if (!Replication.IsServer()) return;
        m_eStatus = s;
        // if RplProp doesn't auto-bump on assign, call m_Rpl.SetBumping() as needed
    }
}
```
Custom snapshot (collections/complex data, JIP-safe):
```enforce
override bool RplSave(ScriptBitWriter writer)
{
    writer.WriteInt(m_players.Count());
    foreach (int id : m_players) writer.WriteInt(id);
    return true;
}
override bool RplLoad(ScriptBitReader reader)
{
    int n; reader.ReadInt(n);
    m_players.Clear();
    for (int i = 0; i < n; i++) { int id; reader.ReadInt(id); m_players.Insert(id); }
    return true;
}
```
After mutating data that RplSave serializes, bump so proxies get an update
(`m_Rpl.SetBumping()` / the project's equivalent).

RPC:
```enforce
[RplRpc(RplChannel.Reliable, RplRcver.Server)]
protected void StartEscape() { /* runs on the server */ }

void StartEscapeRcp() { Rpc(StartEscape); } // call from a client that OWNS this entity
```
See references/multiplayer-replication.md for the routing rules - do NOT send RplRcver.Server
from a client through an entity it does not own.

## Verifying an entity is replicated (do this FIRST when visibility is broken)

On the authority, right after spawn (or in EOnInit for a placed entity):
```enforce
RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
Print(string.Format("[AUTH] rpl=%1 role=%2", rpl, rpl ? rpl.Role() : "NONE"), LogLevel.ERROR);
```
- NONE => not replicated. Fix the prefab (inherit from a vanilla base with an RplComponent).
- Authority => good; if clients still don't see it, it is a loadtime/runtime streaming issue
  (see references/multiplayer-replication.md), not a missing-component issue.

On a client:
```enforce
RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
Print(string.Format("[CLIENT] rpl=%1 role=%2", rpl, rpl ? rpl.Role() : "NONE"), LogLevel.ERROR);
```
- NONE on a client that should have the entity => it hasn't streamed in (runtime + far), or the
  prefab lacks an RplComponent (server-only entity).

## Common prefab mistakes

- Extending a root that lost the RplComponent => server-only entity. Re-parent onto a vanilla
  base that carries one.
- Editing a child .et and expecting to "see" inherited components. You won't; check the parent
  chain or verify at runtime.
- Spawning a prefab on a client to "fix" visibility. That creates a local-only entity; stop.
- Forgetting SetEventMask(owner, EntityEvent.INIT) in OnPostInit => EOnInit never runs.
- Forgetting `if (!GetGame().InPlayMode()) return;` => editor-mode side effects.
