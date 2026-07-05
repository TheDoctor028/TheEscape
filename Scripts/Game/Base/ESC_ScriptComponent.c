/* QUICK START:
------
class ESC_<REPLACEME>ComponentClass : ESC_ScriptComponentClass {}

[ComponentEditorProps(category: "Escape/Game", description: "<TODO>")]
class ESC_<REPLACEME>Component : ESC_ScriptComponent
{ }
------
*/

class ESC_ScriptComponentClass : ScriptComponentClass { }

class ESC_ScriptComponent : ScriptComponent
{
	protected IEntity m_owner;
	
	override void EOnInit(IEntity owner)
    {	
		if(!GetGame().InPlayMode()) return;
		
		m_owner = owner;
		
		if (Replication.IsServer()) OnInitServer();
		
		if (Replication.IsClient()) OnInitClient();
    }

    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }
	
	protected void OnInitServer();	
	
	protected void OnInitClient();
}
