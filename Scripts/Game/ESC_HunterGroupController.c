class ESC_HunterGroupController
{
	protected ref array<ResourceName> m_prefabs = {};
	
	protected ref array<ref ESC_Patrol> m_huneterGroups = {};
	
	protected int m_markerUpdateInterval[2] = {30, 60}; // in min, max s
	
	protected float m_spawnRanges[2] = {800, 600}; // in meters, max, min
	
	protected ref array<ref ESC_Patrol> m_wpUpdateQueue = {};
	
	void ESC_HunterGroupController(array<ResourceName> prefabs)
	{
		m_prefabs = prefabs;
	}
	
	protected vector GetRandomPlayerPos()
	{
		const array<ref ESC_Player> players = ESC_Utils.GetPlayers();
		const int i = Math.RandomInt(0, players.Count());
		const vector playerPos = players.Get(i).GetOrigin();
		
		return playerPos;
	}
	
	protected void Spawn()
	{
		const vector playerPos = GetRandomPlayerPos();
		const ESC_Patrol g = ESC_Patrol(ESC_Utils.GetRandomRscName(m_prefabs), ESC_Utils.GetRandomPositionInCircle(playerPos, m_spawnRanges[0], m_spawnRanges[1]));
		m_huneterGroups.Insert(g);
		
		// Kick off the initial waypoint spawning
		m_wpUpdateQueue.Insert(g);
	}
	
	protected void Handler()
	{
		while(m_wpUpdateQueue.Count() > 0)
		{
			const ref ESC_Patrol g = m_wpUpdateQueue.Get(0);
			m_wpUpdateQueue.Remove(0);

			Print("ESC_HunterGroupController.Handler: Updating wp for " + g.Name());
			const int nextUpdateInt = Math.RandomInt(m_markerUpdateInterval[0], m_markerUpdateInterval[1]) * 1000;
			g.PatrolAttack(GetRandomPlayerPos());
			GetGame().GetCallqueue().CallLater(m_wpUpdateQueue.Insert, nextUpdateInt, false, g);
		}
		
		GetGame().GetCallqueue().CallLater(Handler, 1000, false);
	}

	void Start()
	{
		Spawn();
		GetGame().GetCallqueue().CallLater(Handler, 1000, false);
	}
}