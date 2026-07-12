//------------------------------------------------------------------------------------------------
//! Class identity for the roadblock manager component.
class ESC_RoadblockManagerComponentClass : ESC_ScriptComponentClass { }

//------------------------------------------------------------------------------------------------
//! Procedural roadblock generator component. Nested as a child of
//! `ESC_TheatreComponent` on the GameMode entity, and owns all of its own config
//! (prefab pools, count, distances, wide-road threshold) as editor attributes.
//! Scatters roadblock prefabs across the loaded map by snapping random points to
//! the nearest road via the `RoadNetworkManager`, then orienting each block
//! perpendicular to the road. Map-agnostic: reads the world bounding box at
//! runtime, and skips candidates too close to the military bases the theatre
//! already discovers. Self-driving: `OnInitServer` defers `SpawnRoadblocks` past
//! the theatre's 100 ms town query so those bases are available when it runs.
[ComponentEditorProps(category: "Escape/Game", description: "Procedural roadblock generator: snaps prefabs to the road network across the active map. Add as a child of ESC_TheatreComponent.")]
class ESC_RoadblockManagerComponent : ESC_ScriptComponent
{
	//! Roadblock prefabs randomly picked for each spawn. Required.
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Roadblock prefabs randomly placed across roads")]
	protected ref array<ResourceName> m_roadblockPrefabs;

	//! Optional pool used on roads wider than `m_wideRoadThreshold`. Falls back to the standard pool.
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Roadblock prefabs for wide roads (optional)")]
	protected ref array<ResourceName> m_wideRoadPrefabs;

	//! AI group prefabs randomly picked to defend each spawned roadblock. Optional;
	//! when empty no defenders are spawned and roadblocks stand undefended.
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "AI group prefabs spawned to defend each roadblock")]
	protected ref array<ResourceName> m_roadblockDefenderPrefabs;

	//! How many roadblocks to place across the map.
	[Attribute(uiwidget: UIWidgets.Auto, defvalue: "12", desc: "Number of roadblocks to generate")]
	protected int m_roadblockCount;

	//! Max distance (m) from a random point to the nearest road before the hit is discarded.
	[Attribute(uiwidget: UIWidgets.Auto, defvalue: "400", desc: "Max distance (m) from a random point to the nearest road")]
	protected float m_maxRoadSearchDistance;

	//! Min distance (m) a roadblock must keep from discovered military bases.
	[Attribute(uiwidget: UIWidgets.Auto, defvalue: "200", desc: "Min distance (m) from military bases")]
	protected float m_minDistanceFromBases;

	//! Roads wider than this (m) use `m_wideRoadPrefabs`; narrower roads use `m_roadblockPrefabs`.
	[Attribute(uiwidget: UIWidgets.Auto, defvalue: "6", desc: "Road width (m) above which the wide-road prefab pool is used")]
	protected float m_wideRoadThreshold;

	//! Delay (ms) before spawning, sized to land after the theatre's 100 ms `InitTowns` query.
	protected const int SPAWN_DELAY_MS = 250;

	//! Safety multiplier so scarce road networks don't spin the loop forever.
	protected const int MAX_ATTEMPTS_MULTIPLIER = 32;

	//! Roadblocks spawned this round, kept for cleanup/debug by other systems.
	protected ref array<IEntity> m_spawnedRoadblocks = {};

	//! Defender patrols spawned this round, kept for cleanup/debug by other systems.
	protected ref array<ref ESC_Patrol> m_spawnedDefenders = {};

	//------------------------------------------------------------------------------------------------
	//! Singleton accessor resolved through the theatre (the roadblock component
	//! is a child of `ESC_TheatreComponent` on the GameMode entity).
	//! \return This component, or null if the theatre or this component is missing.
	static ESC_RoadblockManagerComponent GetInstance()
	{
		ESC_TheatreComponent theatre = ESC_TheatreComponent.GetInstance();
		if (!theatre)
			return null;

		return ESC_RoadblockManagerComponent.Cast(theatre.FindComponent(ESC_RoadblockManagerComponent));
	}

	//------------------------------------------------------------------------------------------------
	//! Server init hook. Defers `SpawnRoadblocks` so the theatre's military-base
	//! discovery (scheduled at 100 ms) has completed before we query roads.
	override void OnInitServer()
	{
		GetGame().GetCallqueue().CallLater(SpawnRoadblocks, SPAWN_DELAY_MS);
	}

	//------------------------------------------------------------------------------------------------
	//! Orchestrator: resolves the road network and world bounds, then spawns up to
	//! `m_roadblockCount` roadblocks. Aborts early if the map exposes no road
	//! network or no prefabs are configured.
	protected void SpawnRoadblocks()
	{
		if (!m_roadblockPrefabs || m_roadblockPrefabs.Count() == 0)
		{
			Print("ESC_RoadblockManagerComponent.SpawnRoadblocks: No roadblock prefabs configured, skipping.", LogLevel.WARNING);
			return;
		}

		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
		{
			Print("ESC_RoadblockManagerComponent.SpawnRoadblocks: No SCR_AIWorld available, aborting.", LogLevel.ERROR);
			return;
		}

		RoadNetworkManager roadManager = aiWorld.GetRoadNetworkManager();
		if (!roadManager)
		{
			Print("ESC_RoadblockManagerComponent.SpawnRoadblocks: No RoadNetworkManager available, aborting.", LogLevel.WARNING);
			return;
		}

		BaseWorld world = GetGame().GetWorld();
		vector minBox, maxBox;
		world.GetBoundBox(minBox, maxBox);

		int spawned = 0;
		int attempts = 0;
		int maxAttempts = m_roadblockCount * MAX_ATTEMPTS_MULTIPLIER;

		while (spawned < m_roadblockCount && attempts < maxAttempts)
		{
			attempts++;

			// 1. Random point inside the active map's bounding box, snapped to the ground.
			float randX = Math.RandomFloat(minBox[0], maxBox[0]);
			float randZ = Math.RandomFloat(minBox[2], maxBox[2]);
			vector searchPos = ESC_Utils.GetOnGround(Vector(randX, 0, randZ));

			// 2. Snap to the nearest road; discard hits that are too far from any road.
			BaseRoad foundRoad;
			float distance;
			roadManager.GetClosestRoad(searchPos, foundRoad, distance);

			if (!foundRoad || distance > m_maxRoadSearchDistance)
				continue;

			// 3. Sample the road spline and pick a random segment along it.
			array<vector> roadPoints = {};
			foundRoad.GetPoints(roadPoints);
			if (roadPoints.Count() < 2)
				continue;

			// Math.RandomInt is inclusive on both ends, so the upper bound is
			// Count() - 2 to keep index + 1 inside the array.
			int index = Math.RandomInt(0, roadPoints.Count() - 2);
			vector spawnPos = roadPoints.Get(index);
			vector nextPos = roadPoints.Get(index + 1);

			// Degenerate segment (two identical points) - skip to avoid a zero direction.
			if (vector.Distance(spawnPos, nextPos) < 0.001)
				continue;

			// 4. Keep clear of military bases discovered by the theatre.
			if (!IsFarFromMilitaryBases(spawnPos))
				continue;

			// 5. Yaw perpendicular to the road direction. Flip the sign if the prefab's
			// long axis ends up parallel to the road instead of across it.
			vector roadDir = nextPos - spawnPos;
			roadDir[1] = 0;
			roadDir.Normalize();
			float yaw = Math.Atan2(roadDir[0], roadDir[2]) * Math.RAD2DEG;

			// 6. Pick a prefab (wide roads may use a dedicated pool) and spawn it.
			ResourceName prefab = PickRoadblockPrefab(foundRoad.GetWidth());
			if (prefab == "")
				continue;

			IEntity roadblock = ESC_Utils.SpawnEntityRotated(prefab, spawnPos, yaw);
			if (roadblock)
			{
				m_spawnedRoadblocks.Insert(roadblock);
				spawned++;
				Print(string.Format("ESC_RoadblockManagerComponent: Spawned %1/%2 at %3 (width %4m)",
					spawned, m_roadblockCount, spawnPos.ToString(), foundRoad.GetWidth()), LogLevel.DEBUG);
	
				SpawnRoadblockDefender(spawnPos);
			}
			
			ESC_Utils.NavmeshRebuild(spawnPos, {50, 30, 50});
		}

		Print(string.Format("ESC_RoadblockManagerComponent.SpawnRoadblocks: Done. %1/%2 roadblocks after %3 attempts.",
			spawned, m_roadblockCount, attempts), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Spawns a random defender AI group (wrapped in an ESC_Patrol) at `position` and
	//! assigns a Defend waypoint on the roadblock. No-op when no defender prefabs are
	//! configured, so roadblock generation still works without defenders.
	//! \param position World position of the roadblock to defend.
	protected void SpawnRoadblockDefender(vector position)
	{
		if (!m_roadblockDefenderPrefabs || m_roadblockDefenderPrefabs.Count() == 0)
		{
			Print("ESC_RoadblockManagerComponent.SpawnRoadblockDefender: No defender prefabs configured, skipping.", LogLevel.WARNING);
			return;
		}

		ResourceName defenderPrefab = m_roadblockDefenderPrefabs.GetRandomElement();
		if (defenderPrefab == "")
		{
			Print("ESC_RoadblockManagerComponent.SpawnRoadblockDefender: Picked empty prefab, skipping.", LogLevel.WARNING);
			return;
		}

		vector groundPos = ESC_Utils.GetOnGround(position);
		ESC_Patrol patrol = new ESC_Patrol(defenderPrefab, groundPos);

		if (!patrol.IsAlive())
		{
			Print("ESC_RoadblockManagerComponent.SpawnRoadblockDefender: Defender group failed to spawn, discarding.",
				LogLevel.WARNING);
			return;
		}

		patrol.PatrolDefend(groundPos);
		m_spawnedDefenders.Insert(patrol);

		Print(string.Format("ESC_RoadblockManagerComponent: Defender patrol %1 defending roadblock at %2",
			patrol.Name(), groundPos.ToString()), LogLevel.DEBUG);
	}

	//------------------------------------------------------------------------------------------------
	//! Returns a prefab for the roadblock, preferring the wide-road pool for broad
	//! roads and falling back to the standard pool. Returns "" if nothing is configured.
	//! \param roadWidth Width (m) of the candidate road from `BaseRoad.GetWidth`.
	//! \return A prefab resource name, or "" if no pool is populated.
	protected ResourceName PickRoadblockPrefab(float roadWidth)
	{
		if (roadWidth > m_wideRoadThreshold && m_wideRoadPrefabs && m_wideRoadPrefabs.Count() > 0)
			return m_wideRoadPrefabs.GetRandomElement();

		if (m_roadblockPrefabs && m_roadblockPrefabs.Count() > 0)
			return m_roadblockPrefabs.GetRandomElement();

		return "";
	}

	//------------------------------------------------------------------------------------------------
	//! Returns true when `pos` is far enough from every military base discovered by
	//! the theatre component. When no theatre (or no bases) are present the filter is skipped.
	//! \param pos Candidate roadblock position.
	//! \return True if the position is clear of military bases.
	protected bool IsFarFromMilitaryBases(vector pos)
	{
		ESC_TheatreComponent theatre = ESC_TheatreComponent.GetInstance();
		if (!theatre)
			return true;

		array<IEntity> bases = theatre.GetMilitaryBases();
		if (!bases || bases.Count() == 0)
			return true;

		foreach (IEntity militaryBase : bases)
		{
			if (vector.Distance(pos, militaryBase.GetOrigin()) < m_minDistanceFromBases)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the roadblocks spawned this round, for cleanup/debug by other systems.
	//! \return Array of spawned roadblock entities (may be empty).
	array<IEntity> GetSpawnedRoadblocks()
	{
		return m_spawnedRoadblocks;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the defender patrols spawned this round, for cleanup/debug by other systems.
	//! \return Array of spawned defender patrols (may be empty).
	array<ref ESC_Patrol> GetSpawnedDefenders()
	{
		return m_spawnedDefenders;
	}
}
