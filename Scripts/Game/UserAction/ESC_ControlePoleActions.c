//------------------------------------------------------------------------------------------------
//! Interaction stub on the control pole. Caches the escape manager during init.
//! `PerformAction` is intentionally left as a TODO; this action can later be used
//! for player-ready signalling or lobby management.
class ESC_ReadyUserAction: ScriptedUserAction
{
	//! Cached pointer to the escape manager, resolved during `Init`.
	protected ESC_EscapeManagerComponent m_manager;

	//------------------------------------------------------------------------------------------------
	//! Resolves the escape manager on the current game mode so the ready action
	//! can interact with it in the future.
	//! \param pOwnerEntity Entity the action is attached to.
	//! \param pManagerComponent Owning component of the action.
    override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
    {
		m_manager = ESC_EscapeManagerComponent.Cast(GetGame().GetGameMode().FindComponent(ESC_EscapeManagerComponent));
    }

	//------------------------------------------------------------------------------------------------
	//! Placeholder action body. Does nothing today; reserved for future lobby/ready logic.
	//! \param pOwnerEntity Entity the action is attached to.
	//! \param pUserEntity Player entity performing the action.
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
		// TODO
		return;
    }
}

//------------------------------------------------------------------------------------------------
//! Interaction on the control pole that starts the Escape round.
//! This is the player-facing entry point: it calls the manager's `StartEscapeRcp`
//! RPC, which forwards the request to the server.
class ESC_StartEscapeUserAction: ScriptedUserAction
{

	//------------------------------------------------------------------------------------------------
	//! Empty init. The action doesn't need any local state beyond the default action.
	//! \param pOwnerEntity Entity the action is attached to.
	//! \param pManagerComponent Owning component of the action.
    override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
    {

    }

	//------------------------------------------------------------------------------------------------
	//! Triggers the escape sequence by calling the manager's RPC. The actual start
	//! logic runs on the server in `ESC_EscapeManagerComponent.StartEscape`.
	//! \param pOwnerEntity Entity the action is attached to.
	//! \param pUserEntity Player entity pressing the start button.
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
		ESC_Utils.GetManager().StartEscapeRcp();
    }

	//------------------------------------------------------------------------------------------------
	//! The start button is always available when the user is looking at the pole.
	//! \param user Player entity attempting to use the action.
	//! \return Always true.
    override bool CanBePerformedScript(IEntity user)
    {
        return true;
    }
}
