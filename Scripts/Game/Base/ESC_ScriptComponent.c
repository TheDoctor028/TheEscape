/* QUICK START:
------
class ESC_<REPLACEME>ComponentClass : ESC_ScriptComponentClass {}

[ComponentEditorProps(category: "Escape/Game", description: "<TODO>")]
class ESC_<REPLACEME>Component : ESC_ScriptComponent
{ }
------
*/

//------------------------------------------------------------------------------------------------
//! Class identity for the project's base script component. Use this as the parent
//! for any project component that needs the shared server/client init pattern.
class ESC_ScriptComponentClass : ScriptComponentClass { }

//------------------------------------------------------------------------------------------------
//! Base class for all ESC (Escape) script components.
//! Provides a shared `EOnInit`/`OnPostInit` pattern that splits initialization between
//! server-only and client-only handlers, plus a cached `m_owner` pointer.
//! Components that extend this only need to override `OnInitServer` or `OnInitClient`
//! instead of reproducing the replication branch in every class.
class ESC_ScriptComponent : ScriptComponent
{
	//! Cached owner entity set in `EOnInit` for fast access by subclasses.
	protected IEntity m_owner;

	//------------------------------------------------------------------------------------------------
	//! Engine init entry point. Skips work in the Workbench and dispatches to the
	//! server/client init handlers based on the current replication role.
	//! \param owner Entity this component is attached to; cached into `m_owner`.
	override void EOnInit(IEntity owner)
    {
		if(!GetGame().InPlayMode()) return;

		m_owner = owner;

		if (Replication.IsServer()) OnInitServer();

		if (Replication.IsClient()) OnInitClient();
    }

	//------------------------------------------------------------------------------------------------
	//! Late-init hook used to subscribe to the INIT entity event mask so subclasses
	//! can react once other components have completed their init phase.
	//! \param owner Entity this component is attached to.
    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }

	//------------------------------------------------------------------------------------------------
	//! Server-only init hook. Overridden by subclasses to perform authoritative
	//! setup (spawning, registering, RPC setup). Called from `EOnInit` when running as host.
	protected void OnInitServer();

	//------------------------------------------------------------------------------------------------
	//! Client-only init hook. Overridden by subclasses for visual/UI setup that
	//! should not run on dedicated servers. Called from `EOnInit` when running as client.
	protected void OnInitClient();
}
