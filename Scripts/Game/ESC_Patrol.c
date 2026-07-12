class ESC_Patrol
{

	protected string m_id;
	
	protected string m_name;
	
	protected SCR_AIGroup m_group;
	
	protected ResourceName m_groupResource;
	
	protected bool m_spawned = false;
	
	protected bool m_inPatrol = false;
	
	
	void ESC_Patrol(ResourceName groupResource, vector position = ESC_Utils.neg_one_vector)
	{	
		m_id = "" + UUID.GenV4();
		
		m_name = "ESC_Patrol_" + m_id;
		
		m_groupResource = groupResource;
		
		if (!ESC_Utils.PosVecEQ(position, ESC_Utils.null_vector))
		{
			Spawn(position);
		}
	}
	
	string Name()
	{
		return m_name;
	}

	vector GetOrigin()
	{
		if (!m_group)
			return vector.Zero;

		return m_group.GetOrigin();
	}
	
	// True while the underlying AI group is spawned and still alive.
	// m_group is a weak reference, so it naturally becomes null once the
	// group entity is deleted (e.g. killed by a player).
	bool IsAlive()
	{
		return m_spawned && m_group;
	}
	
	// Spawns the patrol to the given location without waypoints
	bool Spawn(vector position)
	{
		if (m_spawned) return false;
		
        SCR_AIGroup aiGroup = SCR_AIGroup.Cast(ESC_Utils.SpawnEntity(m_groupResource, position));

        if (!aiGroup) return false;
		
		aiGroup.SetName(Name());

        Print("ESC_Patrol.Spawn: " + Name() + " Spawned successfully at -> " + position.ToString());
		
		m_group = aiGroup;
		
		m_spawned = true;
		
		return true;
	}
	
	
	/*
		Creates a patrol waypoint seq for patroling in a circle from r radius from the position
			position - the postion to start a patrol at
			wapointCount - the number of waypoints to have
			radius - the radius of the patrol
			force - true if the group should start this patrol if already in patrol
		Return true if the given patrol has strated false otherwise.
	*/
	bool PatrolCircle(vector position, int waypointCount, int radius, bool force = false)
	{
		if (!m_spawned)
		{
			Print("ESC_Patrol.PatrolCircle: group " + Name() + " is not yet spawned cant start patrol", LogLevel.WARNING);
			return false;
		}
		
		if (m_inPatrol && !force)
		{
			Print("ESC_Patrol.PatrolCircle: group " + Name() + " already in patrol and not forced to start a new.", LogLevel.WARNING);
			return false;
		}
		
		ClearWaypoints();
		
		
		// If we are more then 100 m away from the waypoint lets move there first
		if (vector.Distance(position, m_group.GetOrigin()) > 100)
		{
			AIWaypoint mvWp = ESC_Waypoints.SpawnWaypoint(ESC_WaypointType.MOVE, position);
			m_group.AddWaypoint(mvWp);
		}
		
		float angleStep = (Math.PI * 2) / waypointCount;
		
		array<AIWaypoint> wps = {};
		
		const UUID wpSeqUUID = UUID.GenV4();
		
		for (int i = 0; i < waypointCount; i++)
		{
			float angle = angleStep * i;
			float wpX = position[0] + radius * Math.Sin(angle);
			float wpZ = position[2] + radius * Math.Cos(angle);
			vector wpPos = Vector(wpX, position[1], wpZ);

			AIWaypoint waypoint = ESC_Waypoints.SpawnWaypoint(ESC_WaypointType.PATROL, wpPos);
			
			if (waypoint == null)
			{
				Print("ESC_Patrol.PatrolCircle: Waypoint is null", LogLevel.ERROR);
				return false;
			}
			
			waypoint.SetName(Name() + "_WP_" + wpSeqUUID + "_" + i);
						
			Print("ESC_Patrol.PatrolCircle: Spawning patrol waypoint " + i + 
			" at -> " +
			 wpPos.ToString() + "(" + waypoint.GetName() + ")", LogLevel.DEBUG);
			
			wps.Insert(waypoint);
		}
		
		// Create a cycle waypoint to add other waypoints
		AIWaypointCycle cycle = AIWaypointCycle.Cast(ESC_Waypoints.SpawnWaypoint(ESC_WaypointType.CYCLE, position));
		
		cycle.SetName(Name()+"_Cycle_" + UUID.GenV4());
		
		Print("ESC_Patrol.PatrolCircle: Spawning cycle waypoint at -> " +
			 position.ToString() + "(" + cycle.GetName() + ")", LogLevel.DEBUG);
		
		cycle.SetWaypoints(wps);
		m_group.AddWaypoint(cycle);
		
		m_inPatrol = true;
		
		return true;
	}
	
	/*
	Creates a randomized patrol waypoint seq within a circle of radius r from the position
		position - the center position of the patrol area
		waypointCount - the number of waypoints to generate
		radius - the max radius of the patrol area
		force - true if the group should start this patrol if already in patrol
	Return true if the given patrol has started, false otherwise.
	*/
	bool PatrolRandomInRadius(vector position, int waypointCount, int radius, bool force = false)
	{
		if (!m_spawned)
		{
			Print("ESC_Patrol.PatrolRandomInRadius: group " + Name() + " is not yet spawned cant start patrol", LogLevel.WARNING);
			return false;
		}
		
		if (m_inPatrol && !force)
		{
			Print("ESC_Patrol.PatrolRandomInRadius: group " + Name() + " already in patrol and not forced to start a new.", LogLevel.WARNING);
			return false;
		}
		
		ClearWaypoints();
		
		// If we are more than 100m away from the center, let's move there first
		if (vector.Distance(position, m_group.GetOrigin()) > 100)
		{
			AIWaypoint mvWp = ESC_Waypoints.SpawnWaypoint(ESC_WaypointType.MOVE, position);
			m_group.AddWaypoint(mvWp);
		}
		
		array<AIWaypoint> wps = {};
		const UUID wpSeqUUID = UUID.GenV4();
		
		// Set a minimum radius (20% of max radius) so waypoints don't cluster directly at the center coordinate
		float minRadius = radius * 0.2;
		
		for (int i = 0; i < waypointCount; i++)
		{
			// 1. Pick a random angle between 0 and 360 degrees (in radians)
			float randomAngle = Math.RandomFloat(0, Math.PI * 2);
			
			// 2. Pick a random distance from the center point
			float randomDist = Math.RandomFloat(minRadius, radius);
			
			// 3. Convert polar coordinates to Cartesian (X/Z) world positions
			float wpX = position[0] + randomDist * Math.Sin(randomAngle);
			float wpZ = position[2] + randomDist * Math.Cos(randomAngle);
			
			// Note: Using position[1] assumes your waypoint spawner snaps to terrain height automatically.
			vector wpPos = Vector(wpX, position[1], wpZ);
	
			AIWaypoint waypoint = ESC_Waypoints.SpawnWaypoint(ESC_WaypointType.PATROL, wpPos);
			
			if (waypoint == null)
			{
				Print("ESC_Patrol.PatrolRandomInRadius: Waypoint is null", LogLevel.ERROR);
				return false;
			}
			
			waypoint.SetName(Name() + "_WP_" + wpSeqUUID + "_" + i);
						
			Print("ESC_Patrol.PatrolRandomInRadius: Spawning random patrol waypoint " + i + 
			" at -> " + wpPos.ToString() + "(" + waypoint.GetName() + ")", LogLevel.DEBUG);
			
			wps.Insert(waypoint);
		}
		
		// Create a cycle waypoint so the AI endlessly loops through this random sequence of points
		AIWaypointCycle cycle = AIWaypointCycle.Cast(ESC_Waypoints.SpawnWaypoint(ESC_WaypointType.CYCLE, position));
		
		cycle.SetName(Name() + "_Cycle_" + UUID.GenV4());
		
		Print("ESC_Patrol.PatrolRandomInRadius: Spawning cycle waypoint at -> " +
			 position.ToString() + "(" + cycle.GetName() + ")", LogLevel.DEBUG);
		
		cycle.SetWaypoints(wps);
		m_group.AddWaypoint(cycle);
		
		m_inPatrol = true;
		
		return true;
	}
	
	
	// Removes all the waypoints of the Patrol group
	void ClearWaypoints()
	{
		array<AIWaypoint> wps = {};
		
		int count = m_group.GetWaypoints(wps);
		
		for (int i = 0; i < count; i++)
		{
			m_group.RemoveWaypointAt(i);
			const AIWaypoint wp = AIWaypoint.Cast(wps.Get(i));
			SCR_EntityHelper.DeleteEntityAndChildren(wp);
		}
	}
	
	/*
	Sends the patrol to attack a given position. Clears the current waypoints of
	the patrol and spawns a single Attack waypoint at the given position.
		pos - the position to attack
	*/
	void PatrolAttack(vector pos)
	{
		if (!m_spawned)
		{
			Print("ESC_Patrol.PatrolAttack: group " + Name() + " is not yet spawned cant start attack", LogLevel.WARNING);
			return;
		}
		
		ClearWaypoints();
		
		AIWaypoint waypoint = ESC_Waypoints.SpawnWaypoint(ESC_WaypointType.ATTACK, pos);
		
		if (waypoint == null)
		{
			Print("ESC_Patrol.PatrolAttack: Waypoint is null", LogLevel.ERROR);
			return;
		}
		
		waypoint.SetName(Name() + "_Attack_WP_" + UUID.GenV4());
		
		Print("ESC_Patrol.PatrolAttack: Spawning attack waypoint at -> " +
			 pos.ToString() + "(" + waypoint.GetName() + ")", LogLevel.DEBUG);
		
		m_group.AddWaypoint(waypoint);
		
		m_inPatrol = true;
	}
	
}