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
	
	protected float m_cityWeight = 1;
	
	protected float m_villageWeight = 0.8;
	
	override void OnInitServer()
	{
		GetGame().GetCallqueue().CallLater(InitTowns, 100);
		GetGame().GetCallqueue().CallLater(SpawnActorsToTowns, 500);
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
	
}