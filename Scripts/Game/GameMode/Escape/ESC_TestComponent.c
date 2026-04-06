[ComponentEditorProps(category: "Escape/Debug", description: "Tests our spawn functions on startup.")]
class ESC_EscapeTestComponentClass : ScriptComponentClass {}

class ESC_EscapeTestComponent : ScriptComponent
{
	
    // EOnInit runs exactly once when the entity this is attached to is loaded into the world
    override void EOnInit(IEntity owner)
    {
		if(!GetGame().InPlayMode()) return;
		
        Print("=========================================", LogLevel.WARNING);
        Print("ESCAPE MODE: RUNNING SPAWN TESTS!", LogLevel.WARNING);
        
        // Test Option A: The Square
        vector squareSpawn = ESC_EscapeSpawnManager.GetRandomSpawnInSquare(6164, 6165, 6428, 6429);
        
		ESC_EscapeSpawnManager.SpawnMarkerAtLocation(squareSpawn, "{EC95FBEA75AE409B}Prefabs/Markers/MapMarkerDotCircle.et");
		
		ESC_EscapeSpawnManager.SpawnPatrolAroundCoordinate(squareSpawn, 
		"{06F0C9675883F18A}Prefabs/Groups/OPFOR/KLMK/Group_USSR_ReconTeam.et", 6, 50);
		
        Print("=========================================", LogLevel.WARNING);
    }

    // You MUST include this in Enfusion to tell the engine that this component 
    // actually wants to use the EOnInit function.
    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }
}
