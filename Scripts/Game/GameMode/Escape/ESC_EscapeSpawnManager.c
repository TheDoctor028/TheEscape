class ESC_EscapeSpawnManager
{
	
	const static float YLimitForSpawn = 1.5;
	
	const static float SteepnessLimitForSpawn = 2;
	
	const static float MinimumExtractionDistance = 1500;
	
	const static ResourceName m_startingBagpack = "{1BF7A0AE48B36B17}Prefabs/ESC_Starting_Backpack.et";
	
	const static ResourceName m_prision = "{9FB70D958724EF8A}Prefabs/ESC_Prision_1.et";
	
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
	
	static float GetSteepness(vector center, float size, float step = 1.0)
    {
        float half = size * 0.5;
		// There is no max float enum so we are using a "large" number
        float minY = 1000000000000000;
        float maxY = -1000000000000000;

        BaseWorld world = GetGame().GetWorld();

        for (float x = center[0] - half; x <= center[0] + half; x += step)
        {
            for (float z = center[2] - half; z <= center[2] + half; z += step)
            {
                float y = world.GetSurfaceY(x, z);

                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }

        return maxY - minY;
    }
	
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
			GetSteepness(sp, 10) > SteepnessLimitForSpawn ||
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
	
	static void SpawnMarkerAtLocation(vector location, ResourceName prefabPath)
    {
		
		SCR_MapMarkerManagerComponent mmmc = SCR_MapMarkerManagerComponent.GetInstance();
		
		mmmc.InsertStaticMarkerByType(SCR_EMapMarkerType.PLACED_CUSTOM, 5000, 5000, false, -1, 0, true);
		array<SCR_MapMarkerBase> markers = mmmc.GetStaticMarkers();
		
		SCR_MapMarkerBase marker = mmmc.GetStaticMarkerByID(1);
    	marker.SetCustomText("SPAWN");
		
	}
	
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
}