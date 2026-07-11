//------------------------------------------------------------------------------------------------
//! Classification of an `ESC_Town` by its size, used to pick dominant patrolling
//! behaviours (e.g. heavier OPFOR activity in CITY, lighter in VILLAGE).
enum ESC_Town_Size
{
	CITY = 0,
	TOWN,
	VILLAGE
}

//------------------------------------------------------------------------------------------------
//! Lightweight value type that wraps a single map descriptor entity as a "town".
//! Stores the underlying entity (for `GetOrigin` lookups), a generated UUID, and
//! the size classification. Currently exposes only `GetOrigin`; future fields
//! like `heat` and per-town activity tracking are reserved for upcoming work.
class ESC_Town
{
	//! Stable UUID assigned at construction time; used for tagging patrols/logs.
	protected UUID m_uuid;

	//! The map descriptor entity this town object represents.
	protected IEntity m_entity;

	//! Reserved field for future per-town threat/activity tracking.
	protected int heat = 0;

	//! Size classification of this town.
	protected ESC_Town_Size m_size;

	//------------------------------------------------------------------------------------------------
	//! Builds a new town wrapper around `entity` with the given `size`. A fresh
	//! UUID is generated for log/patrol correlation.
	//! \param entity Map descriptor entity (typically an SCR map icon).
	//! \param size Size classification of this town.
	void ESC_Town(IEntity entity, ESC_Town_Size size)
	{
		m_uuid = UUID.GenV4();

		m_entity = entity;
		m_size = size;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the world-space position of the wrapped descriptor entity.
	//! \return Origin vector of the underlying entity.
	vector GetOrigin()
	{
		return m_entity.GetOrigin();
	}
}
