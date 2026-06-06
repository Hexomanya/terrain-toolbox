#ifdef WORKBENCH
modded class WallGeneratorEntity : SCR_LineTerrainShaperGeneratorBaseEntity
{
	// ── Attributes ────────────────────────────────────────────────────────
	[Attribute(defvalue: "0", desc: "Flip ALL generated objects (walls, first/middle/last, custom point meshes) by 180° around the Yaw axis.\nThis also automatically shifts each piece by its own length so there are no holes and the wall still follows the polyline/spline perfectly.", category: "Other")]
	protected bool m_bFlipWalls180;

	// ── Static Caches ─────────────────────────────────────────────────────
	protected static ref map<ResourceName, float> m_mFlipShifts = new map<ResourceName, float>();

	// ── Override Methods ──────────────────────────────────────────────────

	//-----------------------------------------------------------------------
	protected override IEntitySource PlacePrefab(bool generate, ResourceName name, out vector pos, vector dir, vector prevDir, float rotationAdjustment, bool isGeneratorVisible, float length, float prePadding, float postPadding, float offsetUp, bool alignNext, bool prepadNext, bool snapToGround = false, bool allowClipping = false, vector offsetRight = vector.Zero)
	{
		if (!m_bFlipWalls180 || length <= 0)
			return super.PlacePrefab(generate, name, pos, dir, prevDir, rotationAdjustment, isGeneratorVisible, length, prePadding, postPadding, offsetUp, alignNext, prepadNext, snapToGround, allowClipping, offsetRight);

		vector orientation;
		if (alignNext)
			orientation = dir;
		else
			orientation = prevDir;

		vector prepadDirection;
		if (prepadNext)
			prepadDirection = dir;
		else
			prepadDirection = prevDir;

		// 1. Advance the cursor EXACTLY as the base class does for pre-padding
		pos = pos + (prepadDirection * prePadding);

		// 2. Calculate perfect flipped position using bounding box geometry
		int axis = 2;
		if (UseXAsForward)
			axis = 0;

		WorldEditorAPI api = _WB_GetEditorAPI();
		float shiftAmount = GetFlipShift(name, axis, api);
		
		vector spawnPosition = pos + (dir * shiftAmount);
		spawnPosition = spawnPosition + offsetRight;

		float flippedRotation = rotationAdjustment + 180.0;

		IEntitySource entity;
		if (generate)
		{
			WorldEditorAPI worldEditorAPI = _WB_GetEditorAPI();
			
			if (m_bSnapOffsetShapeToTheGround)
			{
				vector worldPosition = CoordToParent(spawnPosition);
				spawnPosition[1] = worldEditorAPI.GetTerrainSurfaceY(worldPosition[0], worldPosition[2]);
				
				if (m_ParentShapeSource)
					spawnPosition[1] = spawnPosition[1] - worldEditorAPI.SourceToEntity(m_ParentShapeSource).GetOrigin()[1];
			}
			
			spawnPosition[1] = spawnPosition[1] + offsetUp;

			vector rotationMatrix[4];
			Math3D.DirectionAndUpMatrix(orientation, vector.Up, rotationMatrix);
			vector rotation = Math3D.MatrixToAngles(rotationMatrix);
			rotation = { 0, Math.Repeat(rotation[0] + flippedRotation, 360), 0 };

			if (snapToGround)
				entity = worldEditorAPI.CreateEntityExt(name, string.Empty, m_iSourceLayerID, m_Source, spawnPosition, rotation, TraceFlags.ENTS);
			else
				entity = worldEditorAPI.CreateEntity(name, string.Empty, m_iSourceLayerID, m_Source, spawnPosition, rotation);

			worldEditorAPI.SetEntityVisible(entity, isGeneratorVisible, false);
		}

		// 3. Advance the cursor EXACTLY as the base class does for the remainder
		float stepLength = length;
		if (allowClipping)
			stepLength = 0;

		pos = pos + (dir * Math.Max(0.1, stepLength + postPadding));

		return entity;
	}

	//-----------------------------------------------------------------------
	protected override IEntitySource CreateWallEntity(WorldEditorAPI worldEditorAPI, BaseWorld world, ResourceName resourceName, inout vector startPos, vector relPosOffset, float prePadding, float postPadding, float dirOffset, int anchorLimit = -1)
	{
		if (!m_bFlipWalls180)
			return super.CreateWallEntity(worldEditorAPI, world, resourceName, startPos, relPosOffset, prePadding, postPadding, dirOffset, anchorLimit);

		int axis = 2;
		if (UseXAsForward)
			axis = 0;

		float meshLength = MeasureEntity(resourceName, axis);
		if (meshLength <= 0) 
			return null;

		float totalSlotSize = meshLength + prePadding + postPadding;
		
		vector nextPoint;
		if (!m_ShapeNextPointHelper.GetNextPoint(totalSlotSize, nextPoint, anchorLimit, xzMode: true))
			return null;

		vector direction = vector.Direction(startPos, nextPoint).Normalized();
		if (direction == vector.Zero)
			return null;

		// 1. Find the base position (start + prepad)
		vector baseSpawnPos = startPos + (direction * prePadding);
		
		// 2. Shift to align the flipped bounding box
		WorldEditorAPI api = _WB_GetEditorAPI();
		float shiftAmount = GetFlipShift(resourceName, axis, api);
		vector flippedPosition = baseSpawnPos + (direction * shiftAmount);
		
		vector rightVector = direction * -vector.Up;
		flippedPosition = flippedPosition + (relPosOffset[0] * rightVector) + (relPosOffset[1] * vector.Up);
		
		if (m_bSnapOffsetShapeToTheGround)
		{
			vector absolutePosition = CoordToParent(flippedPosition);
			absolutePosition[1] = world.GetSurfaceY(absolutePosition[0], absolutePosition[2]) + m_vShapeOffset[1];
			flippedPosition = CoordToLocal(absolutePosition);
		}
		
		flippedPosition[1] = flippedPosition[1] + m_vShapeOffset[1];

		vector angles = direction.VectorToAngles();
		angles = { 0, Math.Repeat(angles[0] + dirOffset + 180, 360), 0 };

		IEntitySource result = worldEditorAPI.CreateEntity(resourceName, string.Empty, m_iSourceLayerID, m_Source, flippedPosition, angles);
		
		startPos = nextPoint; 
		return result;
	}

	//-----------------------------------------------------------------------
	protected override void GenerateClosestToShape(notnull array<vector> anchorPoints, float rotationOffset)
	{
		super.GenerateClosestToShape(anchorPoints, rotationOffset);
		MoveLastObjectToStart(anchorPoints);
	}

	//-----------------------------------------------------------------------
	protected override void GenerateInStraightLine(notnull array<vector> anchorPoints, float rotationOffset, int forwardAxis)
	{
		super.GenerateInStraightLine(anchorPoints, rotationOffset, forwardAxis);
		MoveLastObjectToStart(anchorPoints);
	}

	// ── Protected Methods ─────────────────────────────────────────────────

	//-----------------------------------------------------------------------
	protected static float GetFlipShift(ResourceName resourceName, int measureAxis, WorldEditorAPI api)
	{
		if (!resourceName) 
			return 0;
			
		float result;
		if (m_mFlipShifts.Find(resourceName, result)) 
			return result;

		Resource resource = Resource.Load(resourceName);
		if (!resource.IsValid())
		{
			m_mFlipShifts.Insert(resourceName, 0);
			return 0;
		}

		IEntity dummyEntity;
		if (s_World) 
			dummyEntity = GetGame().SpawnEntityPrefab(resource, s_World);
		else
		{
			if (api) 
				dummyEntity = GetGame().SpawnEntityPrefab(resource, api.GetWorld());
		}

		if (dummyEntity)
		{
			vector minBB, maxBB;
			dummyEntity.GetBounds(minBB, maxBB);
			delete dummyEntity;
			
			// The mathematical offset needed to perfectly align a flipped bounding box
			result = minBB[measureAxis] + maxBB[measureAxis];
		}
		else
		{
			result = 0;
		}

		m_mFlipShifts.Insert(resourceName, result);
		return result;
	}

	//-----------------------------------------------------------------------
	protected void MoveLastObjectToStart(notnull array<vector> anchorPoints)
	{
		if (!m_bFlipWalls180 || !m_bEnableLastObject || !LastObject || anchorPoints.Count() < 2) 
			return;

		int childCount = m_Source.GetNumChildren();
		if (childCount == 0) 
			return;

		IEntitySource lastObjectSource = IEntitySource.Cast(m_Source.GetChild(childCount - 1));
		if (!lastObjectSource) 
			return;

		WorldEditorAPI worldEditorAPI = _WB_GetEditorAPI();
		
		vector startPosition = anchorPoints[0];
		vector firstSegmentDirection = vector.Direction(anchorPoints[0], anchorPoints[1]).Normalized();
		vector rightVector = firstSegmentDirection * -vector.Up;

		vector finalPosition = startPosition;
		finalPosition = finalPosition + (rightVector * LastObjectOffsetRight);
		
		float verticalTotal = LastObjectOffsetUp + m_vShapeOffset[1];
		
		if (m_bSnapOffsetShapeToTheGround)
		{
			vector worldPosition = CoordToParent(finalPosition);
			finalPosition[1] = worldEditorAPI.GetTerrainSurfaceY(worldPosition[0], worldPosition[2]);
			
			if (m_ParentShapeSource)
			{
				IEntity shapeEntity = worldEditorAPI.SourceToEntity(m_ParentShapeSource);
				finalPosition[1] = finalPosition[1] - shapeEntity.GetOrigin()[1];
			}
		}
		
		finalPosition[1] = finalPosition[1] + verticalTotal;

		vector segmentAngles = firstSegmentDirection.VectorToAngles();
		float rotationOffset = 0;
		if (UseXAsForward)
			rotationOffset = -90;

		float finalYaw = Math.Repeat(segmentAngles[0] + rotationOffset + 180.0, 360.0);
		vector finalAngles = { 0, finalYaw, 0 };

		worldEditorAPI.SetVariableValue(lastObjectSource, null, "coords", string.Format("%1 %2 %3", finalPosition[0], finalPosition[1], finalPosition[2]));
		worldEditorAPI.SetVariableValue(lastObjectSource, null, "angles", string.Format("%1 %2 %3", finalAngles[0], finalAngles[1], finalAngles[2]));
	}
}
#endif