#ifdef WORKBENCH

[WorkbenchPluginAttribute( name: "Print River Lengths", description: "Prints a sorted list of all rivers with their XZ lengths into the console", category: "DAB - Misc", wbModules: {"WorldEditor"}, awesomeFontCode: 0xF773)]
class DAB_PrintRiverLengths : WorkbenchPlugin
{
    override void Run()
    {
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;

		WorldEditorAPI worldEditorAPI = worldEditor.GetApi();
		if (!worldEditorAPI)
			return;
		
		IEntitySource entitySource;
		IEntity entity;
		int editorEntitiesCount = worldEditorAPI.GetEditorEntityCount();
		array<ref DAB_LengthEntry> lengthEntries = {};
		
		for (int i = 0; i < editorEntitiesCount; i++)
		{
			entitySource = worldEditorAPI.GetEditorEntity(i);
			entity = worldEditorAPI.SourceToEntity(entitySource);
			if (!entity.IsInherited(RiverEntity)) continue;
			
			AnalyzeRiverEntity(entity, entitySource, lengthEntries, worldEditorAPI);
		}
		
		PrintSortedList(lengthEntries, worldEditorAPI);
	}
	
	protected void AnalyzeRiverEntity(notnull IEntity entity, notnull IEntitySource entitySource, notnull inout array<ref DAB_LengthEntry> lengthEntries, WorldEditorAPI worldEditorAPI)
	{
		IEntitySource parentSource = entitySource.GetParent();
		if(!parentSource)
		{
			PrintFormat("River %1 has no parent!", Debug.GetEntityLinkString(entity));
			lengthEntries.Insert(new DAB_LengthEntry(entity, -1));
			return;
		}
		
		IEntity parent = worldEditorAPI.SourceToEntity(parentSource);
		ShapeEntity shape = ShapeEntity.Cast(parent);
		if(!shape)
		{
			PrintFormat("Rivers parent %1 is no shape entity!", Debug.GetEntityLinkString(parent));
			lengthEntries.Insert(new DAB_LengthEntry(entity, -1));
			return;
		}
		
		lengthEntries.Insert(new DAB_LengthEntry(entity, GetLengthXZ(shape)));
	}
	
	protected void PrintSortedList(array<ref DAB_LengthEntry> lengthEntries, WorldEditorAPI worldEditorAPI)
	{
		for (int i = 1; i < lengthEntries.Count(); i++)
	    {
	        DAB_LengthEntry entry = lengthEntries[i];
	        int j = i - 1;
	
	        while (j >= 0 && lengthEntries[j].m_fLength < entry.m_fLength)
	        {
	            lengthEntries[j + 1] = lengthEntries[j];
	            j--;
	        }
	        lengthEntries[j + 1] = entry;
	    }
		
		foreach (DAB_LengthEntry entry : lengthEntries)
	    {
	        PrintFormat("Entity: %1 has length of: %2", Debug.GetEntityLinkString(entry.m_Entity), entry.m_fLength);
	    }
	}
	
	protected float GetLengthXZ(ShapeEntity shape)
	{
		array<vector> positions = {};
		shape.GetPointsPositions(positions);
		
		float lengthXZ = 0;
		for (int i = 0; i < positions.Count() - 1; i++)
		{
			int j = (i + 1);
			
			vector currentPoint = positions[i];
			vector nextPoint = positions[j];
			
			lengthXZ += vector.DistanceXZ(currentPoint, nextPoint);
		}
		
		return lengthXZ;
	}
}

class DAB_LengthEntry
{
	IEntity m_Entity;
	float m_fLength = -1;
	
	void DAB_LengthEntry(IEntity entity, float length)
    {
        m_Entity  = entity;
        m_fLength = length;
    }
}

#endif // WORKBENCH
