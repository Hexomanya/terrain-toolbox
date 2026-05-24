#ifdef WORKBENCH

[WorkbenchPluginAttribute( name: "Align into Z Axis line", description: "Moves objects onto same z axis", category: "DAB - Misc", shortcut: "Shift+K", wbModules: {"WorldEditor"}, awesomeFontCode: 0xF24D)]
class DAB_AlignObjectsAlongAxis : WorkbenchPlugin
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
		if(entityCount <= 1){
			worldEditorAPI.EndEntityAction();
			return;
		}
			
		IEntitySource lastSelectedSource = worldEditorAPI.GetSelectedEntity(entityCount - 1);

		vector lastPos;
		lastSelectedSource.Get(DAB_Names.COORDS, lastPos);
		
		for (int i, count = (entityCount - 1); i < count; i++)
		{
			
			IEntitySource entitySource = worldEditorAPI.GetSelectedEntity(i);

			vector entityPos;
			entitySource.Get(DAB_Names.COORDS, entityPos);
			
			entityPos[2] = lastPos[2];

			//Position
			worldEditorAPI.SetVariableValue(entitySource, {}, DAB_Names.COORDS, entityPos.ToString(false));
		}
		
		worldEditorAPI.SetEntitySelection( worldEditorAPI.GetSelectedEntity(0));
		
		worldEditorAPI.EndEntityAction();
	}
}

#endif // WORKBENCH
