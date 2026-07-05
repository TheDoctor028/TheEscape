class ESC_ReadyUserAction: ScriptedUserAction
{
	protected ESC_EscapeManagerComponent m_manager;
	
	//------------------------------------------------------------------------------------------------
    override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
    {
		m_manager = ESC_EscapeManagerComponent.Cast(GetGame().GetGameMode().FindComponent(ESC_EscapeManagerComponent));
    }
	
    //------------------------------------------------------------------------------------------------
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
		// TODO
		return;   
    }
}

class ESC_StartEscapeUserAction: ScriptedUserAction
{
	
	//------------------------------------------------------------------------------------------------
    override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
    {
		
    }
	
	
    //------------------------------------------------------------------------------------------------
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
		ESC_Utils.GetManager().StartEscapeRcp();
    }

    //------------------------------------------------------------------------------------------------
    override bool CanBePerformedScript(IEntity user)
    {
        return true;
    }	
}
