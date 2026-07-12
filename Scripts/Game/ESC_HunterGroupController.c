class ESC_HunterGroupController
{
	protected ref array<ResourceName> m_prefabs = {};
	
	protected ref array<ref ESC_Patrol> m_hunetGroups = {};
	
	void ESC_HunterGroupController()
	{
	
	}

	void Start()
	{
		const array<ref ESC_Player> players = ESC_Utils.GetPlayers();
		const vector playerPos = players.Get(0).GetOrigin();
		
		const ESC_Patrol g = ESC_Patrol(ESC_Utils.GetRandomRscName(m_prefabs), ESC_Utils.GetRandomPositionInCircle(playerPos, 800, 600));
		
	}
}