class DAB_ShapeHelper
{
	//------------------------------------------------------------------------------------------------
	static void ModifyPolyline(IEntitySource entSrc, array<vector> points)
	{
		WorldEditorAPI api = ((WorldEditor)Workbench.GetModule(WorldEditor)).GetApi();
		
		BaseContainerList oldPoints = entSrc.GetObjectArray("Points");
		
		// TODO: Find out why it crashes? BI bug?
		if(oldPoints.Count() <= 2)
			Print("Modifying polyline with 2 or less points. That could lead to a crash when readding point. (Reasons unknown)", LogLevel.WARNING);
		
		for(int i = (oldPoints.Count() - 1); i >= 0; i--)
		{
			api.RemoveObjectArrayVariableMember(entSrc, null, "Points", i);
		}

		foreach (int i, vector point : points)
		{
			api.CreateObjectArrayVariableMember(entSrc, null, "Points", "ShapePoint", i);
			api.SetVariableValue(entSrc, { new ContainerIdPathEntry("Points", i) }, "Position", string.Format("%1 %2 %3", point[0], point[1], point[2]));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Used after Modifying a spline/shape with a road on it, so the height adjustment still works.
	//TODO: Improve this hacky 'solution'
	static void FixTerrainAdjustmentGenerators(notnull IEntitySource parent){
		WorldEditorAPI api = ((WorldEditor)Workbench.GetModule(WorldEditor)).GetApi();
		
		array<IEntitySource> allRoadsGenerators = {};
		array<IEntitySource> allLineTerrainShapers = {};
		
		DAB_EntityHelper.GetAllChildrenOfType((typename) RoadGeneratorEntity, parent, allRoadsGenerators);
		DAB_EntityHelper.GetAllChildrenOfType((typename) SCR_LineTerrainShaperGeneratorBaseEntity, parent, allLineTerrainShapers);
		
		allRoadsGenerators.InsertAll(allLineTerrainShapers);
		
		for (int i = 0; i < allRoadsGenerators.Count(); i++)
		{
			IEntitySource childSrc = allRoadsGenerators[i];
			
			RoadGeneratorEntity childRoadEntity = RoadGeneratorEntity.Cast(api.SourceToEntity(childSrc));
			if (childRoadEntity)
			{
				childRoadEntity._WB_OnCreate(parent);
				Print("Fixed road entity on adjusted shape.");
				return;
			}
			
			SCR_LineTerrainShaperGeneratorBaseEntity childLineShapeEntity = SCR_LineTerrainShaperGeneratorBaseEntity.Cast(api.SourceToEntity(childSrc));
			if (childLineShapeEntity)
			{
				childLineShapeEntity._WB_OnCreate(parent);
				Print("Fixed LineTerrainShaper entity on adjusted shape.");
				return;
			}
			
			
			Print("Child was neither road nor lineTerrainShaper!", LogLevel.WARNING);
		}
	}
}

