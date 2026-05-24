[EntityEditorProps(category: "GameLib/Scripted/Generator", description: "Rope Generator", dynamicBox: true, visible: false)]
class DAB_RopeGeneratorEntityClass : SCR_LineGeneratorBaseEntityClass
{
	static const string POWER_LINE_CLASS = "PowerlineEntity";
	
	[Attribute(defvalue: "{836210B285A81785}Assets/Structures/Infrastructure/Power/Powerlines/data/default_powerline_wire.emat", desc: "Rope Material (MatPBRCable)",  category: "Rope Settings", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "emat")]
	ResourceName m_ropeMaterial;
	
	[Attribute(defvalue: "{613763D114E7CD1C}Prefabs/WEGenerators/Powerline.et", desc: "Rope Prefab", category: "Rope Settings", params: "et class=" + POWER_LINE_CLASS)]
	ResourceName m_ropePrefab;
}

class DAB_RopeGeneratorEntity : SCR_LineGeneratorBaseEntity 
{
	#ifdef WORKBENCH	
	//------------------------------------------------------------------------------------------------
	protected override bool _WB_OnKeyChanged(BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		super._WB_OnKeyChanged(src, key, ownerContainers, parent);
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected override void OnShapeInitInternal(IEntitySource shapeEntitySrc, ShapeEntity shapeEntity)
	{
		super.OnShapeInitInternal(shapeEntitySrc, shapeEntity);
		OnShapeEvent(shapeEntitySrc, shapeEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected override void OnShapeChangedInternal(IEntitySource shapeEntitySrc, ShapeEntity shapeEntity, array<vector> mins, array<vector> maxes)
	{
		super.OnShapeChangedInternal(shapeEntitySrc, shapeEntity, mins, maxes);
		OnShapeEvent(shapeEntitySrc, shapeEntity);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnShapeEvent(IEntitySource shapeEntitySrc, ShapeEntity shapeEntity)
	{
		if (!m_bEnableGeneration)
		{
			Print("Rope generation is disabled for this shape!", LogLevel.NORMAL);
			return;
		}
		
		WorldEditorAPI worldEditorAPI = _WB_GetEditorAPI();
		if (!worldEditorAPI || worldEditorAPI.UndoOrRedoIsRestoring())
			return;

		bool manageEntityAction = !worldEditorAPI.IsDoingEditAction();
		if (manageEntityAction)
			worldEditorAPI.BeginEntityAction();

		RegenerateRope(m_Source, shapeEntitySrc, shapeEntity);

		if (manageEntityAction)
			worldEditorAPI.EndEntityAction();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void RegenerateRope(IEntitySource generatorSource, IEntitySource shapeEntitySrc, ShapeEntity shapeEntity)
	{
		WorldEditorAPI worldEditorAPI = ((WorldEditor)Workbench.GetModule(WorldEditor)).GetApi(); // cannot use _WB_GetEditorAPI() in a static method
		if (!worldEditorAPI || worldEditorAPI.UndoOrRedoIsRestoring())
			return;

		if (!worldEditorAPI.IsDoingEditAction())
		{
			PrintFormat("[DAB_RoperGeneratorEntity.RegenerateRope] not in Edit action, leaving (" + __FILE__ + " L" + __LINE__ + ")", this, level: LogLevel.WARNING);
			return;
		}
		
		DAB_RopeGeneratorEntity generatorEntity = DAB_RopeGeneratorEntity.Cast(worldEditorAPI.SourceToEntity(generatorSource));
		if(!generatorEntity) return;
		
		generatorEntity.DeleteAllChildren();
		
		//vector shapeOrigin = shapeEntity.GetOrigin();
		
		array<vector> anchorPoints = {};
		shapeEntity.GetPointsPositions(anchorPoints); //Because GetAnchorPoints Returns weird Y value
		
		for(int i; i < anchorPoints.Count() - 1; i++)
		{
			GenerateRope(worldEditorAPI, anchorPoints[i], anchorPoints[i+1]);
		}
	}
	
	protected void GenerateRope(WorldEditorAPI worldEditorAPI, vector fromPoint, vector toPoint)
	{	
		DAB_RopeGeneratorEntityClass prefabData = DAB_RopeGeneratorEntityClass.Cast(GetPrefabData());
		if (!prefabData) return;
		
		IEntitySource ropeEntitySource;
		string ropeEntityPrefab;
		
		if (prefabData.m_ropePrefab) // !.IsEmpty()
			ropeEntityPrefab = prefabData.m_ropePrefab;
		else
			ropeEntityPrefab = prefabData.POWER_LINE_CLASS;
		
		
		IEntity referenceEntity =  worldEditorAPI.SourceToEntity(m_Source);
		
		vector createdAt = referenceEntity.CoordToLocal(fromPoint);
		//Print("createdAt: " + createdAt);
		
		Print("fromPoint: " + fromPoint);
		vector spawnPos = fromPoint;
		
		ropeEntitySource = worldEditorAPI.CreateEntity(ropeEntityPrefab, string.Empty, m_iSourceLayerID, m_Source, spawnPos, vector.Zero);
		if (!ropeEntitySource)
		{
			Print("CreateEntity returned null trying to create " + ropeEntityPrefab, LogLevel.ERROR);
			return;
		}
		
		if (prefabData.m_ropeMaterial)
			worldEditorAPI.SetVariableValue(ropeEntitySource, null, "Material", prefabData.m_ropeMaterial);

		//referenceEntity =  worldEditorAPI.SourceToEntity(ropeEntitySource);
		fromPoint = fromPoint - spawnPos; //Should always be vector.zero
		toPoint = toPoint - spawnPos;
			
		worldEditorAPI.CreateObjectArrayVariableMember(ropeEntitySource, null, "Cables", "Cable", 0);	
		array<ref ContainerIdPathEntry> containerPath = { new ContainerIdPathEntry("Cables", 0) };
		worldEditorAPI.SetVariableValue(ropeEntitySource, containerPath, "StartPoint", fromPoint.ToString(false));
		worldEditorAPI.SetVariableValue(ropeEntitySource, containerPath, "EndPoint", toPoint.ToString(false));
	}
	
	//------------------------------------------------------------------------------------------------
	#endif
}