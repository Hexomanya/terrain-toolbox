#ifdef WORKBENCH

[WorkbenchPluginAttribute( name: "Switch Relative_Y", description: "Switch the Relative_Y flag and placement (WARNING)", category: "DAB - Misc", shortcut: "Ctrl+Shift+Q", wbModules: {"WorldEditor"}, awesomeFontCode: 0xF24D)]
class DAB_SwitchRelativeY : WorkbenchPlugin
{	
    override void Run()
    {
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;

		WorldEditorAPI worldEditorAPI = worldEditor.GetApi();
		if (!worldEditorAPI)
			return;
		
		worldEditorAPI.BeginEntityAction();

		int entityCount = worldEditorAPI.GetSelectedEntitiesCount();
		if(entityCount <= 0){
			worldEditorAPI.EndEntityAction();
			return;
		}
	
		for (int i; i < entityCount; i++)
		{
			EntityFlags oldFlags;
			int placementMode;
			IEntitySource entitySource = worldEditorAPI.GetSelectedEntity(i);
			
			entitySource.Get(DAB_Constants.FLAGS, oldFlags);
			bool isRelativeSwitchedOn = oldFlags & EntityFlags.RELATIVE_Y;
			
			if(isRelativeSwitchedOn) {
				oldFlags &= ~EntityFlags.RELATIVE_Y;
				placementMode = DAB_Constants.NONE;
			} else {
				oldFlags |= EntityFlags.RELATIVE_Y;
				placementMode = DAB_Constants.SLOPEANDCONTACT;
			}
			
			//Placement
			worldEditorAPI.SetVariableValue(entitySource, null, DAB_Constants.PLACEMENT, placementMode.ToString());

			//Flags
			worldEditorAPI.SetVariableValue(entitySource, null, DAB_Constants.FLAGS, oldFlags.ToString());
		}

		Print("The Switch Relative_Y plugin currently only works if the object has the default Relative_Y = false. This is a bug on bohemia side.", LogLevel.WARNING);

		worldEditorAPI.EndEntityAction();
	}
}

#endif // WORKBENCH
