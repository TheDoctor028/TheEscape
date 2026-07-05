[ComponentEditorProps(category: "Escape/Components", description: "The Script component that should be put to the bapack to fill its inventory.")]
class ESC_StartingBackpackComponentClass: ScriptComponentClass
{

}

class ESC_StartingBackpackComponent: ScriptComponent
{
	
	protected ResourceName  m_startingPistol = "{1353C6EAD1DCFE43}Prefabs/Weapons/Handguns/M9/Handgun_M9.et";
	protected ResourceName  m_startingMag = "{9C05543A503DB80E}Prefabs/Weapons/Magazines/Magazine_9x19_M9_15rnd_Ball.et";
	
	override void EOnInit(IEntity owner)
    {
		if(!GetGame().InPlayMode() || Replication.IsClient()) return;
		
		Print("ESC_StartingBackpackComponent.EOnInit: Init!");
		
		// There migt be a better way but I am failing to find it, on how to trigger that the correct time.
		GetGame().GetCallqueue().CallLater(SpawnStartingItems, 100, false, owner);
		
		Print("ESC_StartingBackpackComponent.EOnInit: Done!");
	}
	
	protected void SpawnItemToBackpack(SCR_UniversalInventoryStorageComponent inv, ResourceName resource, int count)
	{
		for (int i = 0; i < count; i++)
		{
			const IEntity ent = ESC_Utils.SpawnEntity(resource, {0, 0, 0});
		
			InventoryStorageSlot slot = inv.FindSuitableSlotForItem(ent);
			
			if (slot == null)
			{
				Print("ESC_StartingBackpackComponent.SpawnItemToBackpack: Can't find slot for " + resource + " in " + inv.ClassName(), LogLevel.ERROR);
				return;
			}
			
			
			Print("ESC_StartingBackPackComponent.SpawnItemToBackpack: Found a slot!");
			slot.AttachEntity(ent);
		}
	}
	
	protected void SpawnStartingItems(IEntity owner)
	{
		const SCR_UniversalInventoryStorageComponent inv = SCR_UniversalInventoryStorageComponent.Cast(owner.FindComponent(SCR_UniversalInventoryStorageComponent));
		if (inv == null)
		{
			Print("ESC_StartingBackpackComponent.SpawnStartingItems: Can't find SCR_UniversalInventoryStorageComponent", LogLevel.ERROR);
			return;
		}
		
		
		const int playerCount = ESC_Utils.PlayerCount();
		
		SpawnItemToBackpack(inv, m_startingPistol, playerCount);
		SpawnItemToBackpack(inv, m_startingMag, playerCount*2);

		Print("ESC_StartingBackpackComponent.SpawnStartingItems: Starting bagpack filled");
	}
	
	 override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
		if(!GetGame().InPlayMode()) return;
    }
	
}