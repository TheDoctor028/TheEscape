enum ESC_ControlledEntityType
{
	START_POINT,
	EXTRATION_POINT,
}

enum ESC_EscapeStatus
{
	PREPARING = 0,
	READY = 1,
	INPROGRESS,
	EXTRACTION,
}

class ESC_EscapeManagerComponentClass : ScriptComponentClass {}

[ComponentEditorProps(category: "Escape/Components", description: "Main Escape component.")]
class ESC_EscapeManagerComponent : ScriptComponent
{
	
	[Attribute(uiwidget: UIWidgets.CheckBox, defvalue: "true", desc: "If enabled then we are gonna generate a random point from the map.")]
	protected bool m_randomStartingPointFromTheMap;
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Patrol group prefabs that can be randomly selected for spawning")]
	protected ref array<ResourceName> m_patrolPrefabs;
	
	protected ResourceName m_prisionGuardPrefab = "{CB58D90EA14430AD}Prefabs/Groups/OPFOR/Group_USSR_SentryTeam.et";

	// Auto registered
	protected ref array<IEntity> m_startPoints = {};
	
	// Auto registered
	protected ref array<IEntity> m_extractionPoints = {};
	
	protected vector m_startingCord = {-1, -1, -1};
	
	protected IEntity m_extractionPoint;
	
	protected SCR_Task m_extractionTask;
	
	protected ESC_EscapeStatus m_escapeStatus = ESC_EscapeStatus.READY;
	
	protected ref map<int, ref ESC_Player> m_players = new map<int, ref ESC_Player>();

	protected ref ESC_PatrolController m_patrolController;
	
	protected FactionKey m_escapingFaction = "US"; // TODO make this config
	
	protected int m_minimumExtrationDistance = 1500;
	
	
	override void EOnInit(IEntity owner)
    {	
		if(!GetGame().InPlayMode()) return;
		
		if (Replication.IsServer()) OnInitServer();
		
		if (Replication.IsClient()) OnInitClient();
    }

    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }
	
	protected void OnInitServer()
	{
		
		SCR_TaskSystem ts = SCR_TaskSystem.GetInstance();
		
		
		m_extractionTask = ts.CreateTask("{B3C3E51AB5662621}ESC_ExtractionTask.et", "", "", "", {6162.398, 165.896, 6426.726});
		
		if (m_extractionTask == null)
		{
			Print("ESC_EscapeManagerComponent.StartEscape: Failed to seup extraction task", LogLevel.ERROR); 
			return;
		}
		
		GetGame().GetCallqueue().CallLater(RegisterPalyers, 1000, true);
	}
	
	
	protected void OnInitClient()
	{
	}
	
	
	protected void RegisterPalyers()
	{
		array<ref ESC_Player> players = ESC_Utils.GetPlayers();
		
		foreach(ref ESC_Player player : players)
		{
			Print("ESC_EscapeManagerComponent.registerPalyers: Got Player with ID: " + player.GetPlayerID(), LogLevel.DEBUG);
			if (m_players.Get(player.GetPlayerID()) == null)
			{				
				m_players.Insert(player.GetPlayerID(), player);
				Print("ESC_EscapeManagerComponent.registerPalyers: Player :" + player.GetName() + " (" + player.GetPlayerID() + ") is registered", LogLevel.NORMAL);
			}
		}
	}
	
	void StartEscapeRcp()
	{
		Rpc(StartEscape);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void StartEscape()
	{
		Print("ESC_EscapeManagerComponent.StartEscape: Starting Escape. Got " + m_extractionPoints.Count() + " extraction points and " + m_startPoints.Count() + " starting points.");
		
		if (m_extractionPoints.Count() == 0)
		{
			Print("ESC_EscapeManagerComponent.StartEscape: No extraction points found.", LogLevel.ERROR);
			return;
		}
		
		if (m_escapeStatus != ESC_EscapeStatus.READY)
		{
			Print("ESC_EscapeManagerComponent.StartEscape: Escape can't started beause the status is not ready.", LogLevel.WARNING);
			return;
		}
		
		const int randomI = Math.RandomInt(0, m_extractionPoints.Count() - 1);
		if (randomI < 0) return;
		
		m_extractionPoint = m_extractionPoints.Get(randomI);
		
		Print("ESC_EscapeManagerComponent.StartEscape: Extraction point index: " + randomI);
		
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		
		if (m_randomStartingPointFromTheMap)
		{
			Print("ESC_EscapeManagerComponent.StartEscape: Getting random spawnpoint from the map");
			m_startingCord = ESC_EscapeSpawnManager.GenerateStartPoint(m_extractionPoint.GetOrigin());
			Print("ESC_EscapeManagerComponent.StartEscape: Starting coordinates: " + m_startingCord);
		} else {
			const int randomStartPointI = Math.RandomInt(0, m_startPoints.Count() - 1);
			if (randomStartPointI < 0) return;
			
			vector startingPoint = m_extractionPoint.GetOrigin();
			while (
				vector.Distance(startingPoint, m_extractionPoint.GetOrigin()) < ESC_EscapeSpawnManager.MinimumExtractionDistance
			)
			{
				startingPoint = m_startPoints.Get(randomStartPointI).GetOrigin();
			}
			
			m_startingCord = startingPoint;
			
			ESC_EscapeSpawnManager.SpawnStartingPrision(m_startingCord);
		}
		
		ESC_Utils.SpawnEntity("{01BA312A286BBCA2}Prefabs/ESC_ExtractionSmokeTrigger.et", m_extractionPoint.GetOrigin());
				
		ESC_EscapeSpawnManager.SpawnPatrolAroundCoordinate(m_startingCord + {10, 0, 0}, m_prisionGuardPrefab, 6, 15);
		
		foreach(ref ESC_Player player : ESC_Utils.GetPlayers() )
		{
			m_extractionTask.SetOrigin(player.GetOrigin());
			//GetGame().GetCallqueue().CallLater(taskSystem.AssignTask, 500, false, m_extractionTask, player.GetTaskExecutor());
			taskSystem.AssignTask(m_extractionTask, player.GetTaskExecutor());
			player.Teleport(m_startingCord);
		}
		

		m_extractionTask.SetOrigin(m_extractionPoint.GetOrigin());
		
		if (m_patrolPrefabs.Count() > 0)
		{
			m_patrolController = new ESC_PatrolController(m_patrolPrefabs);
			m_patrolController.StartPatrolling();
		} else {
			Print("ESC_EscapeManagerComponent.StartEscape: No patrol config set, patrols will not spawn.", LogLevel.WARNING);
		}

		m_escapeStatus = ESC_EscapeStatus.INPROGRESS;
	}
	
	void Register(IEntity entity, ESC_ControlledEntityType t)
	{
		switch(t)
		{
			case ESC_ControlledEntityType.START_POINT:
				m_startPoints.Insert(entity);
				break;
			case ESC_ControlledEntityType.EXTRATION_POINT:
				m_extractionPoints.Insert(entity);
				break;
			default:
				Print("ESC_EscapeManagerComponent.Register: Unknown ESC_ControlledEntityType for registration: " + t, LogLevel.ERROR);
				return;
		}
	}
	
	SCR_Faction GetEscapingFaction()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
	    if (!factionManager)
	        return null;
	
	    SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(m_escapingFaction));
	    if (!faction)
	        return null;
		
		return faction;
	}
	
	void NotifyFaction(FactionKey factionKey, ENotification notification)
	{
	    if (!Replication.IsServer())
	        return;
	
	    SCR_NotificationsComponent.SendToFaction(GetEscapingFaction(), false, notification);
	}
		
	void StartExtraction()
	{
		if (m_escapeStatus == ESC_EscapeStatus.EXTRACTION /* || m_escapeStatus != ESC_EscapeStatus.INPROGRESS*/) return;
		m_escapeStatus = ESC_EscapeStatus.EXTRACTION;
		
		Print("ESC_EscapeManagerComponent.StartExtration: Starting extration phase");
		//NotifyFaction(m_escapingFaction, ENotification.UNKNOWN);
		
		// After 30 s lets end the game for now.
		GetGame().GetCallqueue().CallLater(ESC_Utils.EndGame, 5000);
	}
}