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
}

class ESC_EscapeManagerComponentClass : ScriptComponentClass {}

[ComponentEditorProps(category: "Escape/Components", description: "Main Escape component.")]
class ESC_EscapeManagerComponent : ScriptComponent
{
	
	[Attribute(uiwidget: UIWidgets.CheckBox, defvalue: "true", desc: "If enabled then we are gonna generate a random point from the map.")]
	protected bool m_randomStartingPointFromTheMap;
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Patrol group prefabs that can be randomly selected for spawning")]
	protected ref array<ResourceName> m_patrolPrefabs ;

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
		
		m_extractionTask =  SCR_Task.Cast(ESC_Utils.SpawnEntity("{B3C3E51AB5662621}ESC_ExtractionTask.et", this.m_extractionPoint.GetOrigin()));
		if (m_extractionTask == null)
		{
			Print("ESC_EscapeManagerComponent.StartEscape: Failed to seup extraction task", LogLevel.ERROR); 
			return;
		}
		
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
		
		ESC_EscapeSpawnManager.SpawnPatrolAroundCoordinate(m_startingCord + {10, 0, 0}, "{CB58D90EA14430AD}Prefabs/Groups/OPFOR/Group_USSR_SentryTeam.et", 6, 15);
		
		foreach(ref ESC_Player player : ESC_Utils.GetPlayers() )
		{
			if (!m_extractionTask.AddTaskAssignee(player.GetTaskExecutor()))
			{
				Print("ESC_EscapeManagerComponent.StartEscape: Failed to assigne extraction task to player: " + player.GetPlayerID(), LogLevel.ERROR);
				continue;
			}
			
			//player.GetPlayer().Teleport(mat);
			player.GetPlayer().SetOrigin(m_startingCord);
		}
		
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
}