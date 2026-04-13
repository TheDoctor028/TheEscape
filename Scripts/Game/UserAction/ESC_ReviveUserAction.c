class ESC_ReviveUserAction: ScriptedUserAction
{
    //------------------------------------------------------------------------------------------------
    override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
    {
        SetActionDuration(5.0); // 5 second hold to revive
    }

    //------------------------------------------------------------------------------------------------
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
    override bool CanBePerformedScript(IEntity user)
    {
        return IsTargetUnconscious();
    }

    //------------------------------------------------------------------------------------------------
    override bool CanBeShownScript(IEntity user)
    {
        return IsTargetUnconscious();
    }

    //------------------------------------------------------------------------------------------------
    override bool GetActionNameScript(out string outName)
    {
        outName = "Revive";
        return true;
    }

    //------------------------------------------------------------------------------------------------
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