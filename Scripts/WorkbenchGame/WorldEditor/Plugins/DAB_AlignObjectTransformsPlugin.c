#ifdef WORKBENCH

[WorkbenchPluginAttribute( name: "Copy Transform", description: "Copies transform data to selected objects", category: "DAB - Misc", shortcut: "Ctrl+Shift+W", wbModules: {"WorldEditor"}, awesomeFontCode: 0xF24D)]
class AlignObjectTransformsToLastSelectedPlugin : WorkbenchPlugin
{	
    override void Run()
    {
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
			return;
		
		api.BeginEntityAction();

		int entityCount = api.GetSelectedEntitiesCount();
		if(entityCount <= 1){
			api.EndEntityAction();
			return;
		}
			
		IEntitySource lastSelectedSource = api.GetSelectedEntity(entityCount - 1);

		vector angles, lastPos;
		lastSelectedSource.Get(DAB_Names.COORDS, lastPos);
		lastSelectedSource.Get(DAB_Names.ANGLES, angles);
		
		int placementMode;
		
		for (int i, count = (entityCount - 1); i < count; i++)
		{
			IEntitySource entitySource = api.GetSelectedEntity(i);
			api.BeginEditSequence(entitySource);
			
			//Rotation
			api.SetVariableValue(entitySource, {}, DAB_Names.ANGLES, angles.ToString(false));

			//Placement
			lastSelectedSource.Get(DAB_Names.PLACEMENT, placementMode);
			api.SetVariableValue(entitySource, {}, DAB_Names.PLACEMENT, placementMode.ToString());
			
			//Flags - Relative_Y
			EntityFlags lastFlags, newFlags;
			lastSelectedSource.Get(DAB_Names.FLAGS, lastFlags);
			entitySource.Get(DAB_Names.FLAGS, newFlags);

			if(lastFlags & EntityFlags.RELATIVE_Y) {
				newFlags |= EntityFlags.RELATIVE_Y;
			} else {
				newFlags &= ~EntityFlags.RELATIVE_Y;
			}

			//Flags
			//api.SetVariableValue(entitySource, null, DAB_Names.FLAGS, newFlags.ToString());
			
			//Position
			api.SetVariableValue(entitySource, {}, DAB_Names.COORDS, lastPos.ToString(false));
			api.EndEditSequence(entitySource);
		}
		
		api.SetEntitySelection( api.GetSelectedEntity(0));
		
		api.EndEntityAction();
	}
}

#endif // WORKBENCH
