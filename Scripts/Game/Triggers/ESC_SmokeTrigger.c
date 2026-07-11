//------------------------------------------------------------------------------------------------
//! Class identity for the extraction smoke trigger component.
class ESC_SmokeTriggerComponentClass : ESC_ScriptComponentClass { }

//------------------------------------------------------------------------------------------------
//! Trigger listener placed on the extraction point. Subscribes to the underlying
//! `SCR_BaseTriggerEntity.OnActivate` event and, when triggered, kicks off the
//! escape manager's extraction phase. Guarded by `m_activated` so subsequent
//! activation events are no-ops.
[ComponentEditorProps(category: "Escape/Triggers", description: "Add this to the trigger that OnActivate needs to handle the start of the extration.")]
class ESC_SmokeTriggerComponent : ESC_ScriptComponent
{
	//! Latched flag ensuring the extraction phase starts exactly once.
	protected bool m_activated = false;

	//------------------------------------------------------------------------------------------------
	//! Server init hook. Hooks the owner trigger's `OnActivate` invoker to `HandleActivate`.
	override void OnInitServer()
	{
		ScriptInvoker onActivate = SCR_BaseTriggerEntity.Cast(m_owner).GetOnActivate();

		onActivate.Insert(HandleActivate);
	}

	//------------------------------------------------------------------------------------------------
	//! Activation callback. Latches `m_activated`, logs the event, and asks the
	//! escape manager to start the extraction phase.
	//! \param ent Entity that entered the trigger (unused; the manager handles it).
	protected void HandleActivate(IEntity ent)
	{
		if (m_activated) return;
		m_activated = true;

		Print("ESC_SmokeTriggerComponent.HandleActivate: Got somoke");
		ESC_Utils.GetManager().StartExtraction();
	}
}
