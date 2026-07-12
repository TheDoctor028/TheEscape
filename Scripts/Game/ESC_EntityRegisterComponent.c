class ESC_EntityRegisterComponentClass : ESC_ScriptComponentClass {}

[ComponentEditorProps(category: "Escape/Components", description: "Add this to any prefab that you want to be able to register them self oninit to the manager.")]
class ESC_EntityRegisterComponent : ESC_ScriptComponent
{
	[Attribute(uiwidget: UIWidgets.ComboBox, desc: "The type of this entity that we are registering to the manager.", enums: ParamEnumArray.FromEnum(ESC_ControlledEntityType))]
	protected ESC_ControlledEntityType m_type;
	
	override protected void OnInitServer()
	{
		RegisterOwner();
	}
	
	protected void RegisterOwner()
	{
		const ESC_EscapeManagerComponent m_manager = ESC_EscapeManagerComponent.GetInstance();
		if (m_manager == null)
		{
			Print("ESC_EntityRegisterComponent.RegisterSelf: Cant find manager on GameMode", LogLevel.ERROR);
			return;
		}
		
		m_manager.Register(GetOwner(), m_type);
	}
	
}