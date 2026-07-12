enum ESC_WaypointType
{
	MOVE,
	PATROL,
	CYCLE,
	ATTACK,
	DEFEND,
}

class ESC_Waypoints
{
	
	static ResourceName GetWaypointResourceName(ESC_WaypointType type)
	{
		switch(type)
		{	
			case ESC_WaypointType.MOVE:
				return "{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et";
			case ESC_WaypointType.PATROL:
				return "{22A875E30470BD4F}Prefabs/AI/Waypoints/AIWaypoint_Patrol.et";
			case ESC_WaypointType.CYCLE:
				return "{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et";
			case ESC_WaypointType.ATTACK:
				return "{1B0E3436C30FA211}Prefabs/AI/Waypoints/AIWaypoint_Attack.et";
			case ESC_WaypointType.DEFEND:
				return "{93291E72AC23930F}Prefabs/AI/Waypoints/AIWaypoint_Defend.et";
		}
		
		return "{49CED34BBCD060F0}Prefabs/AI/Waypoints/AIWaypoint_Base.et";
	}
	
	static Resource GetWaypointResource(ResourceName name)
	{
		return ESC_Utils.LoadResource(name);
	}
	
	static AIWaypoint SpawnWaypoint(ESC_WaypointType type, vector postion)
	{
		AIWaypoint createdWP = AIWaypoint.Cast(ESC_Utils.SpawnEntity(GetWaypointResourceName(type) ,postion));
		
		if (createdWP == null)
		{
			Print("ESC_Waypoints.SpawnWaypoint: Faild to create WP.", LogLevel.ERROR);
			return null;
		}
		
		return createdWP;
	}
	
	static void Cycle(notnull array<vector> positions, notnull SCR_AIGroup group)
	{
		const string name = "";
		if (positions.Count() == 0)
		{
			Print("ESC_Waypoints.WPCycle: positions count must be > 0", LogLevel.ERROR);
			return;
		}
		
		if (positions.Count() == 1)
		{
			positions.InsertAt(positions[0], 0); // Move waypoint at the cycle
			positions.InsertAt(group.GetOrigin(), 0); // Mover wp at the base point
		}
	
		array<AIWaypoint> wps = {};
		
		const UUID wpSeqUUID = UUID.GenV4();
		
		for (int i = 0; i < positions.Count()-1; i++)
		{
			const vector wpPos = positions[i];
			
			AIWaypoint waypoint = ESC_Waypoints.SpawnWaypoint(ESC_WaypointType.PATROL, wpPos);
			
			if (waypoint == null)
			{
				Print("ESC_Waypoints.Cycle: Waypoint is null", LogLevel.ERROR);
				return;
			}
			
			waypoint.SetName(name + "_WP_" + wpSeqUUID + "_" + i);
						
			Print("ESC_Waypoints.Cycle: Spawning patrol waypoint " + i + 
			" at -> " +
			 wpPos.ToString() + "(" + waypoint.GetName() + ")", LogLevel.DEBUG);
			
			wps.Insert(waypoint);
		}
		
		const vector cyclePostion = positions[positions.Count()-1];
		
		// Create a cycle waypoint to add other waypoints
		AIWaypointCycle cycle = AIWaypointCycle.Cast(ESC_Waypoints.SpawnWaypoint(ESC_WaypointType.CYCLE, cyclePostion));
		
		cycle.SetName(name + "_Cycle_" + UUID.GenV4());
		
		Print("ESC_Waypoints.Cycle: Spawning cycle waypoint at -> " +
			 cyclePostion.ToString() + "(" + cycle.GetName() + ")", LogLevel.DEBUG);
		
		cycle.SetWaypoints(wps);
		group.AddWaypoint(cycle);
	}
	
}
