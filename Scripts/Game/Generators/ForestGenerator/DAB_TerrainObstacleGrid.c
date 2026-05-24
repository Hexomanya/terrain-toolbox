class DAB_TerrainObstacleGrid
{
	#ifdef WORKBENCH
	
	const float INVALID_SLOPE = - 1;
	protected const ref array<ref array<int>> TILE_EDGE_LOOKUP = {};
	
	protected float m_fCellSize = - 1;
	protected vector m_vOriginLocal; // Local offset from parent
	protected vector m_vOriginWorld; // World position
	protected vector m_aWorldTransform[4];
	protected vector m_aInvWorldTransform[4];
	
	protected int m_iRows;
	protected int m_iColumns;
	
	protected ref array<ref DAB_TerrainObstaclePoint> m_aPoints = {};
	protected ref array<ref DAB_OutlinePoint> m_aOutlinePoints = {}; // For quick iteration
	protected ref map<int, ref DAB_OutlinePoint> m_mOutlinePoints = new map<int, ref DAB_OutlinePoint>(); // For lookup
	protected ref array<ref DAB_Outline> m_aSlopeOutlines = {};
	//------------------------------------------------------------------------------------------------
	void DAB_TerrainObstacleGrid()
	{		
		SetupTileEdges();
	}
	
	//------------------------------------------------------------------------------------------------
	void RegenerateGrid(
		float cellSize,
		notnull SCR_AABB bbox,
		notnull array<float> polygon2D,
		vector worldMat[4],
		BaseWorld world,
		float maxSlope,
		bool enableSlopeOutlines,
		float minOutlineArea,
		float maxPointRemovalError
	){
		ClearGrid();
		
		m_fCellSize = cellSize;
		if(m_fCellSize <= 0.001)
		{
			PrintFormat("Cell size can not be smaller than 0! It currently is: %1", m_fCellSize, LogLevel.WARNING);
			return;
		}
		
		m_vOriginLocal = bbox.m_vMin;
		m_vOriginWorld = m_vOriginLocal.Multiply4(worldMat);
		m_aWorldTransform = worldMat;
		Math3D.MatrixGetInverse4(worldMat, m_aInvWorldTransform);
		
		// bbox.m_vDimensions = width, height, depth
		m_iRows = Math.Ceil(bbox.m_vDimensions[0] / m_fCellSize) + 1; // +1 Because if a point is close to the bounding box, we are not guaranteed a closed loop
		m_iColumns = Math.Ceil(bbox.m_vDimensions[2] / m_fCellSize) + 1; // +1 Because if a point is close to the bounding box, we are not guaranteed a closed loop

		array<int> slopeTileIndices = {};
		
		for(int x = 0; x < m_iRows; x++)
		{
			for(int z = 0; z < m_iColumns; z++)
			{
				vector localPointPosition = GetLocalPointPosition(x,z);
				
				bool isOutside = !Math2D.IsPointInPolygon(polygon2D, localPointPosition[0], localPointPosition[2]);
				
				float slope = INVALID_SLOPE;
				if(!isOutside)
				{
					slope = ComputePointSlope(localPointPosition, world);
					if(slope >= maxSlope) slopeTileIndices.Insert(m_aPoints.Count());
				}
				
				m_aPoints.Insert(new DAB_TerrainObstaclePoint(isOutside, slope));
			}
		}
		if(maxSlope <= 0) return; //This would cause a double outline
		if(!enableSlopeOutlines) return;
		
		BuildOutlineEdges(world, maxSlope);
		BuildOutlines();
		OptimizeOutlines(minOutlineArea, maxPointRemovalError);
		ComputeOutlineDirection();
	}
	
	// Default Orientation is CW for 1, 0 is the default forest outline
	protected void ComputeOutlineDirection()
	{
		array<ref array<int>> outlineContainedBy = {};
		array<ref array<float>> cachedFormats = {};
		
		for (int j = 0; j < m_aSlopeOutlines.Count(); j++)
		{
			cachedFormats.Insert(m_aSlopeOutlines[j].ToXZFloatFormat());
		}
		
		for(int i = 0; i < m_aSlopeOutlines.Count(); i++)
		{
			outlineContainedBy.Insert({});
			
			for(int j = 0; j < m_aSlopeOutlines.Count(); j++)
			{
				if(j == i) continue;
				if(j < i && outlineContainedBy[j].Contains(i)) continue; //If j is already contained by i, j can not contain i.

				vector firstPoint = m_aSlopeOutlines[i].m_aPoints[0].m_vWorldPosition;
				
				if(!Math2D.IsPointInPolygon(cachedFormats[j], firstPoint[0], firstPoint[2])) continue;
			
				outlineContainedBy[i].Insert(j);
			}
			
			if((outlineContainedBy[i].Count() + 1) % 2 == 0)
				m_aSlopeOutlines[i].ReverseOrientation();
		}
	}
	
	void CompileForestGeneratorData(
		notnull out array<ref DAB_ForestOutline> compiledOutlines, 
		float smallOutlineMinArea,
		float middleOutlineMinArea,
	){
		DAB_ForestOutline forestOutline;
		DAB_OutlinePoint currentOutlinePoint;
		SCR_ForestGeneratorPoint forestPoint;
		SCR_ForestGeneratorPoint currentForestPoint;
		SCR_ForestGeneratorPoint nextPoint;
		SCR_ForestGeneratorLine forestLine;
		
		foreach(int j, DAB_Outline outline : m_aSlopeOutlines)
		{
			forestOutline = new DAB_ForestOutline();
			int count = outline.m_aPoints.Count();
			
			bool isSmallOutline = outline.m_fArea >= smallOutlineMinArea;
			bool isMiddleOutline = outline.m_fArea >= middleOutlineMinArea;
			
			if(!isSmallOutline && !isMiddleOutline)
			{
				Print("Found outline that is to small for a small and middle outline. That should have been filtered before!", LogLevel.ERROR);
				continue;
			}
			
			for(int i = 0; i < count; i++)
			{
				currentOutlinePoint = outline.m_aPoints[i];

				forestPoint = new SCR_ForestGeneratorPoint();
				forestPoint.m_vPos = WorldToLocal(currentOutlinePoint.m_vWorldPosition);
				forestPoint.m_bSmallOutline = isSmallOutline;
				forestPoint.m_bMiddleOutline = isMiddleOutline;
				forestOutline.AddPoint(forestPoint, isSmallOutline, isMiddleOutline);
			}
			
			for(int i = 0; i < count; i++)
			{
				currentForestPoint = forestOutline.m_aAllOutlinePoints[i];
				nextPoint = forestOutline.m_aAllOutlinePoints[(i + 1) % count];
				
				forestLine = new SCR_ForestGeneratorLine();
				forestLine.p1 = currentForestPoint;
				forestLine.p2 = nextPoint;
				forestLine.m_fLength = (forestLine.p2.m_vPos - forestLine.p1.m_vPos).Length();
				forestLine.m_bIsSlopeLine = true;
				forestOutline.AddLine(forestLine, isSmallOutline, isMiddleOutline);
				
				//Incoming line is 1, outgoing is 2
				nextPoint.m_Line1 = forestLine;
				currentForestPoint.m_Line2 = forestLine;
			}

			CalculateInvertedOutlineAnglesForPoints(forestOutline.m_aAllOutlinePoints);
			
			compiledOutlines.Insert(forestOutline);
		}
	}

	protected void OptimizeOutlines(float minOutlineArea, float maxPointRemovalError)
	{
		FilterOutlinesByArea(minOutlineArea);
		FilterUnrelevantPoints(maxPointRemovalError);
	}
	
	protected static float PointToSegmentDistance(vector A, vector B, vector P)
	{
	    vector ab = B - A;
	    vector ap = P - A;
	
	    float lenSq = ab[0] * ab[0] + ab[2] * ab[2];
	    if (lenSq < 0.0001)
	        return vector.Distance(P, A);
	
	    float t = (ap[0] * ab[0] + ap[2] * ab[2]) / lenSq;
	    t = Math.Clamp(t, 0.0, 1.0);
	
	    vector Closest;
	    Closest[0] = A[0] + t * ab[0];
	    Closest[1] = A[1] + t * ab[1];
	    Closest[2] = A[2] + t * ab[2];
	
	    return vector.Distance(P, Closest);
	}

	protected void FilterUnrelevantPoints(float maxPointRemovalError)
	{		
		int pointRemovedCount = 0;
		DAB_Outline outline;
		array<float> removalErrors = {};
		DAB_OutlinePoint prevPoint;
		DAB_OutlinePoint nextPoint;
		
		for(int i = 0; i < m_aSlopeOutlines.Count(); i++)
		{
			outline = m_aSlopeOutlines[i];
			removalErrors.Clear();
			
			ComputeRemovalErrors(outline.m_aPoints, removalErrors);
			int pointToRemove = GetSmallestOffender(removalErrors, maxPointRemovalError);
			
			while(pointToRemove != -1)
			{
				if(outline.m_aPoints.Count() <= 3)
        			break;
				
				int count = outline.m_aPoints.Count();
				int prev = (pointToRemove - 1 + count) % count;
	        	int next = (pointToRemove + 1) % count;
				
				prevPoint = outline.m_aPoints[prev];
				nextPoint = outline.m_aPoints[next];
				int removedKey = outline.m_aPoints[pointToRemove].m_iKey;
				
				if(nextPoint.GetEdgeKey(1) == removedKey) nextPoint.SetEdgeKey(1, prevPoint.m_iKey);
				else if(nextPoint.GetEdgeKey(2) == removedKey) nextPoint.SetEdgeKey(2, prevPoint.m_iKey);
				else PrintFormat("Error finding deleted point key on nextPoint. Key was %1", removedKey, LogLevel.ERROR);
				
				if(prevPoint.GetEdgeKey(1) == removedKey) prevPoint.SetEdgeKey(1, nextPoint.m_iKey);
				else if(prevPoint.GetEdgeKey(2) == removedKey) prevPoint.SetEdgeKey(2, nextPoint.m_iKey);
				else PrintFormat("Error finding deleted point key on prevPoint. Key was %1", removedKey, LogLevel.ERROR);
				
				outline.m_aPoints.RemoveOrdered(pointToRemove);	
				removalErrors.RemoveOrdered(pointToRemove);
				pointRemovedCount++;
						
				count = removalErrors.Count();
				UpdateRemovalErrors(outline.m_aPoints, removalErrors, prev % count, pointToRemove % count);
				if(outline.m_aPoints.Count() != removalErrors.Count())
				{
					Print("Dsync between two arrays", LogLevel.ERROR);
					return;
				}
				
				pointToRemove = GetSmallestOffender(removalErrors, maxPointRemovalError);
			}
		}
		
		PrintFormat("Removed %1 points that landed under an error of %2", pointRemovedCount, maxPointRemovalError);
	}
	
	//nextIndex after removal is the same as current index before removal 
	protected static void UpdateRemovalErrors(array<ref DAB_OutlinePoint> points, notnull out array<float> errors, int prevIndex, int nextIndex)
	{
		if(points.Count() != errors.Count())
			PrintFormat("Dsync in array count! points have %1 while errors have %2", points.Count(), errors.Count(), LogLevel.ERROR);
		
		errors[prevIndex] = ComputeRemovalErrorForPoint(points, prevIndex);
		errors[nextIndex] = ComputeRemovalErrorForPoint(points, nextIndex);
	}
	
	protected static float ComputeRemovalErrorForPoint(array<ref DAB_OutlinePoint> points, int currentPointIndex)
	{
		int count = points.Count();
		int prev = (currentPointIndex - 1 + count) % count;
        int next = (currentPointIndex + 1) % count;
		
		if(prev == currentPointIndex || next == currentPointIndex || prev == next)
			PrintFormat("ComputeRemovalErrors: Error when calculating prev, next. currentPointIndex: %1, count: %2", currentPointIndex, count, LogLevel.ERROR);
		
		vector prevPointPos = points[prev].m_vWorldPosition;
		vector nextPointPos = points[next].m_vWorldPosition;
		vector pointPos = points[currentPointIndex].m_vWorldPosition;

        return PointToSegmentDistance(prevPointPos, nextPointPos, pointPos);
	}
	
	protected static void ComputeRemovalErrors(array<ref DAB_OutlinePoint> points,notnull out array<float> errors)
	{
	    errors.Clear();
	    int count = points.Count();
	
	    for (int i = 0; i < count; i++)
	    {
	        float error = ComputeRemovalErrorForPoint(points, i);
			errors.Insert(error);
	    }
	}
	
	protected int GetSmallestOffender(array<float> removalErrors, float maxPointRemovalError)
	{
		int smallestOffender = -1;
		float smallestError = float.MAX;
	
		for(int i = 0; i < removalErrors.Count(); i++)
		{
			float error = removalErrors[i];
			if(error > maxPointRemovalError) continue;
			if(error >= smallestError) continue;
			
			smallestOffender = i;
			smallestError = error;
		}
		
		return smallestOffender;
	}
	
	protected void FilterOutlinesByArea(float minOutlineArea)
	{
		int count = m_aSlopeOutlines.Count();
		
		for(int i = 0; i < count; i++)
		{
			float area = GetOutlinedAreaXZ(m_aSlopeOutlines[i]);
		
			if(area < 0)
			{
				m_aSlopeOutlines[i].ReverseOrientation(); // Force every outline into CCW, holes will be handled later, just need a constant orientation for now
				area = Math.AbsFloat(area);
			}
	
			if(area >= minOutlineArea)
				m_aSlopeOutlines[i].m_fArea = area;
		}
		
		for(int j = count - 1; j >= 0; j--)
		{
			if(float.AlmostEqual(m_aSlopeOutlines[j].m_fArea, -1))
				m_aSlopeOutlines.Remove(j);
		}
		
		PrintFormat("Removed %1 outlines because their areas were too small.", count - m_aSlopeOutlines.Count(), LogLevel.NORMAL);
	}
	
	protected void ReversePointOrder(int outlineIndex)
	{
		DAB_Outline outline =  m_aSlopeOutlines[outlineIndex];
		int count = outline.m_aPoints.Count();
		float iterNum = count * 0.5;
		DAB_OutlinePoint pointSave;
		
		for (int i = 0; i < iterNum; i++)
		{
			pointSave = outline.m_aPoints[i];
			outline.m_aPoints[i] = outline.m_aPoints[count - 1 - i];
			outline.m_aPoints[count - 1 - i] = pointSave;
		}
	}

	// Shoelace + = CW; - = CCW
	protected float GetOutlinedAreaXZ(DAB_Outline outline)
	{
		int count = outline.m_aPoints.Count();
		if(count < 3) return 0;
		
		float areaSum = 0;
		
		for(int i = 0; i < count; i++)
		{
			vector a = outline.m_aPoints[i].m_vWorldPosition;
			vector b = outline.m_aPoints[(i + 1) % count].m_vWorldPosition;
			areaSum += a[0] * b[2] - b[0] * a[2];
		}
		
		return areaSum * 0.5;
	}
	
	protected void BuildOutlines()
	{
		m_aSlopeOutlines.Clear();
		if(m_aOutlinePoints.Count() <= 0) return;
	
		array<ref DAB_OutlinePoint> remainingPoints = {};
		foreach(DAB_OutlinePoint point : m_aOutlinePoints) //Because copy did not work
		{
			remainingPoints.Insert(point);
		}
		
		DAB_Outline outline;
		DAB_OutlinePoint startPoint;
		DAB_OutlinePoint nextPoint;
			
		bool isPointRemaining = remainingPoints.Count() > 0;
		while(isPointRemaining)
		{
			outline = new DAB_Outline();
			startPoint = remainingPoints[0];
			outline.m_aPoints.Insert(startPoint);
			remainingPoints.RemoveItem(startPoint);
			
			int startKey = startPoint.m_iKey;
			int beforeKey = startPoint.m_iKey;
			int nextKey = startPoint.GetEdgeKey(1);
			
			// Outlines should always be closed, otherwise this might crash
			while(nextKey != startKey)
			{
				if(!m_mOutlinePoints.Find(nextKey, nextPoint))
				{
					PrintFormat("Could not find nextKey %1 in map. Aborting...", nextKey, LogLevel.ERROR);
					return;
				}

				
				int safeNextKey = nextKey;				
				nextKey = -1;

				if(nextPoint.GetEdgeKey(1) != beforeKey) nextKey = nextPoint.GetEdgeKey(1);
				if(nextPoint.GetEdgeKey(2) != beforeKey) nextKey = nextPoint.GetEdgeKey(2);
				
				if(nextKey == -1)
				{
					Print("Could not find next edge, this indicates an error in outline generation! Aborting...", LogLevel.ERROR);
					return;
				}
				
				outline.m_aPoints.Insert(nextPoint);
				remainingPoints.RemoveItem(nextPoint);
				
				beforeKey = safeNextKey;
			}
			
			if(outline.m_aPoints.Count() <= 0)
			{
				Print("Generated empty outline!", LogLevel.ERROR);
			} else {
				m_aSlopeOutlines.Insert(outline);
			}
			
			isPointRemaining = remainingPoints.Count() > 0;
		}
		
		PrintFormat("Found %1 seperate outlines!", m_aSlopeOutlines.Count());
	}
	
	//------------------------------------------------------------------------------------------------
	protected float ComputePointSlope(vector localPointPosition, BaseWorld world)
	{
		vector worldPosition = LocalToWorld(localPointPosition);
		vector normal = SCR_TerrainHelper.GetTerrainNormal(worldPosition, world);
		
		if (normal.LengthSq() < 0.0001)
			return INVALID_SLOPE;
		
		normal = normal.Normalized();
		return Math.RAD2DEG * Math.Acos(vector.Dot(Vector(0, 1, 0), normal)); 
	}
	
	//------------------------------------------------------------------------------------------------
	protected vector GetLocalPointPosition(int x, int z)
	{
		return Vector(m_vOriginLocal[0] + x * m_fCellSize, 0, m_vOriginLocal[2] + z * m_fCellSize);
	} 
	
	//------------------------------------------------------------------------------------------------
	protected void BuildOutlineEdges(BaseWorld world, float maxSlope)
	{
		m_aOutlinePoints.Clear();
		m_mOutlinePoints.Clear();
		
		array<ref DAB_TerrainObstaclePoint> tilePoints = {}; // Needs to be dynamic otherwise we get (bugged?) type error 
		int tilePointIndices[4];
		int edgeMidPointKeys[4];
		vector edgeMidPointWorldPositions[4];
		array<int> edges;		
		DAB_OutlinePoint pointA;
		DAB_OutlinePoint pointB;
		
		for(int x = 0; x < m_iRows - 1; x++)
		{
			for(int z = 0; z < m_iColumns - 1; z++)
			{
				int currentPoint = GetIndexFromGridPos(x, z);
				tilePoints.Clear();
				
				if(!TryGetTilePoints(currentPoint, tilePointIndices, tilePoints))
				{
					PrintFormat("Could not retrieve tile points from point: [%1, %2]", x, z, LogLevel.ERROR);
					return;
				}
				
				int state = GetTileState(tilePoints, maxSlope, currentPoint < 1000); // 4 bit state, stored in int because we do not have something better
				if(state <= 0 || state >= 15) continue;
				
				edges = TILE_EDGE_LOOKUP[state];
				GetEdgeMidPointKeys(x, z, edgeMidPointKeys);
				GetEdgeMidPointWorldPositions(x, z, maxSlope, world, tilePoints, edgeMidPointWorldPositions);
						
				for (int i = 0; i < 4; i += 2)
	            {
	                int a = edges[i];
	                int b = edges[i + 1];
	                if (a < 0 || b < 0) continue;
					
					int keyA = edgeMidPointKeys[a];
					int keyB = edgeMidPointKeys[b];
					vector positionA = edgeMidPointWorldPositions[a];
					vector positionB = edgeMidPointWorldPositions[b];

					if(!m_mOutlinePoints.Find(keyA, pointA))
						CreateAndRegisterNewOutlinePoint(keyA, positionA, pointA);
					
					if(!m_mOutlinePoints.Find(keyB, pointB))
						CreateAndRegisterNewOutlinePoint(keyB, positionB, pointB);
					
					if(pointA.m_iKey == -1 || pointB.m_iKey == -1)
						PrintFormat("Inserted invalid key: A: %1, B: %2; i: %3, x: %4, z: %5", pointA.m_iKey, pointB.m_iKey, i, x, z, LogLevel.ERROR);
		
					pointA.InsertEdgePoint(pointB.m_iKey);
					pointB.InsertEdgePoint(pointA.m_iKey);
	            }
			}
		}
	}
	
	protected void CreateAndRegisterNewOutlinePoint(int key, vector worldPosition, out DAB_OutlinePoint newPoint)
	{
		newPoint = new DAB_OutlinePoint(key, worldPosition);
		m_aOutlinePoints.Insert(newPoint);
		m_mOutlinePoints.Insert(key, newPoint);
	}
	
	protected void GetEdgeMidPointWorldPositions(int x, int z, float maxSlope, BaseWorld world, array<ref DAB_TerrainObstaclePoint> tilePoints, out vector worldPositions[4])
	{
		vector localPointPosition = GetLocalPointPosition(x, z);
	
		float value0 = tilePoints[0].GetSlope();
		float value1 = tilePoints[1].GetSlope();
		float value2 = tilePoints[2].GetSlope();
		float value3 = tilePoints[3].GetSlope();
		
		bool out0 = tilePoints[0].IsOutside();
		bool out1 = tilePoints[1].IsOutside();
		bool out2 = tilePoints[2].IsOutside();
		bool out3 = tilePoints[3].IsOutside();
		
		float t0 = 0.5;
		float t1 = 0.5;
		float t2 = 0.5;
		float t3 = 0.5;
		
		if (!out0 && !out1) t0 = SafeInverseLerp(value0, value1, maxSlope);
		if (!out1 && !out2) t1 = SafeInverseLerp(value1, value2, maxSlope);
		if (!out3 && !out2) t2 = SafeInverseLerp(value3, value2, maxSlope); // 3 to 2 is x+ direction
		if (!out0 && !out3) t3 = SafeInverseLerp(value0, value3, maxSlope); // 0 to 3 is z+ direction
	
		worldPositions[0] = localPointPosition + Vector(t0 * m_fCellSize, 0, 0);
		worldPositions[1] = localPointPosition + Vector(m_fCellSize, 0, t1 * m_fCellSize);
		worldPositions[2] = localPointPosition + Vector(t2 * m_fCellSize, 0, m_fCellSize);
		worldPositions[3] = localPointPosition + Vector(0, 0, t3 * m_fCellSize);
		
		for(int i = 0; i < 4; i++)
		{
			worldPositions[i] = LocalToWorld(worldPositions[i]);
			float terrainHeight = world.GetSurfaceY(worldPositions[i][0], worldPositions[i][2]);
			worldPositions[i][1] = terrainHeight;
		}
	}

	protected float SafeInverseLerp(float a, float b, float target)
	{
		float diff = b - a;
	    if (Math.AbsFloat(diff) < 0.0001)
	        return 0.5; 
	        
	    float t = (target - a) / diff;
	    return Math.Clamp(t, 0.0, 1.0);
	
	}
		
	protected void GetEdgeMidPointKeys(int x, int z, out int keys[4])
	{
		int stride = 2 * m_iColumns - 1;
		
		// Double resolution grid; +1 = move half a tile; +2 = move a whole tile
		keys[0] = (2 * x + 1)	* stride	+ (2 * z );
		keys[1] = (2 * x + 2)	* stride	+ (2 * z + 1);
		keys[2] = (2 * x + 1)	* stride	+ (2 * z + 2);
		keys[3] = (2 * x)		* stride	+ (2 * z + 1);
	}
	
	//------------------------------------------------------------------------------------------------
	// (row+1,col) .. (row+1,col+1)
	//   tl[3] ------- tr[2]
	//    |               |
	//   bl[0] ------- br[1]
	// (row,col)   (row,col+1)
	//
	// ^ current cell origin = bl[0]
	protected bool TryGetTilePoints(int currentPoint, out int tilePointIndices[4], notnull out array<ref DAB_TerrainObstaclePoint> tilePoints)
	{
		tilePoints.Clear();
		
		tilePointIndices[0] = currentPoint;
		tilePointIndices[1] = currentPoint + m_iColumns;     // x + 1
		tilePointIndices[2] = currentPoint + m_iColumns + 1; // x + 1, z + 1
		tilePointIndices[3] = currentPoint + 1;              // z + 1
		
		for(int i= 0; i < 4; i++)
		{
			tilePoints.Insert(GetPoint(tilePointIndices[i])); 
			if(! tilePoints[i]) return false;
		}

		if(tilePoints.Count() != 4) return false;
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected int GetTileState(notnull array<ref DAB_TerrainObstaclePoint> tilePoints, float maxSlope, bool print)
	{
		if(tilePoints.Count() != 4) return -1;
		int stateKey = 0;
		
		for(int i = 0; i < 4; i++){
			if(!tilePoints[i].IsOutside() && tilePoints[i].GetSlope() > maxSlope)
				stateKey |= 1 << i;
		}
		
		
		return stateKey;
	}
	
	//------------------------------------------------------------------------------------------------
	protected DAB_TerrainObstaclePoint GetPoint(int index)
	{
		if(!IsValidIndex(index)) return null;
		return m_aPoints.Get(index);
	}
	
	//TODO: We could sample and average 4 points around the localPoint, but that would be slower and probably not worth it
	float GetSlopeAtLocalPoint(vector localPoint)
	{
		if(m_fCellSize < 0)
		{
			Print("Terrain obstacle grid was not set up. You need to enable it in the settings!", LogLevel.ERROR);
			return INVALID_SLOPE;
		}
		
		int x, z
		if(!TryLocalToGridPosition(localPoint, x, z))
			return INVALID_SLOPE;
		
		DAB_TerrainObstaclePoint point = GetPoint(GetIndexFromGridPos(x,z));
		if(!point) return INVALID_SLOPE;
		
		return point.GetSlope();
	}
	
	//------------------------------------------------------------------------------------------------
	protected int GetIndexFromGridPos(int x, int z)
	{
		return x * m_iColumns + z;
	}
	
	protected bool IsValidIndex(int index)
	{
		return index >= 0 && index < m_aPoints.Count();
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsValidGridPos(int x, int z)
	{
		return (x >= 0 && x < m_iRows && z >= 0 && z < m_iColumns);
	}

	//------------------------------------------------------------------------------------------------
	protected vector WorldToLocal(vector worldPos)
	{
		return worldPos.Multiply4(m_aInvWorldTransform);
	}
	
	//------------------------------------------------------------------------------------------------
	protected vector LocalToWorld(vector localPos)
	{
		return localPos.Multiply4(m_aWorldTransform);
	}

	//------------------------------------------------------------------------------------------------
	protected bool TryLocalToGridPosition(vector localPos, out int x, out int z)
	{
		x = Math.Floor((localPos[0] - m_vOriginLocal[0]) / m_fCellSize);
		z = Math.Floor((localPos[2] - m_vOriginLocal[2]) / m_fCellSize);
	
		return IsValidGridPos(x,z);
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool TryGetGridCoordsFromIndex(int index, out int x, out int z)
	{
		if (index < 0 || index >= m_aPoints.Count())
		{
			x = -1;
			z = -1;
			return false;
		}
	
		x = index / m_iColumns;   // integer division
		z = index % m_iColumns;
	
		return true;
	}
		
	//------------------------------------------------------------------------------------------------
	void ClearGrid()
	{
		m_vOriginLocal = vector.Zero;
		m_vOriginWorld = vector.Zero;
		m_iRows = -1;
		m_iColumns = -1;
		
		m_aPoints.Clear();
		m_aOutlinePoints.Clear(); 
		m_mOutlinePoints.Clear(); 
		m_aSlopeOutlines.Clear(); 
	}
	
	//------------------------------------------------------------------------------------------------
	void DebugVisualizeGrid(SCR_DebugShapeManager debugShapeManager, BaseWorld world, float maxSlope)
	{
		ShapeFlags flags = ShapeFlags.TRANSP | ShapeFlags.ADDITIVE | ShapeFlags.DOUBLESIDE | ShapeFlags.NOZBUFFER;
		ShapeFlags lineFlags = ShapeFlags.NOZWRITE | ShapeFlags.NOZBUFFER | ShapeFlags.NOCULL;
			
		foreach(DAB_Outline outline : m_aSlopeOutlines)
		{
			for(int i = 0; i < outline.m_aPoints.Count(); i++)
			{
				DAB_OutlinePoint point = outline.m_aPoints[i];
				
				vector pos0 = point.m_vWorldPosition;
				vector pos1 = outline.m_aPoints[(i + 1) % outline.m_aPoints.Count()].m_vWorldPosition;
				
				if(i == 0)
				{
					DebugTextWorldSpace debugText = DebugTextWorldSpace.Create(world, string.Format("%1 m^2", outline.m_fArea), lineFlags, pos0[0], pos0[1], pos0[2], 15);
					debugShapeManager.Add(debugText);
				}
				
				debugShapeManager.AddLine(pos0, pos1, Color.BLUE, lineFlags);
			}
		}
	}
		
	// We need to do this because you can not set or predefine a [16][4], so we have to do this instead
	protected void SetupTileEdges()
	{
		TILE_EDGE_LOOKUP.Clear();
		for (int i = 0; i < 16; i++)
        	TILE_EDGE_LOOKUP.Insert({});

	    TILE_EDGE_LOOKUP[0] = CombineTileEdgeValues(-1, -1, -1, -1);
    	TILE_EDGE_LOOKUP[1] = CombineTileEdgeValues( 3,  0, -1, -1);
    	TILE_EDGE_LOOKUP[2] = CombineTileEdgeValues( 0,  1, -1, -1);
	    TILE_EDGE_LOOKUP[3] = CombineTileEdgeValues( 3,  1, -1, -1);
	    TILE_EDGE_LOOKUP[4] = CombineTileEdgeValues( 2,  1, -1, -1);
	    TILE_EDGE_LOOKUP[5] = CombineTileEdgeValues( 3,  2,  0,  1);
	    TILE_EDGE_LOOKUP[6] = CombineTileEdgeValues( 0,  2, -1, -1);
	    TILE_EDGE_LOOKUP[7] = CombineTileEdgeValues( 3,  2, -1, -1);
	    TILE_EDGE_LOOKUP[8] = CombineTileEdgeValues( 3,  2, -1, -1);
	    TILE_EDGE_LOOKUP[9] = CombineTileEdgeValues( 0,  2, -1, -1);
	    TILE_EDGE_LOOKUP[10] = CombineTileEdgeValues( 3,  0,  2,  1);
	    TILE_EDGE_LOOKUP[11] = CombineTileEdgeValues( 2,  1, -1, -1);
	    TILE_EDGE_LOOKUP[12] = CombineTileEdgeValues( 3,  1, -1, -1);
	    TILE_EDGE_LOOKUP[13] = CombineTileEdgeValues( 0,  1, -1, -1);
	    TILE_EDGE_LOOKUP[14] = CombineTileEdgeValues( 3,  0, -1, -1);
	    TILE_EDGE_LOOKUP[15] = CombineTileEdgeValues(-1, -1, -1, -1);
		
		if(TILE_EDGE_LOOKUP.Count() != 16)
			PrintFormat("Tile edge count is wrong! Should be 16 but is %1 instead!", TILE_EDGE_LOOKUP.Count(), LogLevel.ERROR);
	}
	
	protected array<int> CombineTileEdgeValues(int a, int b, int c, int d)
	{
		array<int> value = {};
		value.Insert(a);
		value.Insert(b);
		value.Insert(c);
		value.Insert(d);
		return value;
	}
	
	// Static copy from ForestGeneratorEntity then inverted
	protected static void CalculateInvertedOutlineAnglesForPoints(notnull array<ref SCR_ForestGeneratorPoint> points)
	{
		int count = points.Count();
		if (count < 3)
			return;

		SCR_ForestGeneratorPoint previousPoint = points[count - 1];
		SCR_ForestGeneratorPoint currentPoint;
		SCR_ForestGeneratorPoint nextPoint;
		vector dir1;
		vector dir2;
		for (int i; i < count; i++)
		{
			currentPoint = points[i];

			if (i < count - 1)
				nextPoint = points[i + 1];
			else
				nextPoint = points[0];

			dir1 = previousPoint.m_vPos - currentPoint.m_vPos;
			dir2 = nextPoint.m_vPos - currentPoint.m_vPos;
			float yaw1 = dir1.ToYaw();
			float yaw2 = dir2.ToYaw();
			if (yaw1 > yaw2)
			{
				currentPoint.m_fMinAngle = yaw2; // Was yaw1 - 360
				currentPoint.m_fMaxAngle = yaw1; // Was yaw2
			}
			else
			{
				currentPoint.m_fMinAngle = yaw2 - 360; //yaw1
				currentPoint.m_fMaxAngle = yaw1; //yaw2
			}

			currentPoint.m_fAngle = Math.AbsFloat(currentPoint.m_fMaxAngle - currentPoint.m_fMinAngle);

			previousPoint = currentPoint;
		}
	}
	
	#endif
}