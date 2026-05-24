modded class ForestGeneratorEntity {
	#ifdef WORKBENCH

	protected ref DAB_TerrainObstacleGrid m_TerrainObstacleGrid = new DAB_TerrainObstacleGrid();
	
	protected const float MAX_SLOPE_ANGLE = 90;
	
	[Attribute(defvalue: "0", desc: "", category: "Slope Settings")]
	bool m_bEnableSlopeChecks;
	
	[Attribute(defvalue: "0", desc: "This only works if slope checks are enabled", category: "Slope Settings")]
	bool m_bEnableSlopeOutlines;
	
	[Attribute(defvalue: "", category: "Slope Settings", desc: "", uiwidget: UIWidgets.CurveDialog, params: string.Format("%1 1 0 0", MAX_SLOPE_ANGLE))]
	protected ref Curve m_SlopeCurve;
	
	[Attribute(defvalue: "-1", desc: "Will be used if > 0", category: "Slope Settings")]
	float m_fManualOutlineSlope;
	
	[Attribute(defvalue: "0", desc: "", category: "Slope Settings")]
	bool m_bUseSlopeStartAsOutline;
	
	
	[Attribute(defvalue: "80", desc: "", category: "Outline Settings")]
	float m_fSmallOutlineMinArea;
	
	[Attribute(defvalue: "700", desc: "", category: "Outline Settings")]
	float m_fMiddleOutlineMinArea;
	
	
	[Attribute(defvalue: "1", desc: "", category: "Outline Settings - Tree Swapping")]
	float m_fUseOutlinTreesOnSlopeWeight;
	
	[Attribute(defvalue: "1", desc: "", category: "Outline Settings - Tree Swapping")]
	bool m_bSwapWithTopTrees;
	
	[Attribute(defvalue: "0", desc: "", category: "Outline Settings - Tree Swapping")]
	bool m_bSwapWithMiddleTrees;
	
	
	[Attribute(defvalue: "3.5", desc: "Lowering this WILL hurt performance", category: "Outline Settings - Advanced")]
	float m_fQueryGridSize;
	
	[Attribute(defvalue: "0.8", desc: "", category: "Outline Settings - Advanced")]
	float m_fScaleDownFactor;
	
	[Attribute(defvalue: "1.2", desc: "meters", category: "Outline Settings - Advanced")]
	float m_fMaxPointRemovalError;
	
	[Attribute(defvalue: "0", desc: "", category: "Outline Settings - Advanced")]
	bool m_bEnableSlopeDebugVisualization;
	
	
	protected ref array<ref DAB_ForestOutline> m_aSlopeOutlines = {};
	protected ref array<ref ForestGeneratorTree> m_aSwapableOutlineTrees = {};
	int m_iTreeSwapCounter = 0;
	
	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] polygon2D
	//! \param[in] polygon3D
	protected override void PopulateGrid(array<float> polygon2D, array<vector> polygon3D)
	{
		m_Grid.Clear();
		m_aGridEntries.Clear();

		m_fArea = SCR_Math2D.GetPolygonArea(polygon2D);
		
		SCR_AABB bbox = new SCR_AABB(polygon3D);
		m_Grid.Resize(bbox.m_vDimensions[0], bbox.m_vDimensions[2]);

		vector worldMat[4];
		GetWorldTransform(worldMat);
		
		if(m_bEnableSlopeChecks)
		{
			WorldEditorAPI worldEditorAPI = _WB_GetEditorAPI();
			BaseWorld world = worldEditorAPI.GetWorld();
			float slopeCutOff = GetSlopeTotalCutOff();
			
			Debug.BeginTimeMeasure();
			m_TerrainObstacleGrid.RegenerateGrid(m_fQueryGridSize, bbox, polygon2D, worldMat, world, slopeCutOff, m_bEnableSlopeOutlines, Math.Min(m_fSmallOutlineMinArea, m_fMiddleOutlineMinArea), m_fMaxPointRemovalError);
			Debug.EndTimeMeasure("ForestGenerator - Slope generation done");
	
			Debug.BeginTimeMeasure();
			m_aSlopeOutlines.Clear();
			m_TerrainObstacleGrid.CompileForestGeneratorData(m_aSlopeOutlines, m_fSmallOutlineMinArea, m_fMiddleOutlineMinArea);
			AddSlopeOutlinesToExistingOutlines();
			Debug.EndTimeMeasure("ForestGenerator - Slope retrival done");
			
			if(m_bEnableSlopeDebugVisualization) 
			{
				if(!m_bEnableSlopeOutlines) Print("'EnableSlopeDebugVisualization' is enabled but 'EnableSlopeOutlines' is not. The debug view will have nothing to show!", LogLevel.WARNING);
				m_TerrainObstacleGrid.DebugVisualizeGrid(s_DebugShapeManager, world, slopeCutOff);
			}
			
			Debug.BeginTimeMeasure();
			GatherSwapableOutlineTrees();
			Debug.EndTimeMeasure("ForestGenerator - Gather outline swap trees done");
		}
		
		// Needs slope data
		Debug.BeginTimeMeasure();
		Rectangulate(bbox, polygon2D);
		Debug.EndTimeMeasure("ForestGenerator - Rectangulation done");
		
		vector bboxMin = bbox.m_vMin;
		vector bboxMinWorld = bboxMin.Multiply4(worldMat);
		m_Grid.SetPointOffset(bboxMinWorld[0], bboxMinWorld[2]);

		Debug.BeginTimeMeasure();
		GenerateForestGeneratorTrees(polygon2D, bbox);
		Debug.EndTimeMeasure("ForestGenerator - Grid tree generation done");
		
		PrintFormat("Swapped %1 tall trees for outline trees.", m_iTreeSwapCounter);
	}
	
	// Same as base, but with Sqr distance to save some ms
	protected override bool IsBeforeOutlineBorder(notnull SCR_ForestGeneratorRectangle rectangle, vector pointLocal, float offset = 0)
	{		
		foreach (SCR_ForestGeneratorLine line : rectangle.m_aLines)
		{
			// Math3D.PointLineSegmentDistanceSqr => Vanilla points always have Y = 0, so why are we using 3D?
			float distanceSq = PointLineSegmentDistanceSqrXZ(line, pointLocal);
			foreach (int index, ForestGeneratorOutline outline : m_aOutlines)
			{
				if (!outline.m_bGenerate)
					continue;

				if (outline.m_eOutlineType == SCR_EForestGeneratorOutlineType.SMALL && !line.p1.m_bSmallOutline)
					continue;

				if (outline.m_eOutlineType == SCR_EForestGeneratorOutlineType.MIDDLE && !line.p1.m_bMiddleOutline)
					continue;

				float maxDistance = outline.m_fMaxDistance - offset; //Calculating everytime is faster then pre calc and cache
				if (distanceSq < (maxDistance * maxDistance))
					return false;
			}
		}

		return true;
	}

	//This works now, but could break if the ForestGeneratorEnity gets updated and uses these in a different way
	protected void AddSlopeOutlinesToExistingOutlines()
	{
		foreach(DAB_ForestOutline outline : m_aSlopeOutlines)
		{
			// Insert all produces weird mismatch error (when they are the same type)
			foreach(SCR_ForestGeneratorPoint point : outline.m_aSmallOutlinePoints)
				m_aSmallOutlinePoints.Insert(point);
			
			foreach(SCR_ForestGeneratorPoint point : outline.m_aMiddleOutlinePoints)
				m_aMiddleOutlinePoints.Insert(point);
					
			foreach(SCR_ForestGeneratorLine line : outline.m_aSmallOutlineLines)
				m_aSmallOutlineLines.Insert(line);
			
			foreach(SCR_ForestGeneratorLine line : outline.m_aMiddleOutlineLines)
				m_aMiddleOutlineLines.Insert(line);
			
			foreach(SCR_ForestGeneratorLine line : outline.m_aAllOutlineLines)
				m_aLines.Insert(line);
		}
	}
	
	protected float GetSlopeTotalCutOff(float epsilon = 0.0001,)
	{
		if(m_fManualOutlineSlope > 0) return m_fManualOutlineSlope;
		
		int count = m_SlopeCurve.Count();
		if(count < 2)
		{
			PrintFormat("The slope curve needs at least 2 points to work correctly. It has currently has %1", count, LogLevel.WARNING);
			return -1;
		}
		
		for(int i = 0; i < m_SlopeCurve.Count(); i++)
		{
			vector currentPoint = m_SlopeCurve[i];
			
			if(!m_bUseSlopeStartAsOutline) 
			{
				if(currentPoint[1] <= epsilon) return currentPoint[0];
				else continue;
			}
			 
			// m_bUseSlopeStartAsOutline, from here on out
			if(i == 0)
			{
				if(currentPoint[1] >= 1) continue;

				PrintFormat("First point in curve has a value of %1. With m_bUseSlopeStartAsOutline=true this would mean a double outlines!", currentPoint[1], LogLevel.WARNING);
				return -1;
			}
			
			vector prevPoint = m_SlopeCurve[i - 1];
			if(currentPoint[1] < 1) return prevPoint[0];
		}
	
		return -1;
	}
	
	protected bool IsSlopeToSteep(vector pointLocal, BaseWorld world, out float slopeSizeFactor, out bool hardCutOff, out float swapTreePercent)
	{
		if(!m_bEnableSlopeChecks) return false;
		
	    if (!world)
	        return false;
	
	    float slope = m_TerrainObstacleGrid.GetSlopeAtLocalPoint(pointLocal);
		if(slope == m_TerrainObstacleGrid.INVALID_SLOPE) return false;
	
	    array<float> curveKnots = {};
	    foreach (vector p : m_SlopeCurve)
	    {
	        curveKnots.Insert(p[0]);
	    }
	
	    vector curvePoint = LegacyCurve.Curve(ECurveType.CatmullRom, slope, m_SlopeCurve, curveKnots);
	    float shouldExistPercent = Math.Clamp(curvePoint[1], 0 ,1);
		hardCutOff = shouldExistPercent < 1;

	    float rand01 = m_RandomGenerator.RandFloat01();
	    if (shouldExistPercent < rand01)
	        return true;
	
	    slopeSizeFactor = Math.Lerp(m_fScaleDownFactor, 1.0, shouldExistPercent);
		
		swapTreePercent = 1 - shouldExistPercent;
		swapTreePercent = Math.Clamp(swapTreePercent * m_fUseOutlinTreesOnSlopeWeight, 0, 1);
		
	    return false;
	}
	
	protected bool IsEntrySlopeValid(inout ForestGeneratorTree tree, vector pointLocal, out float swapTreePercent)
	{
		if (!m_bEnableSlopeChecks)
			return true;
	
		WorldEditorAPI worldEditorAPI = _WB_GetEditorAPI();
		BaseWorld world = worldEditorAPI.GetWorld();
	
		float slopeSizeFactor = 1;
		bool hardCutOff = false;
		bool isTooSteep = IsSlopeToSteep(pointLocal, world, slopeSizeFactor, hardCutOff, swapTreePercent);
	
		if (ForestGeneratorTreeMiddle.Cast(tree) || ForestGeneratorTreeTall.Cast(tree))
		{
			if (isTooSteep)
				return false;
	
			tree.m_fScale *= slopeSizeFactor;
			return true;
		}
	
		if (FallenTree.Cast(tree))
			return !hardCutOff;
	
		return !isTooSteep;
	}

	// Filter by Slope
	//------------------------------------------------------------------------------------------------
	//! \param[in] tree
	//! \param[in] pointLocal
	//! \return
	protected override bool IsEntryValid(ForestGeneratorTree tree, vector pointLocal)
	{
		return IsEntryValidAdvanced(tree, pointLocal);
	}
	
	protected bool IsEntryValidAdvanced(inout ForestGeneratorTree tree, vector pointLocal)
	{
		if (!tree) return false;
		
		float swapTreePercent
		if(!IsEntrySlopeValid(tree, pointLocal, swapTreePercent)) return false;

		FallenTree fallenTree = FallenTree.Cast(tree);
		if (fallenTree)
		{
			float distance;
			float minDistance;
			foreach (SCR_ForestGeneratorLine line : m_aLines)
			{
				distance = Math3D.PointLineSegmentDistance(pointLocal, line.p1.m_vPos, line.p2.m_vPos);
				minDistance = fallenTree.GetMinDistanceFromLine();
				if (distance < minDistance)
					return false;
			}
			
			return true;
		}

		if(m_fUseOutlinTreesOnSlopeWeight > 0.001) ChangeTreeForOutline(tree, pointLocal, swapTreePercent);
		return true;
	}
	
	//Dirty hack to change trees on the slope for Outline trees
	protected void ChangeTreeForOutline(inout ForestGeneratorTree tree, vector point, float swapTreePercent)
	{
		if(m_aSwapableOutlineTrees.IsEmpty()) return;
		if(m_RandomGenerator.RandFloat01() > swapTreePercent) return;
		
		bool isAllowedTall = m_bSwapWithTopTrees && ForestGeneratorTreeTall.Cast(tree);
		bool isAllowedMiddle = m_bSwapWithMiddleTrees && ForestGeneratorTreeMiddle.Cast(tree);
		if (!isAllowedTall && !isAllowedMiddle) return;
		
		
		ForestGeneratorTree newTree = SelectTreeToSpawn(point, m_aSwapableOutlineTrees);
		if(!newTree) return;

		tree = newTree;
		m_iTreeSwapCounter++;
	}
	
	protected void GatherSwapableOutlineTrees()
	{
		m_aSwapableOutlineTrees.Clear();
		int tallCounter = 0;
		int middleCounter = 0;
		
		if (!m_bSwapWithTopTrees && !m_bSwapWithMiddleTrees) return;
		if(m_fUseOutlinTreesOnSlopeWeight <= 0.001) return;
		
		foreach(ForestGeneratorOutline outline : m_aOutlines)
		{
			foreach(TreeGroupClass group : outline.m_aTreeGroups)
			{
				foreach(ForestGeneratorTree tree : group.m_aTrees)
				{
					// TODO: Readable vs performance
					if(m_bSwapWithTopTrees && ForestGeneratorTreeTall.Cast(tree)) {
						m_aSwapableOutlineTrees.Insert(tree);
						tallCounter++;
						continue;
					} 
					
					if(m_bSwapWithMiddleTrees && ForestGeneratorTreeMiddle.Cast(tree)){
						m_aSwapableOutlineTrees.Insert(tree);
						middleCounter++;
					} 
				}
			}
		}
		
		PrintFormat("Found %1 swapable tree. Tall: %2; Middle: %3;", (tallCounter + middleCounter), tallCounter, middleCounter);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Empty now-useless arrays
	protected override void MemoryCleanup()
	{
		super.MemoryCleanup();
		m_TerrainObstacleGrid.ClearGrid();
		m_aSlopeOutlines.Clear();
		m_aSwapableOutlineTrees.Clear();
		m_iTreeSwapCounter = 0;
	}
	
	//Optimized Rectangulate (~ 10x faster (especially for many lines))
	protected override void Rectangulate(notnull SCR_AABB bbox, notnull array<float> polygon2D)
	{
		array<float> lineLimits = new array<float>(); //minx, maxx, minz, maxz
		
		foreach (SCR_ForestGeneratorLine line : m_aLines)
		{
		    vector p1 = line.p1.m_vPos;
		    vector p2 = line.p2.m_vPos;
		    lineLimits.Insert(Math.Min(p1[0], p2[0]));
		    lineLimits.Insert(Math.Max(p1[0], p2[0]));
			lineLimits.Insert(Math.Min(p1[2], p2[2]));
		    lineLimits.Insert(Math.Max(p1[2], p2[2]));
		}
		
		vector direction = bbox.m_vMax - bbox.m_vMin;
		float targetRectangleWidth = RECTANGULATION_SIZE;
		float targetRectangleLength = RECTANGULATION_SIZE;
		int targetRectangleCountW = Math.Ceil(direction[0] / targetRectangleWidth);
		int targetRectangleCountL = Math.Ceil(direction[2] / targetRectangleLength);
		vector ownerOrigin = GetOrigin();
		SCR_ForestGeneratorRectangle rectangle;
		vector p1;
		Shape shape;
		
		for (int x; x < targetRectangleCountW; x++)
		{
			for (int y; y < targetRectangleCountL; y++)
			{
				bool isInPolygon = false;
				rectangle = new SCR_ForestGeneratorRectangle();
				rectangle.m_iX = x;
				rectangle.m_iY = y;
				rectangle.m_fWidth = targetRectangleWidth;
				rectangle.m_fLength = targetRectangleLength;
				rectangle.m_fArea = rectangle.m_fLength * rectangle.m_fWidth;
				
				p1 = bbox.m_vMin;
				p1[0] = p1[0] + (x * targetRectangleWidth);
				p1[2] = p1[2] + (y * targetRectangleLength);
				rectangle.m_Line4.p2.m_vPos = p1;
				rectangle.m_Line1.p1.m_vPos = p1;
				if (!isInPolygon)
					isInPolygon = Math2D.IsPointInPolygon(polygon2D, p1[0], p1[2]);
				rectangle.m_aPoints.Insert(p1);
				
				p1[0] = p1[0] + targetRectangleWidth;
				rectangle.m_Line1.p2.m_vPos = p1;
				rectangle.m_Line2.p1.m_vPos = p1;
				if (!isInPolygon)
					isInPolygon = Math2D.IsPointInPolygon(polygon2D, p1[0], p1[2]);
				rectangle.m_aPoints.Insert(p1);
				
				p1[2] = p1[2] + targetRectangleLength;
				rectangle.m_Line2.p2.m_vPos = p1;
				rectangle.m_Line3.p1.m_vPos = p1;
				if (!isInPolygon)
					isInPolygon = Math2D.IsPointInPolygon(polygon2D, p1[0], p1[2]);
				rectangle.m_aPoints.Insert(p1);
				
				p1[0] = p1[0] - targetRectangleWidth;
				rectangle.m_Line3.p2.m_vPos = p1;
				rectangle.m_Line4.p1.m_vPos = p1;
				if (!isInPolygon)
					isInPolygon = Math2D.IsPointInPolygon(polygon2D, p1[0], p1[2]);
				rectangle.m_aPoints.Insert(p1);
	
				vector mins;
				vector maxs;
				rectangle.GetBounds(mins, maxs);
				float rectMinX = mins[0];
				float rectMaxX = maxs[0];
				float rectMinZ = mins[2];
				float rectMaxZ = maxs[2];
				
				foreach (int index, SCR_ForestGeneratorLine line : m_aLines)
				{
					int idx = index * 4; //minx, maxx, minz, maxz
					bool isCompletlyOutsideRect = (
						lineLimits[idx] > rectMaxX  
						||	lineLimits[idx + 1] < rectMinX
						|| 	lineLimits[idx + 2] > rectMaxZ 	
						|| 	lineLimits[idx + 3] < rectMinZ
					);
					
					if (isCompletlyOutsideRect) continue;
					
					if (!IsAtLeastOnePointInRect(line, rectMinX, rectMaxX, rectMinZ, rectMaxZ) && !IsIntersectAfterBounds(line, rectangle)) continue;
				
					bool found = false;
					foreach (SCR_ForestGeneratorLine rectLine : rectangle.m_aLines)
					{
						if (rectLine == line)
						{
							found = true;
							break;
						}
					}
					
					if (!found)
						rectangle.m_aLines.Insert(line);
				}
	
				bool areLinesEmpty = rectangle.m_aLines.IsEmpty();
				if (areLinesEmpty && !isInPolygon)
					continue;
				
				m_aRectangles.Insert(rectangle);
				if (areLinesEmpty)
					m_aNonOutlineRectangles.Insert(rectangle);
				else
					m_aOutlineRectangles.Insert(rectangle);
					
				if (m_bDrawDebugShapesRectangulation)
				{
					int red, green, blue;
					if (areLinesEmpty)
					{
						green = Math.Floor(m_RandomGenerator.RandFloatXY(0, 255));
						blue = Math.Floor(m_RandomGenerator.RandFloatXY(0, 255));
					}
					else
					{
						red = 255;
					}
					s_DebugShapeManager.AddBBox(ownerOrigin + rectangle.m_Line1.p1.m_vPos, ownerOrigin + rectangle.m_Line3.p1.m_vPos, ARGB(63, red, green, blue));
				}
			}
		}
	}
	
	protected bool IsAtLeastOnePointInRect(SCR_ForestGeneratorLine line, float minX, float maxX, float minZ, float maxZ)
	{
		if(IsPointInRect(line.p1.m_vPos, minX, maxX, minZ, maxZ)) return true;
		return IsPointInRect(line.p2.m_vPos, minX, maxX, minZ, maxZ);
	}
	
	protected bool IsPointInRect(vector point, float minX, float maxX, float minZ, float maxZ)
	{
	    return (point[0] >= minX && point[0] <= maxX && point[2] >= minZ && point[2] <= maxZ);
	}
		
	// TODO: Check if we actually still need this
	// Fixed version, vanilla version had wrong math. TODO: two of those 4 checks are always true, because this is called after we know the are colinear
	protected override bool OnLine(SCR_ForestGeneratorLine line, SCR_ForestGeneratorPoint point)
	{
		float maxX = Math.Max(line.p1.m_vPos[0], line.p2.m_vPos[0]);
		float minX = Math.Min(line.p1.m_vPos[0], line.p2.m_vPos[0]);
		float maxZ = Math.Max(line.p1.m_vPos[2], line.p2.m_vPos[2]);
		float minZ = Math.Min(line.p1.m_vPos[2], line.p2.m_vPos[2]);

		return
			point.m_vPos[0] <= maxX &&
			point.m_vPos[0] >= minX &&
			point.m_vPos[2] <= maxZ &&
			point.m_vPos[2] >= minZ;
	}
	
	protected bool IsIntersectAfterBounds(SCR_ForestGeneratorLine line1, SCR_ForestGeneratorLine rectLine)
	{
		// four Direction for two lines and points of other line
		int dir1 = Direction(line1.p1, line1.p2, rectLine.p1);
		int dir2 = Direction(line1.p1, line1.p2, rectLine.p2);
		int dir3 = Direction(rectLine.p1, rectLine.p2, line1.p1);
		int dir4 = Direction(rectLine.p1, rectLine.p2, line1.p2);

		return (dir1 != dir2 && dir3 != dir4);
		// We do not need the colinear check, since this would have triggered the Bounds check
	}
	
	protected bool IsIntersectAfterBounds(SCR_ForestGeneratorLine line, SCR_ForestGeneratorRectangle rectangle)
	{
		return
			IsIntersectAfterBounds(line, rectangle.m_Line1) ||
			IsIntersectAfterBounds(line, rectangle.m_Line2) ||
			IsIntersectAfterBounds(line, rectangle.m_Line3) ||
			IsIntersectAfterBounds(line, rectangle.m_Line4);
	}

	
	// Fixes fallen trees
	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] previousPoints2D
	//! \param[in] outlineChecker
	//! \param[in] forceRegeneration
	//! \return
	override int GenerateEntities(notnull array<float> previousPoints2D, SCR_ForestGeneratorOutlinePositionChecker outlineChecker, bool forceRegeneration = false)
	{
		WorldEditorAPI worldEditorAPI = _WB_GetEditorAPI();

		BaseWorld world = worldEditorAPI.GetWorld();
		if (!world)
			return 0;

		// create new trees
		Debug.BeginTimeMeasure();
		RefreshObstacles();
		Debug.EndTimeMeasure("Obstacles scan");

		// draw obstacles
		if (m_bDrawDebugShapesObstacles)
		{
			foreach (SCR_ObstacleDetectorSplineInfo info : s_ObstacleDetector.GetObstacles())
			{
				if (info.m_fClearance == 0) // an area obstacle
				{
					s_DebugShapeManager.AddAABBRectangleXZ(info.m_vMinWithClearance, info.m_vMaxWithClearance);
				}
				else // a road-like obstacle
				{
					vector maxWithClearance;
					foreach (int index, vector minWithClearance : info.m_aMinsWithClearance)
					{
						maxWithClearance = info.m_aMaxsWithClearance[index];
						s_DebugShapeManager.AddAABBRectangleXZ(minWithClearance, maxWithClearance);
					}
				}
			}
		}

		bool partialGeneration = !forceRegeneration && m_bAllowPartialRegeneration && m_Source.GetNumChildren() > 0;

		bool useScaleCurve = m_fGlobalOutlineScaleCurveDistance > 0 && !m_aGlobalOutlineScaleCurve.IsEmpty();
		float scaleCurveDistanceDivisor;
		array<float> curveKnots;
		if (useScaleCurve)
		{
			// scaleCurveDistanceDivisor = 1 / m_fGlobalOutlineScaleCurveDistance;
			scaleCurveDistanceDivisor = SCALE_CURVE_RANGE / m_fGlobalOutlineScaleCurveDistance;
			// scaleCurveDistanceDivisor = 1;
			curveKnots = {};
			foreach (vector scalePoint : m_aGlobalOutlineScaleCurve)
			{
				curveKnots.Insert(scalePoint[0]);
			}
		}

		// init done

		int topLevelEntitiesCount, bottomLevelEntitiesCount;
		int smallOutlineEntitiesCount, middleOutlineEntitiesCount;
		int clusterEntitiesCount, otherEntitiesCount;
		int generatedEntitiesCount;
		int setVariableValueCalls;

		TraceParam traceParam = new TraceParam();
		traceParam.Flags = TraceFlags.WORLD;

		map<ResourceName, ref SCR_RandomisationEditorData> randomValuesMap = new map<ResourceName, ref SCR_RandomisationEditorData>();
		SCR_RandomisationEditorData randomisationData;
		SCR_ForestGeneratorTreeBase baseEntry;
		IEntitySource entitySource;
		vector entityMatrix[4];
		FallenTree fallenTree;
		WideForestGeneratorClusterObject wideObject;

		for (int i, count = m_Grid.GetEntryCount(); i < count; ++i)
		{
			vector worldPos;
			baseEntry = m_Grid.GetEntry(i, worldPos);
			if (!baseEntry.m_Prefab) // .IsEmpty()
				continue;

			worldPos[1] = world.GetSurfaceY(worldPos[0], worldPos[2]);							// snaps to ground here
			vector localPos = CoordToLocal(worldPos);
			localPos[1] = localPos[1] + baseEntry.m_fVerticalOffset;

			if (partialGeneration) // TODO: move to Grid population instead?
			{
				if (
					Math2D.IsPointInPolygon(previousPoints2D, localPos[0], localPos[2]) &&		// if in the old shape
					!outlineChecker.IsPosWithinSetDistance(localPos)							// and not near a new outline
				)
					continue;
			}

			float scale = baseEntry.m_fScale;
			if (useScaleCurve && scale > 0)
			{
				float distanceFromShape = SCR_Math3D.GetDistanceFromSplineXZ(m_aShapePoints, localPos);
				if (distanceFromShape <= m_fGlobalOutlineScaleCurveDistance)
				{
					// (m_fOutlineScaleCurveDistance - distanceFromShape) because right-to-left curve reading
					float scaleFactor = LegacyCurve.Curve(ECurveType.CatmullRom, (m_fGlobalOutlineScaleCurveDistance - distanceFromShape) * scaleCurveDistanceDivisor, m_aGlobalOutlineScaleCurve, curveKnots)[1];

					if (scaleFactor < SCALE_CURVE_MIN_VALUE)
						scaleFactor = SCALE_CURVE_MIN_VALUE;
					else
					if (scaleFactor > SCALE_CURVE_MAX_VALUE)
						scaleFactor = SCALE_CURVE_MAX_VALUE;

					scale *= scaleFactor;
				}
			}

			if (scale < MIN_POSSIBLE_SCALE_VALUE)
			{
				PrintFormat("Avoiding near-zero scale tree (scale = %1 < %2)", scale, MIN_POSSIBLE_SCALE_VALUE, level: LogLevel.DEBUG);
				continue;
			}

			if (s_Benchmark)
				s_Benchmark.BeginMeasure("obstacleDetection");

			// providing a generated entities list would hinder performance here
			bool hasObstacle = s_ObstacleDetector.HasObstacle(worldPos);

			if (s_Benchmark)
				s_Benchmark.EndMeasure("obstacleDetection");

			if (hasObstacle)
				continue;

			if (partialGeneration)
			{
				if (m_bDrawDebugShapesRegeneration)
					s_DebugShapeManager.AddLine(worldPos, worldPos + DEBUG_VERTICAL_LINE, REGENERATION_CREATION_COLOUR);
			}

			if (s_Benchmark)
				s_Benchmark.BeginMeasure("createEntity");

			entitySource = worldEditorAPI.CreateEntity(baseEntry.m_Prefab, string.Empty, m_iSourceLayerID, m_Source, localPos, vector.Zero);

			if (s_Benchmark)
				s_Benchmark.EndMeasure("createEntity");

			generatedEntitiesCount++;
			switch (baseEntry.m_eType)
			{
				case SCR_ETreeType.TOP:				topLevelEntitiesCount++;		break;
				case SCR_ETreeType.BOTTOM:			bottomLevelEntitiesCount++;		break;
				case SCR_ETreeType.MIDDLE_OUTLINE:	middleOutlineEntitiesCount++;	break;
				case SCR_ETreeType.SMALL_OUTLINE:	smallOutlineEntitiesCount++;	break;
				case SCR_ETreeType.CLUSTER:			clusterEntitiesCount++;			break;
				default:							otherEntitiesCount++;			break;
			}

			if (m_bDrawDebugShapes)
				s_DebugShapeManager.AddSphere(worldPos, 1 + 5 - (int)baseEntry.m_eType, GetColorForTree(baseEntry.m_iGroupIndex, baseEntry.m_eType), ShapeFlags.NOOUTLINE);
				// 1 + 5 because 5x SCR_ETreeType types

			worldEditorAPI.SourceToEntity(entitySource).GetTransform(entityMatrix);

			if (s_Benchmark)
				s_Benchmark.BeginMeasure("editSequenceCalculation");

			float randomVerticalOffset;
			vector randomAngles; // pitch yaw roll
			bool alignToNormal;

			if (!randomValuesMap.Find(baseEntry.m_Prefab, randomisationData))
			{
				randomisationData = SCR_RandomisationEditorData.CreateFromEntitySource(entitySource);
				randomValuesMap.Insert(baseEntry.m_Prefab, randomisationData);
			}

			if (randomisationData)
			{
				randomVerticalOffset = SafeRandomFloatInclusive(randomisationData.m_vRandomVertOffset[0], randomisationData.m_vRandomVertOffset[1]);

				randomAngles[0] = SafeRandomFloatInclusive(-randomisationData.m_fRandomPitchAngle, randomisationData.m_fRandomPitchAngle);
				randomAngles[1] = SafeRandomFloatInclusive(0, 360);
				randomAngles[2] = SafeRandomFloatInclusive(-randomisationData.m_fRandomRollAngle, randomisationData.m_fRandomRollAngle);

				alignToNormal = randomisationData.m_bAlignToNormal;
			}
			else
			{
				if (baseEntry.m_fVerticalOffset > 0)
					randomVerticalOffset = SafeRandomFloatInclusive(-baseEntry.m_fVerticalOffset, baseEntry.m_fVerticalOffset);

				if (baseEntry.m_fRandomPitchAngle > 0)
					randomAngles[0] = SafeRandomFloatInclusive(-baseEntry.m_fRandomPitchAngle, baseEntry.m_fRandomPitchAngle);

				randomAngles[1] = SafeRandomFloatInclusive(0, 360);

				if (baseEntry.m_fRandomRollAngle > 0)
					randomAngles[2] = SafeRandomFloatInclusive(-baseEntry.m_fRandomRollAngle, baseEntry.m_fRandomRollAngle);

				// alignToNormal = false;
			}

			fallenTree = FallenTree.Cast(baseEntry);
			wideObject = WideForestGeneratorClusterObject.Cast(baseEntry);

			if (wideObject)
				alignToNormal = wideObject.m_bAlignToNormal;
			else
			if (fallenTree) {
				alignToNormal = fallenTree.m_bAlignToNormal;
			}
			
			// yaw first
			if (randomAngles[1] != 0)
				SCR_Math3D.RotateAround(entityMatrix, entityMatrix[3], entityMatrix[1], -Math.DEG2RAD * randomAngles[1], entityMatrix);
				
			if (randomAngles[0] != 0)
				SCR_Math3D.RotateAround(entityMatrix, entityMatrix[3], entityMatrix[0], -Math.DEG2RAD * randomAngles[0], entityMatrix);
	
			if (randomAngles[2] != 0)
				SCR_Math3D.RotateAround(entityMatrix, entityMatrix[3], entityMatrix[2], -Math.DEG2RAD * randomAngles[2], entityMatrix);
			
			if (alignToNormal)
			{
				traceParam.Start = worldPos + vector.Up;
				traceParam.End = worldPos - vector.Up;
				world.TraceMove(traceParam, null);

				entityMatrix[1] = traceParam.TraceNorm.Normalized();					// newUp
				entityMatrix[0] = (entityMatrix[1] * entityMatrix[2]).Normalized();		// newRight
				entityMatrix[2] = (entityMatrix[0] * entityMatrix[1]).Normalized();		// newForward
			}


			vector angles = Math3D.MatrixToAngles(entityMatrix);
			
			//THIS IS THE MODDED CHANGE: Just fix fallen trees for now
			if (fallenTree) {
				angles[1] = 0;
				angles[2] = 0;
			}
			
			if (s_Benchmark)
				s_Benchmark.EndMeasure("editSequenceCalculation");

			if (s_Benchmark)
				s_Benchmark.BeginMeasure("editSequence");

			if (angles != vector.Zero || scale != 1)
			{
				worldEditorAPI.BeginEditSequence(entitySource);

				if (angles != vector.Zero)
					worldEditorAPI.SetVariableValue(entitySource, null, "angles", string.Format("%1 %2 %3", angles[1], angles[0], angles[2]));

				if (scale != 1)
					worldEditorAPI.SetVariableValue(entitySource, null, "scale", scale.ToString());

				worldEditorAPI.EndEditSequence(entitySource);
			}
			
			if (s_Benchmark)
				s_Benchmark.EndMeasure("editSequence");
		}

		ClearObstacles(); // frees RAM

		if (m_bPrintArea)
			Print("Area of the polygon is: " + m_fArea.ToString(lenDec: 2) + " square meters", LogLevel.NORMAL);

		if (m_bPrintEntitiesCount)
		{
			Print("Forest generator generated: " + topLevelEntitiesCount + " entities in top level", LogLevel.NORMAL);
			Print("Forest generator generated: " + bottomLevelEntitiesCount + " entities in bottom level", LogLevel.NORMAL);
			Print("Forest generator generated: " + middleOutlineEntitiesCount + " entities in middle outline", LogLevel.NORMAL);
			Print("Forest generator generated: " + smallOutlineEntitiesCount + " entities in small outline", LogLevel.NORMAL);
			Print("Forest generator generated: " + clusterEntitiesCount + " entities in clusters", LogLevel.NORMAL);
			if (otherEntitiesCount > 0)
				Print("Forest generator generated: " + clusterEntitiesCount + " uncategorised entities", LogLevel.WARNING);

			Print("Forest generator generated: " + generatedEntitiesCount + " entities in total (" + setVariableValueCalls + " WE API calls)", LogLevel.NORMAL);
		}

		return generatedEntitiesCount;
	}
	
	// Override handles tree swaps
	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] polygon
	//! \param[in] bbox
	//! \param[in] topLevel
	protected override void GenerateTopTrees(array<float> polygon, SCR_AABB bbox, ForestGeneratorTopLevel topLevel)
	{
		if (!topLevel || topLevel.m_aTreeGroups.IsEmpty())
			return;

		if (!GetIsAnyTreeValid(topLevel.m_aTreeGroups))
			return;

		array<float> groupProbas = {};
		array<float> groupCounts = {};
		groupCounts.Resize(topLevel.m_aTreeGroups.Count() + TREE_GROUPS_OFFSET_HACK);

		vector worldMat[4];
		GetWorldTransform(worldMat);

		bool useScaleCurve = topLevel.m_fOutlineScaleCurveDistance > 0 && !topLevel.m_aOutlineScaleCurve.IsEmpty();
		float scaleCurveDistanceDivisor;
		array<float> curveKnots;

		if (useScaleCurve)
		{
			scaleCurveDistanceDivisor = SCALE_CURVE_RANGE / topLevel.m_fOutlineScaleCurveDistance;
			curveKnots = {};
			foreach (vector scalePoint : topLevel.m_aOutlineScaleCurve)
			{
				curveKnots.Insert(scalePoint[0]);
			}
		}

		vector pointLocal;
		int expectedIterCount = m_fArea * HECTARE_CONVERSION_FACTOR * topLevel.m_fDensity;
		vector point;
		ForestGeneratorTree tree;

		foreach (SCR_ForestGeneratorRectangle rectangle : m_aRectangles)
		{
			float area = rectangle.m_fArea * HECTARE_CONVERSION_FACTOR;
			int iterCount = topLevel.m_fDensity * area;

			for (int treeIdx; treeIdx < iterCount; ++treeIdx)
			{
				// generate random point inside the shape (polygon at first)
				pointLocal = GenerateRandomPointInRectangle(rectangle);
				expectedIterCount--;

				if (!rectangle.m_aLines.IsEmpty())
				{
					if (!Math2D.IsPointInPolygon(polygon, pointLocal[0], pointLocal[2]))
						continue;

					if (!IsBeforeOutlineBorder(rectangle, pointLocal, topLevel.m_fOutlineOverlap))
						continue;
				}

				point = pointLocal.Multiply4(worldMat);

				// see which trees are around - count the types
				groupProbas.Copy(topLevel.m_aGroupProbas);
				for (int i, count = groupCounts.Count(); i < count; i++)
				{
					groupCounts[i] = 1;
				}

				if (s_Benchmark)
					s_Benchmark.BeginMeasure("gridCountEntriesAround");

				// HERE is the "Invalid tree group index: 1, there are only 1 groups" error source
				m_Grid.CountEntriesAround(point, topLevel.m_fClusterRadius, groupCounts);

				if (s_Benchmark)
					s_Benchmark.EndMeasure("gridCountEntriesAround");

				// skew the probability of given groups based on counts
				float probaSumToNormalize = 0;
				for (int i, count = groupProbas.Count(); i < count; i++)
				{
					groupProbas[i] = groupProbas[i] * Math.Pow(groupCounts[i], topLevel.m_fClusterStrength);
					probaSumToNormalize += groupProbas[i];
				}

				if (probaSumToNormalize != 0)
				{
					for (int i, count = groupProbas.Count(); i < count; i++)
					{
						groupProbas[i] = groupProbas[i] / probaSumToNormalize;
					}
				}

				float groupProba = m_RandomGenerator.RandFloat01();
				int groupIdx = groupProbas.Count() - 1; // last because there is less than in the loop
				float probaSum = 0;
				for (int i, count = groupProbas.Count(); i < count; ++i)
				{
					probaSum += groupProbas[i];
					if (groupProba < probaSum) // less than to avoid accepting 0 probability tree
					{
						groupIdx = i;
						break;
					}
				}

				tree = SelectTreeToSpawn(point, topLevel.m_aTreeGroups[groupIdx].m_aTrees);
				//if(tree) PrintFormat("Tree before: %1", tree.m_Prefab);
				if (!IsEntryValidAdvanced(tree, pointLocal)) //TODO: handle swap directly here so better performance
					continue;
				
				
				//PrintFormat("Tree after: %1", tree.m_Prefab);
				tree.m_eType = SCR_ETreeType.TOP;
				if (useScaleCurve)
				{
					float distanceFromShape = SCR_Math3D.GetDistanceFromSplineXZ(m_aShapePoints, pointLocal);
					if (distanceFromShape <= topLevel.m_fOutlineScaleCurveDistance)
					{
						// (m_fOutlineScaleCurveDistance - distanceFromShape) because right-to-left curve reading
						float scaleFactor = LegacyCurve.Curve(ECurveType.CatmullRom, (topLevel.m_fOutlineScaleCurveDistance - distanceFromShape) * scaleCurveDistanceDivisor, topLevel.m_aOutlineScaleCurve, curveKnots)[1];
						if (scaleFactor < ForestGeneratorLevel.SCALE_CURVE_MIN_VALUE)
							scaleFactor = ForestGeneratorLevel.SCALE_CURVE_MIN_VALUE;
						else
						if (scaleFactor > ForestGeneratorLevel.SCALE_CURVE_MAX_VALUE)
							scaleFactor = ForestGeneratorLevel.SCALE_CURVE_MAX_VALUE;

						tree.m_fScale *= scaleFactor;
					}
				}

				m_Grid.AddEntry(tree, point);
			}
		}

		int index;
		while (expectedIterCount > 0)
		{
			index = m_RandomGenerator.RandInt(0, m_aRectangles.Count());
			GenerateTreeInsideRectangle(m_aRectangles[index], topLevel, polygon, worldMat);
			expectedIterCount--;
		}
	}
		
	protected float PointLineSegmentDistanceSqrXZ(SCR_ForestGeneratorLine line, vector P)
	{
		vector A = line.p1.m_vPos;
		vector B = line.p2.m_vPos;
		
		vector ab = B - A;
		vector ap = P - A;
		
		float abSqr = vector.DistanceSqXZ(B, A); //We use DistanceSqXZ because there is not a LengthSqXZ
		if(abSqr < 0.0001) // a and b are the same point
			return vector.DistanceSqXZ(P, A);
		
		float t = vector.DotXZ(ab, ap) / abSqr;
		t = Math.Clamp(t, 0, 1);
		
		vector C = A + (ab * t); // We interpolate Y too here, but discard it at the distance
		return vector.DistanceSqXZ(P, C);
	}
	
	#endif
}