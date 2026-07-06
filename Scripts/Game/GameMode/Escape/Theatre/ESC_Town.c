enum ESC_Town_Size
{
	CITY = 0,
	TOWN,
	VILLAGE
}

class ESC_Town
{
	protected UUID m_uuid;
	
	protected IEntity m_entity;
	
	protected int heat = 0;
	
	protected ESC_Town_Size m_size;
	
	void ESC_Town(IEntity entity, ESC_Town_Size size)
	{
		m_uuid = UUID.GenV4();
		
		m_entity = entity;
		m_size = size;
	}
	
	vector GetOrigin()
	{
		return m_entity.GetOrigin();
	}
}