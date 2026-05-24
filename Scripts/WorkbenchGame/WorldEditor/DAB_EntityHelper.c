#ifdef WORKBENCH
class DAB_EntityHelper
{
	//------------------------------------------------------------------------------------------------
	static void GetAllChildrenOfType(typename searchType, notnull IEntitySource parent, notnull inout array<IEntitySource> output){
		array<IEntitySource> allEntities = {};
		DAB_EntityHelper.ExtractFromParent(parent, allEntities);

		for(int i = 0; i < allEntities.Count(); i++){
			IEntitySource src = allEntities[i];
			if(! src.GetClassName().ToType().IsInherited(searchType)) continue;
			
			output.Insert(src);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	static void GetAllSelectedEntitiesOfType(typename searchType, notnull inout array<IEntitySource> output)
	{
		array<IEntitySource> allSelected = {};
		DAB_EntityHelper.GetFlattendSelectedHierachy(allSelected);

		for(int i = 0; i < allSelected.Count(); i++){
			IEntitySource src = allSelected[i];
			if(! src.GetClassName().ToType().IsInherited(searchType)) continue;
			
			output.Insert(src);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	static void GetFlattendSelectedHierachy(notnull inout array<IEntitySource> output){
		WorldEditorAPI api = ((WorldEditor)Workbench.GetModule(WorldEditor)).GetApi();
		
		for(int i = 0; i < api.GetSelectedEntitiesCount(); i++){
			IEntitySource parent = api.GetSelectedEntity(i);
			DAB_EntityHelper.ExtractFromParent(parent, output);	
		}
	}
	
	//------------------------------------------------------------------------------------------------
	static void ExtractFromParent(notnull IEntitySource parent,notnull inout array<IEntitySource> output){
		output.Insert(parent);
		
		for(int i = 0; i < parent.GetNumChildren(); i++){
			IEntitySource child = parent.GetChild(i);
			
			if(child.GetNumChildren() == 0){
				output.Insert(child);
			} else {
				DAB_EntityHelper.ExtractFromParent(child, output);
			}
		}
	}
	
	static void SetEntitySelection(array<IEntitySource> newSelection, bool clearSelection = true)
	{
		WorldEditorAPI api = ((WorldEditor)Workbench.GetModule(WorldEditor)).GetApi();
		
		if(clearSelection) api.ClearEntitySelection();
		foreach(IEntitySource src: newSelection)
		{
			api.AddToEntitySelection(src);
		}
	}
}

#endif