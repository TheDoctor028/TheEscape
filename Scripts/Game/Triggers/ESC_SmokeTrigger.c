class ESC_SmokeTriggerComponentClass : ESC_ScriptComponentClass {}

[ComponentEditorProps(category: "Escape/Triggers", description: "Add this to the trigger that OnActivate needs to handle the start of the extration.")]
class ESC_SmokeTriggerComponent : ESC_ScriptComponent
{
	protected bool m_activated = false;
	
	override void OnInitServer()
	{
		ScriptInvoker onActivate = SCR_BaseTriggerEntity.Cast(m_owner).GetOnActivate();
		
		onActivate.Insert(HandleActivate);
	}
	
	protected void HandleActivate(IEntity ent)
	{
		if (m_activated) return;
		m_activated = true;
		
		Print("ESC_SmokeTriggerComponent.HandleActivate: Got somoke");
		ESC_Utils.GetManager().StartExtraction();
	}	
}