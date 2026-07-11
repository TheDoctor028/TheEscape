//------------------------------------------------------------------------------------------------
//! Scripted interaction that lets a player revive an incapacitated teammate.
//! Only available when the target is in the unconscious state; performing it fully
//! heals the target and updates their consciousness status.
class ESC_ReviveUserAction: ScriptedUserAction
{
    //------------------------------------------------------------------------------------------------
    //! Init hook. The 5-second hold duration is currently commented out; see
    //! the code for the optional `SetActionDuration` line.
    //! \param pOwnerEntity Entity the action is attached to.
    //! \param pManagerComponent Owning component of the action.
    override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
    {
        // SetActionDuration(5.0); // 5 second hold to revive
    }

    //------------------------------------------------------------------------------------------------
    //! Performs the revive: resolves the target's damage manager, runs a full heal,
    //! and then calls `UpdateConsciousness()` to wake the target up.
    //! \param pOwnerEntity Entity the action is attached to.
    //! \param pUserEntity Player entity performing the revive.
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        SCR_CharacterDamageManagerComponent damageManager =
            SCR_CharacterDamageManagerComponent.Cast(
                pOwnerEntity.FindComponent(SCR_CharacterDamageManagerComponent)
            );

        if (!damageManager)
            return;

		// Lets heal the user full and wake him up
		damageManager.FullHeal();
        damageManager.UpdateConsciousness();
    }

    //------------------------------------------------------------------------------------------------
    //! The action is only valid if the interaction target is unconscious.
    //! \param user Player entity attempting to use the action.
    //! \return True if the action can be performed.
    override bool CanBePerformedScript(IEntity user)
    {
        return IsTargetUnconscious();
    }

    //------------------------------------------------------------------------------------------------
    //! The action prompt is only shown when the target is unconscious.
    //! \param user Player entity looking at the action target.
    //! \return True if the prompt should be visible.
    override bool CanBeShownScript(IEntity user)
    {
        return IsTargetUnconscious();
    }

    //------------------------------------------------------------------------------------------------
    //! Sets the displayed name of the action in the interaction menu.
    //! \param outName Output name for the action.
    //! \return Always true.
    override bool GetActionNameScript(out string outName)
    {
        outName = "Revive";
        return true;
    }

    //------------------------------------------------------------------------------------------------
    //! Checks the owner's character controller life state for `INCAPACITATED`.
    //! \return True if the action target is unconscious.
    protected bool IsTargetUnconscious()
    {
        SCR_CharacterControllerComponent controller =
            SCR_CharacterControllerComponent.Cast(
                GetOwner().FindComponent(SCR_CharacterControllerComponent)
            );

        if (!controller)
            return false;

        return controller.GetLifeState() == ECharacterLifeState.INCAPACITATED;
    }
}
