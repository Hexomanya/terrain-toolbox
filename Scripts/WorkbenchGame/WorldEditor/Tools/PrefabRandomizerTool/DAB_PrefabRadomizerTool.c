#ifdef WORKBENCH
enum DAB_SelectionType {
	SELECTED_LAYER,
	RADIUS,
}


[WorkbenchToolAttribute(name: "Prefab Randomizer",description: "Let's you quickly replace basic prefabs with detailed randomized ones.",wbModules: {"WorldEditor"}, awesomeFontCode: 0xF074)]
class DAB_PrefabRadomizerTool : WorldEditorTool
{
	//------------------ Configuration ---------------------------------------------------------------
	[Attribute(defvalue: "", desc: "Randomizer Config", category: "Configuration", params: "conf")]
	protected ref DAB_PrefabRandomizerConfig m_config;
	
	//------------------ Selection -------------------------------------------------------------------
	[Attribute(defvalue: "0", desc: "How you want to select the objects", category: "Selection", uiwidget: UIWidgets.ComboBox,  enums: ParamEnumArray.FromEnum(DAB_SelectionType))]
	protected DAB_SelectionType m_selectionType;
	
	[Attribute(defvalue: "100", desc: "The selection radius from the camera", category: "Selection", uiwidget: UIWidgets.Slider , params: "0 500 1", )]
	protected float m_selectionRadius;
	
	[Attribute(defvalue: "false", desc: "Keep already selected entities selected", category: "Selection", uiwidget: UIWidgets.CheckBox, )]
	protected bool m_keepSelection;
	
	[Attribute(defvalue: "true", desc: "Also select objects that are defined as replacer",  category: "Selection", uiwidget: UIWidgets.CheckBox,)]
	protected bool m_selectReplacers;
	
	//------------------ Local Variables -------------------------------------------------------------
	protected BaseWorld m_World;
	protected const ref array<IEntitySource> m_tempQueryInsert = {};
	protected ref map<string, int> m_prefabConfigIndexMap = new map<string, int>();
	protected ref array<int> m_placeInOrderCounters = {};
	
	//------------------------------------------------------------------------------------------------
	protected override void OnActivate()
	{
		m_World = m_API.GetWorld();
		if (!m_World) {
			Print("There is not world loaded!", LogLevel.ERROR);
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected override void OnDeActivate()
	{
		m_World = null;
	}
	
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Randomize Selected")]
	protected void RandomizeSelected()
	{
		if(m_config == null){
			Print("You have to create a config!", LogLevel.WARNING);
			return;
		}
		
		m_prefabConfigIndexMap.Clear();
		m_placeInOrderCounters.Clear();
		for(int i; i < m_config.m_replacementEntries.Count(); i++) // Init counter array
		{
			m_placeInOrderCounters.Insert(0);
		}
		
		array<IEntitySource> entitiesToReplace = {};
		GetAllEntitiesToReplace(entitiesToReplace);
				
		m_API.BeginEntityAction("Prefab Randomization");
		
		array<IEntitySource> sourcesToDelete = {};
		array<IEntitySource> finalEntities = {};
		
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		WBProgressDialog progress = new WBProgressDialog("Randomizing...?", worldEditor);
		float prevProgress, currProgress;
		
		
		for (int i; i < entitiesToReplace.Count(); i++)
		{
			RandomizeEntity(entitiesToReplace[i], finalEntities, sourcesToDelete);
			
			currProgress = i / entitiesToReplace.Count();
			if (currProgress - prevProgress >= 0.01)	// min 1%
			{
				progress.SetProgress(currProgress);		// expensive
				prevProgress = currProgress;
			}
		}
		
		m_API.DeleteEntities(sourcesToDelete);
		
		m_API.EndEntityAction();
		
		DAB_EntityHelper.SetEntitySelection(finalEntities);
	}
	
	//------------------------------------------------------------------------------------------------
	private void GetAllEntitiesToReplace(notnull inout array<IEntitySource> output)
	{
		array<IEntitySource> selection = {};
		DAB_EntityHelper.GetFlattendSelectedHierachy(selection);
	
		output.Clear();	
		
		foreach(IEntitySource src: selection)
		{
			if(!src) continue;
			AddToArrayIfPrefab(src, output);
		}
	}
	
	
	//------------------------------------------------------------------------------------------------
	private void RandomizeEntity(notnull IEntitySource originalSrc, notnull inout array<IEntitySource> finalEntities, notnull inout array<IEntitySource> sourcesToDelete)
	{
		EntityPrefabData prefabData = m_API.SourceToEntity(originalSrc).GetPrefabData();
		if(prefabData == null) return;
		ResourceName objPrefabName = prefabData.GetPrefabName();
		
		int configIndex;
		if(! m_prefabConfigIndexMap.Find(objPrefabName, configIndex)) return;
		
		DAB_PrefabRandomizerConfigEntry entry = m_config.m_replacementEntries[configIndex];
		
		//--- Randomization Probability ---
		if(Math.RandomFloatInclusive(0,1) > entry.m_randomizeProbability)
		{
			finalEntities.Insert(originalSrc);
			return;
		}
		
		//--- Weights ---
		array<float> saveWeights = {};
		saveWeights.Copy(entry.m_replacerWeights);
		for(int i = 0; i < entry.m_replacerPrefabs.Count(); i++)
		{
			if(i < (saveWeights.Count() - 1)) continue;
			
			saveWeights.Insert(1);
		}
		
		//--- Replacement Selection ---
		int index; 
		if(!m_config.m_replaceInOrder)
		{
			index = SCR_ArrayHelper.GetWeightedIndex(saveWeights, Math.RandomFloat01());
		} else 
		{
			index = m_placeInOrderCounters[configIndex];
			m_placeInOrderCounters.Set(configIndex, (index + 1) % entry.m_replacerPrefabs.Count());
		}
		
		ResourceName replacementName = entry.m_replacerPrefabs[index];
		
		//--- Replace ---
		ReplaceEntity(originalSrc, replacementName, finalEntities, sourcesToDelete);
	}
	
	//------------------------------------------------------------------------------------------------
	// Stolen and modified from ReplacerTool
	private void ReplaceEntity(notnull IEntitySource originalSrc, ResourceName replacementName, notnull inout array<IEntitySource> finalEntities, notnull inout array<IEntitySource> sourcesToDelete)
	{
		int entityLayerId = originalSrc.GetLayerID();

		BaseContainer ancestor = originalSrc.GetAncestor();
		if (!ancestor)
		{
			finalEntities.Insert(originalSrc);
			return;
		}
			
		IEntity originalEntity  = m_API.SourceToEntity(originalSrc);
		vector coords = originalEntity.GetOrigin();
		
		IEntitySource parentSource = originalSrc.GetParent();
		if (parentSource)
			coords = SCR_BaseContainerTools.GetLocalCoords(parentSource, coords);
		
		vector worldPos = coords;
		if (parentSource)
			worldPos = m_API.SourceToEntity(parentSource).CoordToParent(coords);
		
		//Only used for creation. Gets overridden later
		vector angles;
		originalSrc.Get(DAB_Names.ANGLES, angles);
		
		string name = originalEntity.GetName();
		originalEntity.SetName(string.Empty); // prevents conflict

		// --- Creating new object --- using CreateEntity and not CreateEntityExt for exact placement
		IEntitySource createdEntitySource = m_API.CreateEntity(replacementName, name, entityLayerId, parentSource, coords, angles);
		
		// --- Copy Position 
		if (m_API.SourceToEntity(createdEntitySource).GetFlags() & EntityFlags.RELATIVE_Y)
		{
			float terrainY = m_World.GetSurfaceY(worldPos[0], worldPos[2]);

			vector finalCoords;
			if (parentSource)
				finalCoords = coords;
			else
				finalCoords = worldPos;

			finalCoords[1] = finalCoords[1] - terrainY;
			m_API.SetVariableValue(createdEntitySource, null, DAB_Names.COORDS, finalCoords.ToString(false));
		}
		
		// --- Copy Scale
		string scale;
		originalSrc.Get(DAB_Names.SCALE, scale);
		if(scale != string.Empty) //Scale returns empty when it is left at 1 for some reason
			m_API.SetVariableValue(createdEntitySource, {}, DAB_Names.SCALE, scale);
		
		// --- Copy over Relative Y setting
		EntityFlags lastFlags, newFlags;
		originalSrc.Get(DAB_Names.FLAGS, lastFlags);
		createdEntitySource.Get(DAB_Names.FLAGS, newFlags);
		
		if(lastFlags & EntityFlags.RELATIVE_Y) {
			newFlags |= EntityFlags.RELATIVE_Y;
		} else {
			newFlags &= ~EntityFlags.RELATIVE_Y;
		}
		
		m_API.SetVariableValue(createdEntitySource, {}, DAB_Names.FLAGS, (newFlags).ToString());
		
		// --- Copy over placement mode
		string placementMode;
		originalSrc.Get(DAB_Names.PLACEMENT, placementMode);
		m_API.SetVariableValue(createdEntitySource, {}, DAB_Names.PLACEMENT, placementMode);

		/*
		//TODO: Runs into problems when replacing a replacer(that has children)
		IEntitySource childSource;
		// move children to new parent
		for (int j = 0, childrenCount = originalSrc.GetNumChildren(); j < childrenCount; j++)
		{
			childSource = originalSrc.GetChild(j);
			if (!childSource)
				continue;

			m_API.ParentEntity(originalSrc, childSource, true);
		}
		*/

		m_API.SetVariableValue(createdEntitySource, {}, DAB_Names.ANGLES, string.Format("%1 %2 %3", angles[0], angles[1], angles[2]));
		
		finalEntities.Insert(createdEntitySource);
		sourcesToDelete.Insert(originalSrc);
	}
	

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Select Entities")]
	protected void SelectEntities()
	{
		if(m_config == null){
			Print("You have to create a config!", LogLevel.WARNING);
			return;
		}
		
		if(!m_keepSelection) m_API.ClearEntitySelection();
		
		m_prefabConfigIndexMap.Clear();
		array<IEntitySource> entitiesToReplace = {};
		
		DAB_EntityHelper.GetFlattendSelectedHierachy(entitiesToReplace);
		GetEntitySelection(entitiesToReplace);
		
		DAB_EntityHelper.SetEntitySelection(entitiesToReplace);
	}
	
	//------------------------------------------------------------------------------------------------
	private void GetEntitySelection(notnull inout array<IEntitySource> selection)
	{		
		switch(m_selectionType)
		{
			case DAB_SelectionType.SELECTED_LAYER: 
				GetSelectionFromLayer(selection);
				break;
			
			case DAB_SelectionType.RADIUS: 
				GetSelectionFromRadius(selection);
				break;

			default:
				Print("You have to select a selection type first!", LogLevel.WARNING);
				return;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	private void GetSelectionFromLayer(notnull inout array<IEntitySource> selection)
	{		
		vector minPos, maxPos;
		m_World.GetBoundBox(minPos, maxPos);
		m_World.QueryEntitiesByAABB(minPos, maxPos, InsertEntity);
		
		int currentLayerId = m_API.GetCurrentEntityLayerId();
		
		foreach(IEntitySource src: m_tempQueryInsert)
		{
			if(!src) continue;
			if(src.GetLayerID() != currentLayerId) continue;
			
			AddToArrayIfPrefab(src, selection);
		}
		
		m_tempQueryInsert.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	private void GetSelectionFromRadius(notnull inout array<IEntitySource> selection)
	{		
		vector cameraMatrix[4];
		m_World.GetCurrentCamera(cameraMatrix);
		m_World.QueryEntitiesBySphere(cameraMatrix[3], m_selectionRadius, InsertEntity);
		
		foreach(IEntitySource src : m_tempQueryInsert)
		{
			if(!src) continue;
			AddToArrayIfPrefab(src, selection);
		}
		
		m_tempQueryInsert.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool InsertEntity(notnull IEntity entity)
	{
		m_tempQueryInsert.Insert(m_API.EntityToSource(entity));
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	private void AddToArrayIfPrefab(notnull IEntitySource src, notnull inout array<IEntitySource> output)
	{		
		EntityPrefabData prefabData = m_API.SourceToEntity(src).GetPrefabData();
		if(prefabData == null) return;
		
		ResourceName objPrefabName = prefabData.GetPrefabName();
		
		if(!ShouldSelect(objPrefabName)) return;
		
		output.Insert(src);
	}
	
	//------------------------------------------------------------------------------------------------
	private bool ShouldSelect(string prefabName)
	{
		if(prefabName == string.Empty) return false;
		
		if(m_prefabConfigIndexMap.Contains(prefabName)) return true;
		
		for(int i = 0; i < m_config.m_replacementEntries.Count(); i++)
		{
			DAB_PrefabRandomizerConfigEntry entry = m_config.m_replacementEntries[i];
			
			if(IsReplaced(entry, prefabName, i)) return true;
			
			if(!m_selectReplacers) continue;
			
			if(IsReplacer(entry, prefabName, i)) return true;
		}
		
		return false;
	}
	
	private bool IsReplaced(DAB_PrefabRandomizerConfigEntry entry, string prefabName, int i)
	{
		foreach(ResourceName replacedName : entry.m_replacedPrefabs)
		{
			if(replacedName != prefabName) continue;
		
			m_prefabConfigIndexMap.Insert(prefabName, i);
			return true;
		}
		
		return false;
	}
	
	private bool IsReplacer(DAB_PrefabRandomizerConfigEntry entry, string prefabName, int i)
	{
		foreach(ResourceName replacerName : entry.m_replacerPrefabs)
		{
			if(replacerName != prefabName) continue;
		
			m_prefabConfigIndexMap.Insert(prefabName, i);
			return true;
		}
		
		return false;
	}
}
#endif
