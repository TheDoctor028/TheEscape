//------------------------------------------------------------------------------------------------
//! Static helper namespace that owns the spawn rules and prefab paths used by the
//! Escape game mode (starting positions, extraction setups, OPFOR patrols).
//! The class itself is never instantiated: every entry point is a `static` utility.
class ESC_EscapeSpawnManager
{

	//! Minimum surface height (m) accepted for a generated starting point.
	const static float YLimitForSpawn = 1.5;

	//! Maximum elevation delta (m) accepted within a 10m sample area.
	const static float SteepnessLimitForSpawn = 2;

	//! Minimum distance (m) between starting point and extraction point.
	const static float MinimumExtractionDistance = 1500;

	//! Prefab of the shared backpack dropped at the starting point.
	const static ResourceName m_startingBagpack = "{8B62240F1337A67E}Prefabs/ESC_Starting_Backpack_2.et";

	//! Prefab of the prison prop placed at the starting point.
	const static ResourceName m_prision = "{9FB70D958724EF8A}Prefabs/ESC_Prision_1.et";

	//------------------------------------------------------------------------------------------------
	//! Builds a random world-space vector inside the requested square on the XZ plane.
	//! Y is always 0; callers are expected to look up the surface elevation themselves.
	//! \param minX Lower X bound (inclusive).
	//! \param maxX Upper X bound (exclusive).
	//! \param minZ Lower Z bound (inclusive).
	//! \param maxZ Upper Z bound (exclusive).
	//! \return Randomly generated spawn vector with Y = 0.
	static vector GetRandomSpawnInSquare(float minX, float maxX, float minZ, float maxZ)
    {
        // Generate random floats for X and Z within the boundaries
        float randomX = Math.RandomFloat(minX, maxX);
        float randomZ = Math.RandomFloat(minZ, maxZ);

        // Construct the vector. Y is hardcoded to 0 as requested.
        vector generatedSpawn = Vector(randomX, 0, randomZ);

        Print("ESC_EscapeSpawnManager.GetRandomSpawnInSquare: Generated random spawn in square -> " + generatedSpawn.ToString());

        return generatedSpawn;
    }

	//------------------------------------------------------------------------------------------------
	//! Generates a valid escape starting point: rejects positions whose surface is
	//! below `YLimitForSpawn`, whose surroundings are steeper than `SteepnessLimitForSpawn`,
	//! or whose distance to the extraction point is below `MinimumExtractionDistance`.
	//! Loops until a valid spot is found, then spawns the prison + backpack there.
	//! \param extractionPosition World position of the extraction point used for distance tests.
	//! \return Final accepted spawn vector (with the surface height baked into Y).
	static vector GenerateStartPoint(vector extractionPosition)
	{

		/*
		TODOs:
		 - WE need something to check if we are not spawning in to props like tress or house etc.
		 - Get minimum distance from the start postiion.
		*/

		Print("ESC_EscapeSpawnManager.GenerateStartPoint: Generating random starting location.", LogLevel.NORMAL);
		vector sp = {-1, -1, -1};
		float y = 0;

		while(
			vector.Distance(sp, {-1, -1, -1}) == 0 ||
			 y <= this.YLimitForSpawn ||
			ESC_Utils.GetSteepness(sp, 10) > SteepnessLimitForSpawn ||
			vector.Distance(sp, extractionPosition) < MinimumExtractionDistance
		)
		{
			// TODO get size from some where
			sp = GetRandomSpawnInSquare(0, 11500, 0, 12300);
			y = GetGame().GetWorld().GetSurfaceY(sp[0], sp[2]);

			Print("ESC_EscapeSpawnManager.GenerateStartPoint: sp :" + sp + "; y: " + y);
		}

		// Set the correct height so we dont spawn in the ground.
		sp[1] = y;

		SpawnStartingPrision(sp);

		return sp;
	}

	//------------------------------------------------------------------------------------------------
	//! Drops the prison prop and starting backpack at `sp`, then requests a
	//! navmesh rebuild in a 50x10x50 box so AI can immediately navigate the area.
	//! \param sp Starting point chosen by `GenerateStartPoint` (or a designer).
	static void SpawnStartingPrision(vector sp)
	{
		Print("ESC_EscapeSpawnManager.SpawnStartingPrision: Got statin postion: " + sp, LogLevel.NORMAL);

		IEntity prision = ESC_Utils.SpawnEntity(m_prision, sp);

		IEntity starterBagpack = ESC_Utils.SpawnEntity(m_startingBagpack, sp);

		const vector sizeDiff = {25, 10, 25};
		const vector minSp = sp - sizeDiff;
		const vector maxSp = sp + sizeDiff;

		GetGame().GetAIWorld().RequestNavmeshRebuild(minSp, maxSp);
	}

	//------------------------------------------------------------------------------------------------
	//! Resolves a world marker by name and returns its origin. Falls back to
	//! "0 0 0" when the marker is missing, logging an error.
	//! \param markerName Name of the world entity to look up.
	//! \return Origin vector, or "0 0 0" if the marker was not found.
    static vector GetSpawnFromWorldMarker(string markerName)
    {
        // Search the world for an entity with this exact name
        IEntity spawnMarker = GetGame().GetWorld().FindEntityByName(markerName);

        if (spawnMarker)
        {
            vector markerPos = spawnMarker.GetOrigin();
            Print("Escape Mode: Found marker " + markerName + " at -> " + markerPos.ToString());
            return markerPos;
        }

        // Fallback if you typed the name wrong or forgot to place it
        Print("Escape Mode ERROR: Could not find spawn marker named: " + markerName, LogLevel.ERROR);
        return "0 0 0";
    }

	//------------------------------------------------------------------------------------------------
	//! Inserts a custom static "SPAWN" marker into the world map UI. Intended as a
	//! debug/test helper; the arguments beyond `location` are currently hard-coded.
	//! \param location World position of the new marker (unused; the manager ignores it today).
	//! \param prefabPath Prefab path of the marker entity (unused; reserved for future work).
	static void SpawnMarkerAtLocation(vector location, ResourceName prefabPath)
    {

		SCR_MapMarkerManagerComponent mmmc = SCR_MapMarkerManagerComponent.GetInstance();

		mmmc.InsertStaticMarkerByType(SCR_EMapMarkerType.PLACED_CUSTOM, 5000, 5000, false, -1, 0, true);
		array<SCR_MapMarkerBase> markers = mmmc.GetStaticMarkers();

		SCR_MapMarkerBase marker = mmmc.GetStaticMarkerByID(1);
    	marker.SetCustomText("SPAWN");

	}

	//------------------------------------------------------------------------------------------------
	//! Spawns a patrol group prefab at `centerPoint` and builds a circular cycle of
	//! waypoints around it for the group to loop through. The legacy entry point used
	//! by the manager's per-prison guard spawn.
	//! \param centerPoint World position to spawn the AI group at.
	//! \param groupPrefab Prefab of the SCR_AIGroup to spawn.
	//! \param waypointCount Number of perimeter waypoints to create.
	//! \param patrolRadius Radius (m) of the patrol circle.
	//! \return The spawned `SCR_AIGroup`, or null on spawn/waypoint failure.
    static SCR_AIGroup SpawnPatrolAroundCoordinate(vector centerPoint, ResourceName groupPrefab, int waypointCount, float patrolRadius)
    {

		UUID groupID = UUID.GenV4();

        SCR_AIGroup aiGroup = SCR_AIGroup.Cast(ESC_Utils.SpawnEntity(groupPrefab, centerPoint));

        if (!aiGroup) return null;

		aiGroup.SetName("PatrolGroup_" + groupID);

        Print("ESC_EscapeSpawnManager.SpawnPatrolAroundCoordinate: " + aiGroup.GetName() + " Spawned successfully at -> " + centerPoint.ToString());

		float angleStep = (Math.PI * 2) / waypointCount;

		array<AIWaypoint> wps = {};

		for (int i = 0; i < waypointCount; i++)
		{
			float angle = angleStep * i;
			float wpX = centerPoint[0] + patrolRadius * Math.Sin(angle);
			float wpZ = centerPoint[2] + patrolRadius * Math.Cos(angle);
			vector wpPos = Vector(wpX, centerPoint[1], wpZ);

			AIWaypoint waypoint = ESC_Waypoints.SpawnWaypoint(ESC_WaypointType.PATROL, wpPos);

			if (waypoint == null)
			{
				Print("ESC_EscapeSpawnManager.SpawnPatrolAroundCoordinate: Waypoint is null", LogLevel.ERROR);
				return null;
			}

			waypoint.SetName(aiGroup.GetName() + "_WP" + i);

			Print("ESC_EscapeSpawnManager.SpawnPatrolAroundCoordinate: Spawning patrol waypoint " + i +
			" at -> " +
			 wpPos.ToString() + "(" + waypoint.GetName() + ")");

			wps.Insert(waypoint);
		}

		// Create a cycle waypoint to add other waypoints
		AIWaypointCycle cycle = AIWaypointCycle.Cast(ESC_Waypoints.SpawnWaypoint(ESC_WaypointType.CYCLE, centerPoint));

		cycle.SetName(aiGroup.GetName()+"_Cycle");

		Print("ESC_EscapeSpawnManager.SpawnPatrolAroundCoordinate: Spawning cycle waypoint at -> " +
			 centerPoint.ToString() + "(" + cycle.GetName() + ")");

		cycle.SetWaypoints(wps);
		aiGroup.AddWaypoint(cycle);
        return aiGroup;
    }

	//------------------------------------------------------------------------------------------------
	//! Spawns an `ESC_Patrol` at `centerPoint` and assigns a circular patrol pattern.
	//! Preferred over `SpawnPatrolAroundCoordinate` because it returns the higher-level
	//! `ESC_Patrol` wrapper that integrates with the controller.
	//! \param centerPoint World position to spawn the patrol at.
	//! \param groupPrefab Prefab of the SCR_AIGroup managed by the new patrol.
	//! \param waypointCount Number of perimeter waypoints to create.
	//! \param patrolRadius Radius (m) of the patrol circle.
	//! \return The new `ESC_Patrol`, or null if the underlying group failed to spawn.
	 static ESC_Patrol SpawnPatrolAroundCoordinate2(vector centerPoint, ResourceName groupPrefab, int waypointCount, float patrolRadius)
    {
		ESC_Patrol patrol = new ESC_Patrol(groupPrefab);

		if (!patrol.Spawn(centerPoint))
		{
			Print("ESC_EscapeSpawnManager.SpawnPatrolAroundCoordinate2: Patrol group failed to spawn, discarding.", LogLevel.WARNING);
			return null;
		}

		patrol.PatrolCircle(centerPoint, waypointCount, patrolRadius, true);

        return patrol;
    }
}
