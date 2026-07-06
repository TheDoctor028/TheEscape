//! Periodically ensures a minimum number of patrol groups are active near each player.
//! Patrols are spawned in directional sectors so they spread around the player
//! without clustering, while still having some randomness.
class ESC_PatrolController
{
	//! Min distance from the player to spawn a patrol — far enough not to be seen.
	const static float SPAWN_RANGE_MIN = 500;

	//! Max distance from the player to spawn a patrol.
	const static float SPAWN_RANGE_MAX = 750;

	//! How often (ms) the patrol density check runs.
	const static int CHECK_INTERVAL_MS = 15000;

	//! Number of patrol waypoints in the cycle.
	const static int WAYPOINT_COUNT = 6;

	//! Radius of the patrol cycle around the group spawn point.
	const static float PATROL_RADIUS = 750;
	
	protected ref array<ref ESC_Patrol> m_activePatrols2 = {};
	
	protected ref array<ResourceName> m_patrolGroupResource = {};

	protected int m_desiredPatrolsPerPlayer = 6;

	protected float m_checkRadius = SPAWN_RANGE_MIN + PATROL_RADIUS;

	void ESC_PatrolController(array<ResourceName> patrol)
	{
		m_patrolGroupResource = patrol;
		Print(
			"ESC_PatrolController: Initialized with " + m_patrolGroupResource.Count() + " prefabs, "
			+ m_desiredPatrolsPerPlayer + " desired per player, "
			+ m_checkRadius + "m check radius.",
			LogLevel.NORMAL
		);
	}

	//! Call this to begin periodic patrol management.
	//! An immediate check is also done on start so patrols appear
	//! before players are released.
	void StartPatrolling()
	{
		GetGame().GetCallqueue().CallLater(CheckAndSpawnPatrols, CHECK_INTERVAL_MS, true);
		Print("ESC_PatrolController.StartPatrolling: Patrol manager is running.", LogLevel.NORMAL);
	}

	//! Cleans up deleted patrol groups, then for each player counts nearby
	//! patrols and spawns more if the desired count is not met.
	protected void CheckAndSpawnPatrols()
	{
		RemoveDeadPatrols();
		
		array<ChimeraCharacter> players = ESC_Utils.Players();
		foreach (ChimeraCharacter player : players)
		{
			if (!player)
				continue;

			vector playerPos = player.GetOrigin();
			int nearbyCount = CountNearbyPatrols(playerPos);
			int toSpawn = m_desiredPatrolsPerPlayer - nearbyCount;

			if (toSpawn <= 0)
				continue;

			Print(
				"ESC_PatrolController.CheckAndSpawnPatrols: Player has " + nearbyCount
				+ " nearby patrols, spawning " + toSpawn + " more."
			);
			SpawnPatrolsAroundPosition(playerPos, toSpawn);
		}
	}

	//! Removes patrols whose underlying AI group no longer exists (e.g. killed by a player).
	protected void RemoveDeadPatrols()
	{
		for (int i = m_activePatrols2.Count() - 1; i >= 0; i--)
		{
			ESC_Patrol group = m_activePatrols2.Get(i);
			if (!group || !group.IsAlive())
				m_activePatrols2.Remove(i);
		}
	}
	
	//! Returns how many active patrols are within m_checkRadius of the given position.
	protected int CountNearbyPatrols(vector position)
	{
		int count = 0;
		foreach (ESC_Patrol group : m_activePatrols2)
		{
			if (!group)
				continue;
			if (vector.Distance(group.GetOrigin(), position) <= m_checkRadius)
				count++;
		}
		return count;
	}

	//! Spawns `count` patrol groups around `position`.
	//! The 360 degree space is divided into equal sectors and each group is placed
	//! at a random angle within its own sector. This spreads patrols in all
	//! directions while still feeling natural (not perfectly symmetric).
	protected void SpawnPatrolsAroundPosition(vector position, int count)
	{
		if (m_patrolGroupResource.Count() == 0)
		{
			Print("ESC_PatrolController.SpawnPatrolsAroundPosition: No prefabs configured.", LogLevel.WARNING);
			return;
		}

		float sectorDeg = 360.0 / count;
		// Random base offset so cardinal directions aren't always used.
		float baseAngle = Math.RandomFloat(0.0, 360.0);

		for (int i = 0; i < count; i++)
		{
			// Each group lives in its own sector; jitter within the sector avoids
			// perfect rings while keeping reasonable separation between groups.
			float sectorStart = baseAngle + sectorDeg * i;
			float jitter = Math.RandomFloat(sectorDeg * 0.1, sectorDeg * 0.9);
			float angleDeg = sectorStart + jitter;
			float angleRad = angleDeg * (Math.PI / 180.0);

			float dist = Math.RandomFloat(SPAWN_RANGE_MIN, SPAWN_RANGE_MAX);
			float spawnX = position[0] + dist * Math.Sin(angleRad);
			float spawnZ = position[2] + dist * Math.Cos(angleRad);
			float spawnY = GetGame().GetWorld().GetSurfaceY(spawnX, spawnZ);
			vector spawnPos = Vector(spawnX, spawnY, spawnZ);

			int prefabIdx = Math.RandomInt(0, m_patrolGroupResource.Count() - 1);
			ResourceName prefab = m_patrolGroupResource.Get(prefabIdx);

			if (prefab == "")
			{
				Print(
					"ESC_PatrolController.SpawnPatrolsAroundPosition: Prefab entry " + prefabIdx + " is empty, skipping.",
					LogLevel.WARNING
				);
				continue;
			}
			
			if (GetGame().GetWorld().GetSurfaceY(spawnPos[0], spawnPos[2]) <= 5) continue;

			ESC_Patrol group = ESC_EscapeSpawnManager.SpawnPatrolAroundCoordinate2(
				spawnPos, prefab, WAYPOINT_COUNT, PATROL_RADIUS
			);

			if (group)
			{
				m_activePatrols2.Insert(group);
				Print(
					"ESC_PatrolController.SpawnPatrolsAroundPosition: Patrol spawned at " + spawnPos.ToString()
					+ " (angle=" + angleDeg + "deg, dist=" + dist + "m)",
					LogLevel.DEBUG
				);
			}
		}
	}
}
