class ESC_Town
{
	protected UUID m_uuid;
	
	protected IEntity m_entity;
	
	protected int heat = 0;
	
	void ESC_Town(IEntity entity)
	{
		m_uuid = UUID.GenV4();
		
		m_entity = entity;
	}
}