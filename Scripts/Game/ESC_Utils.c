enum ESC_WaypointType
{
	MOVE,
	PATROL,
	CYCLE,
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
	
}


class ESC_Utils
{
	static Resource LoadResource(ResourceName name)
	{
		Resource rsrc = Resource.Load(name);
		
		if (!rsrc.IsValid())
		{
			Print("ESC_Util.LoadResource: Faild to load resoruce: " + name, LogLevel.ERROR);
			return null;
		}
	
		return rsrc;
	}
	
	/*
	TODO
	*/
	static IEntity SpawnEntity(ResourceName name, vector position)
	{
		Resource res = Resource.Load(name);
        if (!res.IsValid())
        {
            Print("ESC_Util.SpawnEntity: Invalid entity prefab path!", LogLevel.ERROR);
            return null;
        }

        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.TransformMode = ETransformMode.WORLD;
        spawnParams.Transform[3] = position;

        IEntity ent = GetGame().SpawnEntityPrefab(res, null, spawnParams);

        if (!ent) return null;
		
		Print("ESC_Util.SpawnEntity: Entity succesfully spawned.", LogLevel.DEBUG);
		
		return ent;
	}
}


