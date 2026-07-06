class ESC_Player
{
	protected int m_playerID;
	protected int m_entityID;
	protected ChimeraCharacter m_chimera;
	protected string m_name;
	
	void ESC_Player(int playerID)
	{
		const PlayerManager pm = GetGame().GetPlayerManager();
		IEntity player = pm.GetPlayerControlledEntity(playerID);
		if (player != null)
		{
			m_playerID = playerID;
			m_entityID = player.GetID();
			m_chimera = ChimeraCharacter.Cast(player);
			m_name = pm.GetPlayerName(playerID);
		} else {
			Print("ESC_Player: Player is null with ID: " + playerID, LogLevel.WARNING);
		}
	}
	
	int GetPlayerID()
	{
		return m_playerID;
	}
	
	int GetEntityID()
	{
		return m_entityID;
	}
	
	PlayerController GetPlayerController()
	{
		return GetGame().GetPlayerManager().GetPlayerController(m_playerID);
	}
	
	SCR_EditableCharacterComponent GetEditableCharacterComponent()
	{
		return SCR_EditableCharacterComponent.Cast(m_chimera.FindComponent(SCR_EditableCharacterComponent));
	}
	
	ref ChimeraCharacter GetPlayer()
	{
		return m_chimera;
	}
	

	// TODO remove me
	ref IEntity GetEntity()
	{
		return m_chimera;
	}
	
	string GetName()
	{
		return m_name;
	}
	
	SCR_TaskExecutor GetTaskExecutor()
	{
		return SCR_TaskExecutor.FromPlayerID(m_playerID);
	}
	
	void Teleport(vector position)
	{
		SCR_EditableCharacterComponent character = GetEditableCharacterComponent();
		vector trans[4];
		character.GetTransform(trans);
		trans[3] = position;
		character.SetTransform(trans);
	}
}

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

enum ESC_SCR_MapDescriptorComponent_Types 
{
	CITY = 59,
	VILLAGE = 60,
	TOWN,
	SETTLEMENT,
	MILITARY_BASE = 79
}

class ESC_Utils
{
	
	static vector neg_one_vector = {-1, -1, -1};
	
	static vector null_vector = {0, 0, 0};
	
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
	
	static int PlayerCount()
	{
		
		const PlayerManager pm = GetGame().GetPlayerManager();
		
		array<int> playerIds = {};
		
		const int playerCount = pm.GetPlayers(playerIds);
		
		return playerCount;
	}
	
	static array<ChimeraCharacter> Players()
	{
		
		const PlayerManager pm = GetGame().GetPlayerManager();
		
		array<int> playerIds = {};
		
		const int playerCount = pm.GetPlayers(playerIds);
		
		array<ChimeraCharacter> players = {};
		
		foreach(int pId : playerIds)
		{
			IEntity player = pm.GetPlayerControlledEntity(pId);
			if (player == null)
			{
				Print("ESC_Utils.Players: Player with id: '" + pId + "' is null", LogLevel.WARNING);
				continue;
			}
			
			players.Insert(ChimeraCharacter.Cast(player));
		}
		
		return players;
	}
	
	static array<int> PlayerIds()
	{
		
		const PlayerManager pm = GetGame().GetPlayerManager();
		
		array<int> playerIds = {};
		
		const int playerCount = pm.GetPlayers(playerIds);
		
		array<int> res = {};	
		
		
		foreach(int pId : playerIds)
		{
			const IEntity player = pm.GetPlayerControlledEntity(pId);
			if (player == null)
			{
				Print("ESC_Utils.Players: Player with id: '" + pId + "' is null", LogLevel.WARNING);
				continue;
			}
			
			res.Insert(pId);
		}
		
		return res;
	}
	
	static array<ref ESC_Player> GetPlayers()
	{
		const PlayerManager pm = GetGame().GetPlayerManager();
		
		array<int> playerIds = {};
		
		const int playerCount = pm.GetPlayers(playerIds);
		
		array<ref ESC_Player> players = {};
		
		foreach(int pId : playerIds)
		{
			const IEntity playerEnt = pm.GetPlayerControlledEntity(pId);
			if (playerEnt == null)
			{
				Print("ESC_Utils.GetPlayers: Player with id: '" + pId + "' is null", LogLevel.WARNING);
				continue;
			}
			
			ref ESC_Player player = new ESC_Player(pId);

			players.Insert(player);
		}
		
		return players;
	}
	
	static bool PosVecEQ(vector a, vector b, int dim = 3)
	{
		for (int i = 0; i < dim; i++)
		{
			if (a[i] != b[i]) return false;
		}
		
		return true;
	}

	
	static void EndGame()
	{
		SCR_BaseGameMode bgm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		SCR_Faction faction = GetManager().GetEscapingFaction();
		
		bgm.EndGameMode(SCR_GameModeEndData.CreateSimple(EGameOverTypes.VICTORY, -1, factionManager.GetFactionIndex(faction)));
	}
	
	static ESC_EscapeManagerComponent GetManager()
	{
		return ESC_EscapeManagerComponent.Cast(GetGame().GetGameMode().FindComponent(ESC_EscapeManagerComponent));
	}
	
	static vector GetOnGround(vector v)
	{
		const float y = GetGame().GetWorld().GetSurfaceY(v[0], v[2]);
		
		return {v[0], y, v[2]};
	}
	
	static void Teleport(notnull IEntity entity, vector transform)
	{
		 vector previousOrigin = entity.GetOrigin();

	    BaseGameEntity baseGameEntity = BaseGameEntity.Cast(entity);
	    if (baseGameEntity && !BaseVehicle.Cast(baseGameEntity))
	    {
	        baseGameEntity.Teleport(transform);
	    }
	    else
	    {
	        entity.SetWorldTransform(transform);
	    }
	
	    Physics physics = entity.GetPhysics();
	    if (physics)
	    {
	        physics.SetVelocity(vector.Zero);
	        physics.SetAngularVelocity(vector.Zero);
	    }
	
	    RplComponent replication = baseGameEntity.GetRplComponent();
	    if (replication)
	        replication.ForceNodeMovement(previousOrigin);
	
	    if (!ChimeraCharacter.Cast(entity))
	        entity.Update();
		}
	}
	
	
}


