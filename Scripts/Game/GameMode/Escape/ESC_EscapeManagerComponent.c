//------------------------------------------------------------------------------------------------
//! Categories used when entities self-register with `ESC_EscapeManagerComponent`.
//! Keeps the manager's `Register` switch simple and editor-driven via combo boxes.
enum ESC_ControlledEntityType
{
	//! Entity marked as a candidate starting point (prison / spawn area).
	START_POINT,
	//! Entity marked as a candidate extraction point reachable from start.
	EXTRATION_POINT,
}

//------------------------------------------------------------------------------------------------
//! State machine for the current round. Lets the rest of the game reason about
//! whether the Escape sequence can be started, is ongoing, or in the final phase.
enum ESC_EscapeStatus
{
	PREPARING = 0,
	READY = 1,
	INPROGRESS,
	EXTRACTION,
}

//------------------------------------------------------------------------------------------------
//! Class identity for the escape manager component.
class ESC_EscapeManagerComponentClass : ScriptComponentClass { }

//------------------------------------------------------------------------------------------------
//! Authoritative conductor of the Escape round. Owns:
//! - the registry of start points and extraction points (populated via `ESC_EntityRegisterComponent`),
//! - the extraction task created at server init,
//! - the per-player `ESC_Player` bookkeeping,
//! - the runtime `ESC_PatrolController` that keeps OPFOR patrols around each player.
//! Game flow: players press the start-escape `ScriptedUserAction` -> the manager
//! picks an extraction point, generates/selects a starting point, deploys the
//! starting props and guard patrol, teleports players, and issues the extraction task.
[ComponentEditorProps(category: "Escape/Components", description: "Main Escape component.")]
class ESC_EscapeManagerComponent : ScriptComponent
{

	//! When true, the manager generates a procedural starting point via
	//! `ESC_EscapeSpawnManager.GenerateStartPoint`; otherwise it picks from registered
	//! `START_POINT` entities.
	[Attribute(uiwidget: UIWidgets.CheckBox, defvalue: "true", desc: "If enabled then we are gonna generate a random point from the map.")]
	protected bool m_randomStartingPointFromTheMap;

	//! Pool of patrol group prefabs used to spin up the runtime `ESC_PatrolController`.
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Patrol group prefabs that can be randomly selected for spawning")]
	protected ref array<ResourceName> m_patrolPrefabs;

	//! Prefab used for the small OPFOR guard spawn placed around the prison.
	protected ResourceName m_prisionGuardPrefab = "{CB58D90EA14430AD}Prefabs/Groups/OPFOR/Group_USSR_SentryTeam.et";

	//------------------------------------------------------------------------------------------------
	// Auto registered
	//! Registered candidate start points (filled by `ESC_EntityRegisterComponent`).
	protected ref array<IEntity> m_startPoints = {};

	//------------------------------------------------------------------------------------------------
	// Auto registered
	//! Registered candidate extraction points (filled by `ESC_EntityRegisterComponent`).
	protected ref array<IEntity> m_extractionPoints = {};

	//! Cached chosen starting coordinate (vector.Zero until `StartEscape` runs).
	protected vector m_startingCord = {-1, -1, -1};

	//! Extraction point picked at the start of the current round.
	protected IEntity m_extractionPoint;

	//! Shared `SCR_Task` representing the extraction objective.
	protected SCR_Task m_extractionTask;

	//! Current phase of the round; used to gate RPCs like `StartEscape`.
	[RplProp()]
	protected ESC_EscapeStatus m_escapeStatus = ESC_EscapeStatus.READY; // TODO This should be preparing after ready up logic is done.

	//! Per-player bookkeeping keyed by the engine player ID.
	protected ref map<int, ref ESC_Player> m_players = new map<int, ref ESC_Player>();

	//! Runtime controller that keeps OPFOR patrols around each player.
	protected ref ESC_PatrolController m_patrolController;

	//! Faction considered "escaping"; current default is US.
	protected FactionKey m_escapingFaction = "US"; // TODO make this config

	//! Minimum distance (m) between the random start point and the picked extraction point.
	protected int m_minimumExtrationDistance = 1500;

	//------------------------------------------------------------------------------------------------
	//! Engine init. Skips the Workbench and dispatches to server/client handlers.
	//! \param owner Entity this component is attached to.
	override void EOnInit(IEntity owner)
    {
		if(!GetGame().InPlayMode()) return;

		if (Replication.IsServer()) OnInitServer();

		if (Replication.IsClient()) OnInitClient();
    }

	//------------------------------------------------------------------------------------------------
	//! Late-init hook. Subscribes the owner to the INIT entity event mask.
	//! \param owner Entity this component is attached to.
    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }

	//------------------------------------------------------------------------------------------------
	//! Server init hook. Builds the shared extraction task and starts the periodic
	//! `RegisterPalyers` loop so late-joiners are tracked.
	protected void OnInitServer()
	{
		GetGame().GetCallqueue().CallLater(RegisterPalyers, 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Client init hook. Intentionally empty.
	protected void OnInitClient()
	{
	}
	
	// TODO-DOCS
	static ESC_EscapeManagerComponent GetInstance()
	{
		// TODO-AI: swap the implementation
		return ESC_Utils.GetManager();
	}

	//------------------------------------------------------------------------------------------------
	//! Periodic (1 s) loop that pulls the latest connected players from
	//! `ESC_Utils.GetPlayers` and adds any missing ones to `m_players`.
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

	//------------------------------------------------------------------------------------------------
	//! Client-callable RPC entry point that fans out to the server-side `StartEscape`.
	void StartEscapeRcp()
	{
		Rpc(StartEscape);
	}

	//------------------------------------------------------------------------------------------------
	//! Server-side RPC handler. Picks an extraction point, settles on a starting
	//! coordinate (procedural or from registered start points), spawns props and
	//! guards, assigns the extraction task, schedules a deferred player teleport
	//! (4 s) via `TeleportPlayersToStart`, and (if configured) starts the patrol
	//! controller. Transitions state from READY -> INPROGRESS.
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

		m_extractionTask = taskSystem.CreateTask("{B3C3E51AB5662621}ESC_ExtractionTask.et", "", "", "", m_extractionPoint.GetOrigin());

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
		
		ESC_Utils.SpawnEntity("{01BA312A286BBCA2}Prefabs/ESC_ExtractionSmokeTrigger.et", m_extractionPoint.GetOrigin());
				
		ESC_EscapeSpawnManager.SpawnPatrolAroundCoordinate(m_startingCord + {10, 0, 0}, m_prisionGuardPrefab, 6, 15);
		
		foreach(ref ESC_Player player : ESC_Utils.GetPlayers())
		{
			taskSystem.AssignTask(m_extractionTask, player.GetTaskExecutor());
		}

		// Give players a short beat before relocating them to the start coordinate.
		GetGame().GetCallqueue().CallLater(TeleportPlayersToStart, 5000);
		

		m_extractionTask.SetOrigin(m_extractionPoint.GetOrigin());
		
		if (m_patrolPrefabs.Count() > 0)
		{
			m_patrolController = new ESC_PatrolController(m_patrolPrefabs);
			m_patrolController.StartPatrolling();
		} else {
			Print("ESC_EscapeManagerComponent.StartEscape: No patrol config set, patrols will not spawn.", LogLevel.WARNING);
		}

		m_escapeStatus = ESC_EscapeStatus.INPROGRESS;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Deferred teleport routine scheduled from `StartEscape`. Relocates every
	//! registered player to the chosen `m_startingCord` after a short delay so the
	//! round setup (props, guards, task) has time to settle before players jump.
	protected void TeleportPlayersToStart()
	{
		foreach(ref ESC_Player player : ESC_Utils.GetPlayers())
		{
			player.Teleport(m_startingCord);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Registers an entity into either the start-point or extraction-point pool,
	//! routing on its declared type. Logs and no-ops for unknown types.
	//! \param entity Entity to register (typically the owner of an `ESC_EntityRegisterComponent`).
	//! \param t Category that determines which pool the entity is added to.
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

	//------------------------------------------------------------------------------------------------
	//! Resolves the configured escaping faction from `m_escapingFaction` via the
	//! faction manager. Returns null when no faction manager or matching faction exists.
	//! \return The escaping `SCR_Faction`, or null if unresolvable.
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

	//------------------------------------------------------------------------------------------------
	//! Broadcasts a notification to the escaping faction. Server-only; ignored on clients.
	//! Currently unused by the gameplay flow but kept for upcoming mission-end UX.
	//! \param factionKey Target faction key (currently informational; the escaping faction is always used).
	//! \param notification Notification ID to broadcast.
	void NotifyFaction(FactionKey factionKey, ENotification notification)
	{
	    if (!Replication.IsServer())
	        return;

	    SCR_NotificationsComponent.SendToFaction(GetEscapingFaction(), false, notification);
	}

	//------------------------------------------------------------------------------------------------
	//! Transitions the round into the EXTRACTION phase and schedules the
	//! victory end-game call (5 s). No-op if already in EXTRACTION.
	void StartExtraction()
	{
		if (m_escapeStatus == ESC_EscapeStatus.EXTRACTION /* || m_escapeStatus != ESC_EscapeStatus.INPROGRESS*/) return;
		m_escapeStatus = ESC_EscapeStatus.EXTRACTION;

		Print("ESC_EscapeManagerComponent.StartExtration: Starting extration phase");
		//NotifyFaction(m_escapingFaction, ENotification.UNKNOWN);

		// After 30 s lets end the game for now.
		GetGame().GetCallqueue().CallLater(ESC_Utils.EndGame, 5000);
	}
	
	
	// TODO-DOCS
	ESC_EscapeStatus GetStatus()
	{
		return m_escapeStatus;
	}
}
