//------------------------------------------------------------------------------------------------
//! Class identity for the theatre component.
class ESC_TheatreComponentClass : ESC_ScriptComponentClass { }

//------------------------------------------------------------------------------------------------
//! World-population controller. Drives the ambient theatre of the Escape mode:
//! discovers settlements and military bases via map descriptors, seeds OPFOR
//! patrols into towns and villages with weights from the editor, and spawns a
//! configurable amount of civilian traffic between random villages so the world
//! feels alive before players drop in.
[ComponentEditorProps(category: "Escape/Game", description: "World-population controller: discovers settlements, seeds OPFOR patrols, and spawns civilian traffic.")]
class ESC_TheatreComponent : ESC_ScriptComponent
{
	//! Raw map descriptor entities classified as military bases (no `ESC_Town` wrapping).
	protected ref array<IEntity> m_militaryBases = {};

	//! Settlements discovered on the map by `TownQueryCB` and classified as CITY/TOWN.
	protected ref array<ref ESC_Town> m_cities = {};

	//! Settlements discovered on the map and classified as VILLAGE.
	protected ref array<ref ESC_Town> m_villages = {};

	//! OPFOR prefab pool eligible for spawning in cities.
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "OPFOR Prefabs that can patrol in cities")]
	protected ref array<ResourceName> m_cityPatrolRscs;

	//! OPFOR prefab pool eligible for spawning in villages.
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "OPFOR Prefabs that can patrol in villages")]
	protected ref array<ResourceName> m_villagePatrolRscs;

	//! Probability (0..1) that an OPFOR patrol is created per village.
	[Attribute(uiwidget: UIWidgets.Auto, defvalue: "1.0", desc: "OPFOR present wight in cities")]
	protected float m_cityWeight = 1;

	//! Probability (0..1) that an OPFOR patrol is created per city.
	[Attribute(uiwidget: UIWidgets.Auto, defvalue: "0.8", desc: "OPFOR present wight in villages")]
	protected float m_villageWeight = 0.8;

	//! Relative weight for civilian traffic (currently informational; reserved).
	[Attribute(uiwidget: UIWidgets.Auto, defvalue: "1", desc: "CIV present wight in villages")]
	protected float m_civilTrafficRatio = 1;

	//! Pool of civilian vehicle prefabs used by `SpawnCivilianTraffic`.
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "CIV Prefabs for traffic in the world")]
	protected ref array<ResourceName> m_civilTrafficVehicles;

	//! Civilian character prefab spawned as the driver of each civilian vehicle.
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, defvalue: "{22E43956740A6794}Prefabs/Characters/Factions/CIV/GenericCivilians/Character_CIV_Randomized.et", desc: "CIV Prefabs for the character")]
	protected ResourceName m_civilCharacter = "{22E43956740A6794}Prefabs/Characters/Factions/CIV/GenericCivilians/Character_CIV_Randomized.et";

	//! Number of civilian vehicle cycles spawned during world population.
	protected int m_civTrafficIntensity = 6;

	//------------------------------------------------------------------------------------------------
	//! Server init hook. Schedules `InitTheatre` on a small delay so map entities
	//! are guaranteed available before queries run.
	override void OnInitServer()
	{
		GetGame().GetCallqueue().CallLater(InitTheatre, 100);
	}

	//------------------------------------------------------------------------------------------------
	//! Orchestrator that runs discovery (towns), population (patrols), and traffic.
	//! Called once from the deferred init scheduled by `OnInitServer`.
	protected void InitTheatre()
	{
		InitTowns();
		SpawnActorsToTowns();
		SpawnCivilianTraffic();
		//GetGame().GetCallqueue().CallLater(InitTowns, 100);
		//GetGame().GetCallqueue().CallLater(SpawnActorsToTowns, 500);
	}

	//------------------------------------------------------------------------------------------------
	//! World-query callback. Inspects `SCR_MapDescriptorComponent` instances and
	//! buckets them into cities, villages, or military bases based on `GetBaseType()`.
	//! \param entity Entity returned by the world query.
	//! \return Always true so the query continues exploring.
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

	//------------------------------------------------------------------------------------------------
	//! Scans the world around a fixed map center (4000, 4000) within a 10 km
	//! radius using `QueryEntitiesBySphere`, dispatching to `TownQueryCB` for
	//! every entity. Logs the discovered counts at the end.
	protected void InitTowns()
	{
	    vector mapCenter = "4000 0 4000"; // Adjust to roughly the center of your specific terrain
	    float searchRadius = 10000;       // 10km radius covers most Reforger maps

	    GetGame().GetWorld().QueryEntitiesBySphere(mapCenter, searchRadius, TownQueryCB);

	    Print(string.Format("ESC_TheatreComponent.PopulateTowns: Found %1 villages and %2 cities, %3 military bases on the map!", m_villages.Count(), m_cities.Count(), m_militaryBases.Count()));
	}

	//------------------------------------------------------------------------------------------------
	//! Spawns one randomly-picked OPFOR patrol per village, gated by `m_villageWeight`,
	//! and one per city, both patrolling a random radius around the town origin.
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

	//------------------------------------------------------------------------------------------------
	//! Spawns `m_civTrafficIntensity` civilian vehicles at random villages, gives each
	//! driver an AI group, and assigns a 1-3 hop waypoint cycle between random villages.
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
			
			driver.Activate();
		}
	}

}
