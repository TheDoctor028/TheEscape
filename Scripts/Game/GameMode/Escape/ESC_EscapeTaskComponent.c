class ESC_EscapeTaskComponentClass : ScriptComponentClass {}

[ComponentEditorProps(category: "Escape/Tasks", description: "The EscapeTask component.")]
class ESC_EscapeTaskComponent : ScriptComponent
{
	protected SCR_TriggerTask m_task;
	
	override void EOnInit(IEntity owner)
    {	
		if(!GetGame().InPlayMode()) return;
		
		if (Replication.IsServer()) OnInitServer();
		
		if (Replication.IsClient()) OnInitClient();
		
		m_task = SCR_TriggerTask.Cast(owner);
    }

    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }
	
	protected void OnInitServer()
	{
	}
	
	
	protected void OnInitClient()
	{
	}
}