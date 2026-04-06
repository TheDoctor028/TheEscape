class ESC_EscapeSpawnManager
{
	static vector GetRandomSpawnInSquare(float minX, float maxX, float minZ, float maxZ)
    {
        // Generate random floats for X and Z within the boundaries
        float randomX = Math.RandomFloat(minX, maxX);
        float randomZ = Math.RandomFloat(minZ, maxZ);
        
        // Construct the vector. Y is hardcoded to 0 as requested.
        vector generatedSpawn = Vector(randomX, 0, randomZ);
        
        Print("Escape Mode: Generated random spawn in square -> " + generatedSpawn.ToString());
        
        return generatedSpawn;
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