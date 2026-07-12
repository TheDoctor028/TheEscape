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
	
	vector GetOrigin()
	{
		return m_chimera.GetOrigin();
	}
}
