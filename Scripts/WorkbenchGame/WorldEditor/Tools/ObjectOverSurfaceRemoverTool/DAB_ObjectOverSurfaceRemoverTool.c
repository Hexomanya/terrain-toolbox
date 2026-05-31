#ifdef WORKBENCH
[WorkbenchToolAttribute(name: "Select Objects over Surface",description: "Selects objects over the defined Terrain Surfaces",wbModules: {"WorldEditor"}, awesomeFontCode: 0xF12D)]
class DAB_ObjectOverSurfaceRemover : WorldEditorTool
{	
	const protected ref array<float> m_aSearchAngles = {0, 45, 90, 135, 180, 225, 270, 315};
	const protected ref array<float> m_aSearchRadii = {0.25, 0.5, 1.0, 2.0};
	
	//------------------ Surface Materials -----------------------------------------------------------
	[Attribute(defvalue: "", desc: "Select objects over these surfaces",  category: "Surface Materials", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "emat")]
	protected ref array<ResourceName> m_selectionSurfaces;
	
	//------------------ Terrain Properties ----------------------------------------------------------
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "If enabled, also selects objects above roads", category: "Terrain Properties")]
	protected bool m_bRoadSelection;
	
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "If enabled, also selects objects above rivers", category: "Terrain Properties")]
	protected bool m_bRiverSelection;
	
	//------------------ Terrain Properties ----------------------------------------------------------
	[Attribute(defvalue: "100", desc: "", category: "Selection")]
	protected float m_fSelectionRadius;
	
	[Attribute(defvalue: "1", desc: "", category: "Selection")]
	protected bool m_bIgnoreGeneratorEntities;
	
	[Attribute(defvalue: "1", desc: "", category: "Selection")]
	protected bool m_bIgnoreShapeEntities;
	
	//------------------ Move Properties ----------------------------------------------------------
	[Attribute(defvalue: "", desc: "Will overwrite default(0.25, 0.5, 1.0, 2.0) when count > 0", category: "Move Entities")]
	protected ref array<float> m_aMoveRadiiOverwrite = {};
	
	
	//------------------ Local Variables -------------------------------------------------------------
	protected BaseWorld m_World;
	protected ref array<IEntitySource> m_aTempQueryInsert = {};
	
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
	[ButtonAttribute("Select Entities")]
	protected void SelectEntities()
	{
		m_API.ClearEntitySelection();
		array<IEntitySource> entitiesInRadius = {};
		GetSelectionFromRadius(entitiesInRadius, m_fSelectionRadius);
		
		array<IEntitySource> illegalEntitySources = {};
		foreach(IEntitySource source : entitiesInRadius)
		{
			IEntity entity = m_API.SourceToEntity(source);
			if(IsOnIllegalSurface(entity))
				illegalEntitySources.Insert(source);
		}
		
		if(illegalEntitySources.IsEmpty())
			Print("Did not find any entities that are over the defined surfaces.");
		
		DAB_EntityHelper.SetEntitySelection(illegalEntitySources);
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Filter Entities")]
	protected void FilterEntities()
	{
		array<IEntitySource> illegalEntitySources = {};
		
		if(m_selectionSurfaces.Count() < 1){
			Print("You need to set a surface first!", LogLevel.WARNING);
			return;
		}
		
		int entityCount = m_API.GetSelectedEntitiesCount();
		
		m_API.BeginEntityAction();
		
		for (int i; i < entityCount; i++)
		{
			IEntitySource entitySource = m_API.GetSelectedEntity(i);
			IEntity entity = m_API.SourceToEntity(entitySource);
			
			if(IsOnIllegalSurface(entity))
				illegalEntitySources.Insert(entitySource);
		}
		
		DAB_EntityHelper.SetEntitySelection(illegalEntitySources);
		m_API.EndEntityAction();
		
		PrintFormat("Found %1 entities that are over the defined surfaces.", illegalEntitySources.Count(), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Move to Valid Surface")]
	protected void MoveToValidSurface()
	{
	    if (m_selectionSurfaces.Count() < 1) {
	        Print("You need to set a surface first!", LogLevel.WARNING);
	        return;
	    }
	
	    int entityCount = m_API.GetSelectedEntitiesCount();
		int unmovedEntityCount = 0;
		array<float> searchRadii = m_aSearchRadii;
		if(m_aMoveRadiiOverwrite.Count() > 0) searchRadii.Copy(m_aMoveRadiiOverwrite);
		
	    m_API.BeginEntityAction();
	
	    for (int i = entityCount - 1; i >= 0; i--)
	    {
	        IEntitySource entitySource = m_API.GetSelectedEntity(i);
	        if (!entitySource) continue;
	
	        IEntity entity = m_API.SourceToEntity(entitySource);
	        if (!entity) continue;
	
	        if (!IsOnIllegalSurface(entity))
	            continue;
	
			vector mat[4];
			entity.GetWorldTransform(mat);
			vector worldPos = mat[3];
	        
	        bool found = false;
			
	

	        foreach (float radius : searchRadii)
	        {
	            if (found) break;
	
	            foreach (float angleDeg : m_aSearchAngles)
	            {
	                float angleRad = angleDeg * Math.DEG2RAD;
	
	                vector candidate;
	                candidate[0] = worldPos[0] + radius * Math.Cos(angleRad);
	                candidate[2] = worldPos[2] + radius * Math.Sin(angleRad);
					candidate[1] = m_API.GetTerrainSurfaceY(candidate[0], candidate[2]);
	                
	                if (IsIllegalSurface(candidate, entity)) continue;
	                
				    vector coordsToSet = candidate;
					IEntitySource parentSource = entitySource.GetParent();
				    if (parentSource)
				    {
				        IEntity parentEntity = m_API.SourceToEntity(parentSource);
				        if (parentEntity)
				            coordsToSet = parentEntity.CoordToLocal(candidate);
				    }
			
				    m_API.SetVariableValue(entitySource, {}, DAB_Constants.COORDS, coordsToSet.ToString(false));
				
				    found = true;
				    break;
	            }
	        }
			
	        if (!found) unmovedEntityCount++;
	        	
	    }
		
		PrintFormat("Could not find valid surface within max radius %1m for %2 entities", searchRadii[searchRadii.Count() - 1], unmovedEntityCount, LogLevel.WARNING);
	
	    m_API.EndEntityAction();
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool TryGetSurface(vector groundPosition, out TraceParam params)
	{
		if(!m_World){
			Print("There is no world to trace!", LogLevel.ERROR);
			return false;
		}
		params = new TraceParam();
		params.Flags = TraceFlags.WORLD | TraceFlags.ANY_CONTACT;
		params.Start = groundPosition + (0.1 * vector.Up);
		params.End = groundPosition - (1000 * vector.Up);
		
		float hitPercantage = m_World.TraceMove(params, null);
		bool didHit = !float.AlmostEqual(hitPercantage, 1);
		return didHit;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsInOrAboveLakeOrRiver(IEntity entity, vector groundPosition)
	{
		if(!m_World){
			Print("There is no world to trace!", LogLevel.ERROR);
			return false;
		}
		
		TraceParam params = new TraceParam();
		params.Flags = TraceFlags.ENTS | TraceFlags.ANY_CONTACT;
		params.TargetLayers = EPhysicsLayerDefs.Water;
		params.Exclude = entity;
		params.Start = groundPosition - (0.01 * vector.Up);
		params.End = groundPosition + (1000 * vector.Up);
		
		m_World.TraceMove(params, null);
		return params.TraceEnt != null;
	}
	
	protected bool IsIllegalSurface(vector groundWorldPosition, IEntity excludeFromTrace = null)
	{
		TraceParam params;
		if(TryGetSurface(groundWorldPosition, params))
		{
			if(IsOnBlacklistedSurfaces(params.TraceMaterial)) return true;
			if(m_bRoadSelection)
			{
				bool isOverRoad = SurfaceIsRoad(params.TraceMaterial);
				if(isOverRoad) return true;
			}
		}
		
		if(m_bRiverSelection)
		{
			vector outWaterSurfacePoint;
			EWaterSurfaceType outType; 
			vector transformWS[4];
			vector obbExtents;
			
			// Only works for ocean in editor
			bool didHitOcean = ChimeraWorldUtils.TryGetWaterSurface(m_World, groundWorldPosition, outWaterSurfacePoint, outType, transformWS, obbExtents);
			if(didHitOcean) return true;
			
			bool didHitWater = IsInOrAboveLakeOrRiver(excludeFromTrace, groundWorldPosition);
			if(didHitWater) return true;
		}
	
		return false;
	}
		
	//------------------------------------------------------------------------------------------------
	protected bool IsOnIllegalSurface(IEntity entity)
	{
		bool isGenerator = m_bIgnoreGeneratorEntities && GeneratorBaseEntity.Cast(entity);
		if(isGenerator) Print("Filtered generator!");
		
		bool isShape = m_bIgnoreShapeEntities && ShapeEntity.Cast(entity);
		if(isShape) Print("Filtered shape!");
		if(isGenerator || isShape ) return false;
		
		vector originPosition = entity.GetOrigin();
		float entityWorldHeight = m_API.GetTerrainSurfaceY(originPosition[0], originPosition[2]);
		vector groundWorldPosition = Vector(originPosition[0], entityWorldHeight, originPosition[2]);
		
		return IsIllegalSurface(groundWorldPosition, entity);
	}
	
	protected bool SurfaceIsRoad(string surfaceMaterial)
	{
		return surfaceMaterial.IndexOf("Road_") >= 0;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsOnBlacklistedSurfaces(string surfaceMaterial){
		if(surfaceMaterial.IsEmpty())
		{
			Print("IsOnBlacklistSurfaces was provided a empty surfaceMaterial!", LogLevel.ERROR);
			return false;
		}
		
		foreach(ResourceName blacklistSurface: m_selectionSurfaces)
		{
			if(blacklistSurface.GetPath() == surfaceMaterial) return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void GetSelectionFromRadius(notnull inout array<IEntitySource> selection, float radius)
	{	
		m_aTempQueryInsert.Clear();	
		
		vector cameraMatrix[4];
		m_World.GetCurrentCamera(cameraMatrix);
		m_World.QueryEntitiesBySphere(cameraMatrix[3], radius, InsertEntity);
		
		foreach(IEntitySource src : m_aTempQueryInsert)
		{
			if(!src) continue;
			selection.Insert(src);
		}
		
		m_aTempQueryInsert.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool InsertEntity(notnull IEntity entity)
	{
		m_aTempQueryInsert.Insert(m_API.EntityToSource(entity));
		return true;
	}
	
}
#endif
