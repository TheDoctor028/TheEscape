class ESC_EntityRegisterComponentClass : ScriptComponentClass {}

[ComponentEditorProps(category: "Escape/Components", description: "Add this to any prefab that you want to be able to register them self oninit to the manager.")]
class ESC_EntityRegisterComponent : ScriptComponent
{
	[Attribute(uiwidget: UIWidgets.ComboBox, desc: "The type of this entity that we are registering to the manager.", enums: ParamEnumArray.FromEnum(ESC_ControlledEntityType))]
	protected ESC_ControlledEntityType m_type;
	
	protected ESC_EscapeManagerComponent m_manager;
	
	override void EOnInit(IEntity owner)
    {	
		if(!GetGame().InPlayMode()) return;
		
		if (Replication.IsServer()) OnInitServer();
		
		if (Replication.IsClient()) OnInitClient();
    }

    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }
	
	protected void OnInitServer()
	{
		RegisterOwner();
	}
	
	
	protected void OnInitClient()
	{
	}
	
	protected void RegisterOwner()
	{
		m_manager = ESC_EscapeManagerComponent.Cast(GetGame().GetGameMode().FindComponent(ESC_EscapeManagerComponent));
		if (m_manager == null)
		{
			Print("ESC_EntityRegisterComponent.RegisterSelf: Cant find manager on GameMode", LogLevel.ERROR);
			return;
		}
		
		//GetGame().GetCallqueue().CallLater(m_manager.Register, 100, false, GetOwner(), m_type);
		m_manager.Register(GetOwner(), m_type);
	}
	
}