//------------------------------------------------------------------------------------------------
//! Class identity for the escape task component.
class ESC_EscapeTaskComponentClass : ScriptComponentClass { }

//------------------------------------------------------------------------------------------------
//! Component attached on top of an `SCR_TriggerTask` prefab for backward
//! compatibility with systems that expect a `ScriptComponent` on a task entity.
//! Caches the owner as an `SCR_TriggerTask` so other systems (manager, UI) can
//! reach the task without re-casting on every access.
[ComponentEditorProps(category: "Escape/Tasks", description: "The EscapeTask component.")]
class ESC_EscapeTaskComponent : ScriptComponent
{
	//! Cached pointer to the owner `SCR_TriggerTask`, set in `EOnInit`.
	protected SCR_TriggerTask m_task;

	//------------------------------------------------------------------------------------------------
	//! Engine init. Skips the Workbench, dispatches to server/client, and casts
	//! the owner entity into `m_task`.
	//! \param owner Entity this component is attached to (must be an `SCR_TriggerTask`).
	override void EOnInit(IEntity owner)
    {
		if(!GetGame().InPlayMode()) return;

		if (Replication.IsServer()) OnInitServer();

		if (Replication.IsClient()) OnInitClient();

		m_task = SCR_TriggerTask.Cast(owner);
    }

	//------------------------------------------------------------------------------------------------
	//! Subscribes to the INIT entity event mask for late-init support.
	//! \param owner Entity this component is attached to.
    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }

	//------------------------------------------------------------------------------------------------
	//! Server init hook. Intentionally empty: the component itself does not require
	//! any server-only setup because task state is owned by `SCR_TriggerTask`.
	protected void OnInitServer()
	{
	}

	//------------------------------------------------------------------------------------------------
	//! Client init hook. Intentionally empty.
	protected void OnInitClient()
	{
	}
}
