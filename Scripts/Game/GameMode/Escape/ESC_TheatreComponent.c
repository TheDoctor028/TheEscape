class ESC_TheatreComponentClass : ESC_ScriptComponentClass {}

[ComponentEditorProps(category: "Escape/Game", description: "<TODO>")]
class ESC_TheatreComponent : ESC_ScriptComponent
{
	protected ref array<IEntity> m_militaryBases = {};
	
	protected ref array<ref ESC_Town> m_cities = {};
	
	protected ref array<ref ESC_Town> m_villages = {};
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "OPFOR Prefabs that can patrol in cities")]
	protected ref array<ResourceName> m_cityPatrolRscs;
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "OPFOR Prefabs that can patrol in villages")]
	protected ref array<ResourceName> m_villagePatrolRscs;
	
	[Attribute(uiwidget: UIWidgets.Auto, defvalue: "1.0", desc: "OPFOR present wight in cities")]
	protected float m_cityWeight = 1;
	
	[Attribute(uiwidget: UIWidgets.Auto, defvalue: "0.8", desc: "OPFOR present wight in villages")]
	protected float m_villageWeight = 0.8;
	
	[Attribute(uiwidget: UIWidgets.Auto, defvalue: "1", desc: "CIV present wight in villages")]
	protected float m_civilTrafficRatio = 1;
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "CIV Prefabs for traffic in the world")]
	protected ref array<ResourceName> m_civilTrafficVehicles;
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, defvalue: "{22E43956740A6794}Prefabs/Characters/Factions/CIV/GenericCivilians/Character_CIV_Randomized.et", desc: "CIV Prefabs for the character")]
	protected ResourceName m_civilCharacter = "{22E43956740A6794}Prefabs/Characters/Factions/CIV/GenericCivilians/Character_CIV_Randomized.et";
	
	protected int m_civTrafficIntensity = 6;
	
	override void OnInitServer()
	{
		GetGame().GetCallqueue().CallLater(InitTheatre, 100);
	}
	
	protected void InitTheatre()
	{
		InitTowns();
		SpawnActorsToTowns();
		SpawnCivilianTraffic();
		//GetGame().GetCallqueue().CallLater(InitTowns, 100);
		//GetGame().GetCallqueue().CallLater(SpawnActorsToTowns, 500);
	}
	
	protected bool TownQueryCB(IEntity entity)
	{
	    SCR_MapDescriptorComponent mapDesc = SCR_MapDescriptorComponent.Cast(entity.FindComponent(SCR_MapDescriptorComponent));
		if (mapDesc != null) 
		{
			auto str = mapDesc.ToString();
			auto baseT = mapDesc.GetBaseType();
			auto unitT = mapDesc.GetUnitType();
			auto groupT = mapDesc.GetGroupType();
			
			switch(baseT)
			{
				case ESC_SCR_MapDescriptorComponent_Types.TOWN:
					m_cities.Insert(new ESC_Town(entity, ESC_Town_Size.TOWN));
					break;
				case ESC_SCR_MapDescriptorComponent_Types.CITY:
					m_cities.Insert(new ESC_Town(entity, ESC_Town_Size.CITY));
					break;
				case ESC_SCR_MapDescriptorComponent_Types.VILLAGE:
					m_villages.Insert(new ESC_Town(entity, ESC_Town_Size.VILLAGE));
					break;
				case ESC_SCR_MapDescriptorComponent_Types.MILITARY_BASE:
					m_militaryBases.Insert(entity);
					break;
			}
		}
		
		return true; // True to continue query
	}
	
	
	protected void InitTowns()
	{
	    vector mapCenter = "4000 0 4000"; // Adjust to roughly the center of your specific terrain
	    float searchRadius = 10000;       // 10km radius covers most Reforger maps
		
	    GetGame().GetWorld().QueryEntitiesBySphere(mapCenter, searchRadius, TownQueryCB);
	    
	    Print(string.Format("ESC_TheatreComponent.PopulateTowns: Found %1 villages and %2 cities, %3 military bases on the map!", m_villages.Count(), m_cities.Count(), m_militaryBases.Count()));
	}
	
	protected void SpawnActorsToTowns()
	{
		
		foreach (ESC_Town town : m_villages)
		{
			if (Math.RandomFloat(0, 1) > m_villageWeight) continue;
			const int rnd = Math.RandomInt(0, m_villagePatrolRscs.Count());
			
			ESC_Patrol p = new ESC_Patrol(m_villagePatrolRscs.Get(rnd), ESC_Utils.GetOnGround(town.GetOrigin()));
			p.PatrolRandomInRadius(p.GetOrigin(), 8, 400);
		}
		
		foreach (ESC_Town town : m_cities)
		{
			const int rnd = Math.RandomInt(0, m_cityPatrolRscs.Count());
			
			ESC_Patrol p = new ESC_Patrol(m_cityPatrolRscs.Get(rnd), ESC_Utils.GetOnGround(town.GetOrigin()));
			p.PatrolRandomInRadius(p.GetOrigin(), 8, 600);
		}
	}
	
	protected void SpawnCivilianTraffic()
	{	
		for(int i = 0; i < m_civTrafficIntensity; i++)
		{
			ESC_Town start = ESC_Utils.GetRandomTown(m_villages);
			
			Vehicle vehicle;
			ChimeraCharacter driver;
			
			ESC_Utils.SpawnVehicleWithDriver(
			ESC_Utils.GetRandomRscName(m_civilTrafficVehicles), m_civilCharacter, start.GetOrigin(), vehicle, driver);
			
			SCR_AIGroup ai = ESC_Utils.CreateAIGroupFromUnit(driver);
			
			array<vector> trail = {};
			
			const int maxJ = Math.RandomInt(1, 4);
			
			for(int j = 0; j < maxJ; j++)
			{
				trail.Insert(ESC_Utils.GetRandomTown(m_villages).GetOrigin());
			}
			
			ESC_Waypoints.Cycle(trail, ai);
		}
	}
	
}