#ifdef WORKBENCH
class DAB_WorldHelper
{
	
	//------------------------------------------------------------------------------------------------
	static bool IsPositionOverSurfaces(
		BaseWorld world,
		vector groundWorldPosition, 
		array<ResourceName> surfaces, 
		bool includeWater = false,
		bool includeRoads = false,
		IEntity excludeFromTrace = null
	)
	{
		TraceParam params;
		if(DAB_WorldHelper.TryGetSurface(world, groundWorldPosition, params))
		{
			if(DAB_WorldHelper.IsOnBlacklistedSurfaces(params.TraceMaterial, surfaces)) return true;
			if(includeRoads)
			{
				bool isOverRoad = SurfaceIsRoad(params.TraceMaterial);
				if(isOverRoad) return true;
			}
		}
		
		if(!includeWater) return false;
		
		vector outWaterSurfacePoint;
		EWaterSurfaceType outType; 
		vector transformWS[4];
		vector obbExtents;
		
		// Only works for ocean in editor
		bool didHitOcean = ChimeraWorldUtils.TryGetWaterSurface(world, groundWorldPosition, outWaterSurfacePoint, outType, transformWS, obbExtents);
		if(didHitOcean) return true;
		
		bool didHitWater = DAB_WorldHelper.IsInOrAboveLakeOrRiver(world, excludeFromTrace, groundWorldPosition);
		if(didHitWater) return true;
		
		return false;
	}
		
	//------------------------------------------------------------------------------------------------
	static bool TryGetSurface(BaseWorld world, vector groundPosition, out TraceParam params)
	{
		if(!world){
			Print("There is no world to trace!", LogLevel.ERROR);
			return false;
		}
		params = new TraceParam();
		params.Flags = TraceFlags.WORLD | TraceFlags.ANY_CONTACT;
		params.Start = groundPosition + (0.1 * vector.Up);
		params.End = groundPosition - (1000 * vector.Up);
		
		float hitPercantage = world.TraceMove(params, null);
		bool didHit = !float.AlmostEqual(hitPercantage, 1);
		return didHit;
	}
	
	static bool SurfaceIsRoad(string surfaceMaterial)
	{
		return surfaceMaterial.IndexOf("Road_") >= 0;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool IsOnBlacklistedSurfaces(string surfaceMaterial, array<ResourceName> surfaces){
		if(surfaceMaterial.IsEmpty())
		{
			Print("IsOnBlacklistSurfaces was provided a empty surfaceMaterial!", LogLevel.ERROR);
			return false;
		}
		
		foreach(ResourceName blacklistSurface: surfaces)
		{
			if(blacklistSurface.GetPath() == surfaceMaterial)
			{
				return true;
			} 
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool IsInOrAboveLakeOrRiver(BaseWorld world, IEntity entity, vector worldGroundPosition)
	{
		if(!world){
			Print("There is no world to trace!", LogLevel.ERROR);
			return false;
		}
		
		// TODO: We could ensure that we have the ground position, but that would require another call (Performance Tradeoff)
		
		TraceParam params = new TraceParam();
		params.Flags = TraceFlags.ENTS | TraceFlags.ANY_CONTACT;
		params.TargetLayers = EPhysicsLayerDefs.Water;
		params.Exclude = entity;
		params.Start = worldGroundPosition - (0.01 * vector.Up);
		params.End = worldGroundPosition + (1000 * vector.Up);
		
		world.TraceMove(params, null);
		return params.TraceEnt != null;
	}
}
#endif // Workbench