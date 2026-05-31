#ifdef WORKBENCH

[WorkbenchPluginAttribute( name: "Fix Road Mat Sort Settings", description: "Uses Road Sort Order instead of Mat Sort bias in selected Roads", category: "DAB - Misc", wbModules: {"WorldEditor"}, awesomeFontCode: 0xF4DB)]
class DAB_RoadMatSortFixPlugin : WorkbenchPlugin
{
	WorldEditorAPI m_API;
	ref array<IEntitySource> m_allEntities = {};
	
	//------------------------------------------------------------------------------------------------
    override void Run()
    {
		m_API = ((WorldEditor)Workbench.GetModule(WorldEditor)).GetApi();
		if (!m_API)
			return;
		
		GetAllEntities(m_API.GetWorld());

		WBProgressDialog progress = new WBProgressDialog("Fixing Roads..", (WorldEditor)Workbench.GetModule(WorldEditor));
		m_API.BeginEntityAction("Fix Roads");
				
		int changedCount = 0;
		
		for (int i = 0; i < m_allEntities.Count(); i++)
		{
			IEntitySource src = m_allEntities[i];
			
			RoadEntity road = RoadEntity.Cast(m_API.SourceToEntity(src));
			if(!road) {
				Print("Something went wrong when converting a Entity to Road. Skipping...", LogLevel.WARNING);
				progress.SetProgress(i / m_allEntities.Count());
				continue;
			}
			
			
			int matSort;
			src.Get(DAB_Constants.MATSORTBIAS, matSort);
			if(matSort == 0) continue;
			
			src.Set(DAB_Constants.ROADSORT, matSort);
			
			string defaultVal;
			src.GetDefaultAsString(DAB_Constants.MATSORTBIAS, defaultVal);
			src.Set(DAB_Constants.MATSORTBIAS, defaultVal);
			
			changedCount++;
			progress.SetProgress(i / m_allEntities.Count());
		}
		
		m_API.EndEntityAction();
		
		Print("Fixed " + changedCount + "/" + m_allEntities.Count() + " Entities.");
	}
		
	//------------------------------------------------------------------------------------------------
	protected void GetAllEntities(BaseWorld baseWorld)
	{
		m_allEntities.Clear();
		vector minPos, maxPos;
		baseWorld.GetBoundBox(minPos, maxPos);
		baseWorld.QueryEntitiesByAABB(minPos, maxPos, AddEntity, FilterEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected bool AddEntity(IEntity entity)
	{
		if (entity) {
			m_allEntities.Insert(m_API.EntityToSource(entity));
		}
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool FilterEntity(IEntity entity)
	{
		IEntitySource src = m_API.EntityToSource(entity);
		if(! src) return false;
		return src.GetClassName().ToType().IsInherited((typename) RoadEntity);
	}
	
}

#endif // WORKBENCH
