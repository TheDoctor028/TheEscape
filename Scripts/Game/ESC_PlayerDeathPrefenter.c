modded class SCR_CharacterDamageManagerComponent : SCR_DamageManagerComponent
{	
	protected override void OnDamage(notnull BaseDamageContext damageContext)
	{
		if (SCR_CharacterHelper.IsAPlayer(GetOwner()))
        {
			if(GetDefaultHitZone().GetHealthScaled() < 0.01)
			{
				Print("ESC_SCR_CharacterDamageManagerComponent.OnDamage: Preventing death");
				GetDefaultHitZone().SetHealthScaled(0.01);
				
				ChimeraCharacter character = ChimeraCharacter.Cast(GetOwner());
	            CharacterControllerComponent controller = character.GetCharacterController();
	            
				// Use existing logic to determine unconscious state, but NEVER call Kill()
				if (controller && character)
	                controller.SetUnconscious(ShouldBeUnconscious());
			}
		}
		
		super.OnDamage(damageContext);
	}
}
