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
	
	//------------------------------------------------------------------------------------------------
	//! Measures how steep the terrain is around `center` by sampling surface heights
	//! in a square grid and returning max - min. Used as a "slope reject" filter.
	//! \param center Center of the sampling region.
	//! \param size Edge length of the sampling region (square).
	//! \param step Spacing between samples (defaults to 1m).
	//! \return Elevation delta (m) inside the sampled square.
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
	
	static bool SpawnVehicleWithDriver(ResourceName vehicleRsc, ResourceName driverRsc, vector pos, out Vehicle vehicle, out ChimeraCharacter driver)
	{
		vehicle = Vehicle.Cast(ESC_Utils.SpawnEntity(vehicleRsc, pos));
		driver = ChimeraCharacter.Cast(ESC_Utils.SpawnEntity(driverRsc, pos + {1, 0, 1})); // Spawn it 1 meter apart to avoid collision
		
		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(
        	driver.FindComponent(SCR_CompartmentAccessComponent)
    	);
		
		VehicleControllerComponent vcc = VehicleControllerComponent.Cast(vehicle.FindComponent(VehicleControllerComponent));
		
		
		if (access)
		{
			vcc.StartEngine();
			return access.MoveInVehicle(vehicle, ECompartmentType.PILOT);
		} else {
			Print("ESC_Utils.SpawnVehicleWithDriver: Faild to get SCR_CompartmentAccessComponent of driver.", LogLevel.ERROR);
			return false;
		}
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
	
	static SCR_AIGroup CreateAIGroupFromUnit(notnull ChimeraCharacter unit)
	{
		// TODO add error handling
		
		SCR_AIGroup group = SCR_AIGroup.Cast(ESC_Utils.SpawnEntity("{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et", vector.Zero));
		SCR_CharacterFactionAffiliationComponent cfac = SCR_CharacterFactionAffiliationComponent.Cast(unit.FindComponent(SCR_CharacterFactionAffiliationComponent));
		
		group.SetFaction(cfac.GetPerceivedFaction());
		group.SetName("ESC_Group_"+UUID.GenV4());
		
		group.AddAIEntityToGroup(unit);

		return group;		
	}
	
	static ResourceName GetRandomRscName(array<ResourceName> items)
	{
		int randI = Math.RandomInt(0, items.Count());
		return items[randI];
	}
	
	static ref ESC_Town GetRandomTown(array<ref ESC_Town> items)
	{
		int randI = Math.RandomInt(0, items.Count());
		return items[randI];
	}
}


