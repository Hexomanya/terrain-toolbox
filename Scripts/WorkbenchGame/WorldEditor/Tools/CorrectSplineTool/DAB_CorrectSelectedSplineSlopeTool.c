#ifdef WORKBENCH

enum FixedSplineEndPoint {
	FIX_HEIGHEST_POINT, //Top to Bottm
	FIX_LOWEST_POINT, //Bottom to Top
}

[WorkbenchToolAttribute( name: "Correct Spline Slope", description: "Clamps the local slope on each spline point.", wbModules: {"WorldEditor"}, awesomeFontCode:0xf715)]
class DAB_CorrectSelectedSplineSlopeTool: WorldEditorTool
{
	//------------------ Slope Correction -----------------------------------------------------------
	[Attribute(defvalue: "1", desc: "Decides if the heighest or lowest point will stay fixed. The other one might be moved!", category: "Slope Correction", uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(FixedSplineEndPoint))]
	protected FixedSplineEndPoint m_fixedPoint;
	
	[Attribute(defvalue: "false", desc: "", category: "Slope Correction", uiwidget: UIWidgets.CheckBox)]
	protected bool m_useHymanInstead;
	
	[Attribute(defvalue: "1", desc: "Minimal local slope", category: "Slope Correction")]
	protected float m_minSlope;
	
	[Attribute(defvalue: "70", desc: "Maximum local slope", category: "Slope Correction")]
	protected float m_maxSlope;
	
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Analyze Splines")]
	protected void AnalyzeSplines()
	{		
		array<IEntitySource> selectedSplines = GetSelectedSplines();
				
		for (int i; i < selectedSplines.Count(); i++)
		{
			IEntitySource splineSrc = selectedSplines[i];
			AnalyzeSpline(splineSrc);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void AnalyzeSpline(IEntitySource splineSrc)
	{	
		SplineShapeEntity spline = SplineShapeEntity.Cast(m_API.SourceToEntity(splineSrc));
		if(!spline) {
			Print("Something went wrong when converting a Entity to Spline. Skipping...", LogLevel.WARNING);
			return;
		}
		
		array<vector> positions = {};
		spline.GetPointsPositions(positions);
		if(positions.Count() < 3) {
			Print("To analyze a spline it needs at least three points. Skipping...", LogLevel.WARNING);
			return;
		}
		
		
		float lowestSlope = float.MAX;
		float highestSlope = -float.MAX;
		float averageCount = 0;
			
		
		for (int j = 0; j < positions.Count() - 1; j++) {
			vector currentPoint = positions[j];
			vector nextPoint = positions[j+1];
		
			float distanceXZ = vector.DistanceXZ(nextPoint, currentPoint);
			float actualHeight = nextPoint[1] - currentPoint[1];
			float slope = Math.AbsFloat(Math.Atan2(Math.AbsFloat(actualHeight), distanceXZ) * Math.RAD2DEG);
		
			lowestSlope = Math.Min(lowestSlope, Math.AbsFloat(slope));
			highestSlope = Math.Max(highestSlope, Math.AbsFloat(slope));
			averageCount += slope;
		}
		
		Print("Values for spline: " + spline.GetName() + " ------------");
		Print("Lowest Slope: " + lowestSlope);
		Print("Heighest Slope: " + highestSlope);
		Print("Average Slope: " + (averageCount / (positions.Count() - 1)));
	}
	
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Correct Slopes")]
	protected void CorrectSlopes()
	{		
		array<IEntitySource> selectedSplines = GetSelectedSplines();
				
		for (int i; i < selectedSplines.Count(); i++)
		{
			IEntitySource splineSrc = selectedSplines[i];
			CorrectSpline(splineSrc);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected array<IEntitySource> GetSelectedSplines() {
		array<IEntitySource> selectedSplines = {};
		
		for (int i; i < m_API.GetSelectedEntitiesCount(); i++)
		{
			IEntitySource entSrc = m_API.GetSelectedEntity(i);
			
			if(!entSrc || !m_API.SourceToEntity(entSrc).IsInherited(SplineShapeEntity)) continue;
			
			if(selectedSplines.Contains(entSrc)) continue;
			selectedSplines.Insert(entSrc);
		}
		
		return selectedSplines;
	}
	

	//------------------------------------------------------------------------------------------------
	protected void CorrectSpline(IEntitySource splineSrc)
	{
		SplineShapeEntity spline = SplineShapeEntity.Cast(m_API.SourceToEntity(splineSrc));
		if(!spline) {
			Print("Something went wrong when converting a Entity to Spline. Skipping...", LogLevel.WARNING);
			return;
		}
		
		bool didChangeHeights = false;
		array<vector> positions = {};
		spline.GetPointsPositions(positions);
		if(positions.Count() < 3) {
			Print("To correct a spline it needs at least three points. Skipping...", LogLevel.WARNING);
			return;
		}
		
		float startHeight = positions[0][1];
		float endHeight = positions[positions.Count() - 1][1];
		
		Print("startHeight: " + startHeight + "; endHeight: " + endHeight);
		
		int adjustedPointCount = 0;
		bool goingUphill = m_fixedPoint == FixedSplineEndPoint.FIX_LOWEST_POINT;
		bool countFromBackToFront = (goingUphill && (endHeight < startHeight)) 
									|| (!goingUphill && (endHeight > startHeight));
		
		if(countFromBackToFront){
			
			//Print("Back -> Front");
			//Ignore first point
			for (int j = positions.Count() - 2; j >= 0; j--){
				vector currentPoint = positions[j];
				vector previousPoint = positions[j+1];

				float distanceXZ = vector.DistanceXZ(previousPoint, currentPoint);
				
				float actualHeight = currentPoint[1] - previousPoint[1];
				if(!goingUphill) actualHeight = previousPoint[1] - currentPoint[1];	
				
				float minHeight = Math.Tan(Math.DEG2RAD * m_minSlope) * distanceXZ;
				float maxHeight = Math.Tan(Math.DEG2RAD * m_maxSlope) * distanceXZ;
				
				if(actualHeight > minHeight && actualHeight < maxHeight) continue;
				
				didChangeHeights = true;
				adjustedPointCount++;
				
				actualHeight = Math.Clamp(actualHeight, minHeight, maxHeight);
				if(goingUphill){
					currentPoint[1] = previousPoint[1] + actualHeight;
				} else {
					currentPoint[1] = previousPoint[1] - actualHeight;
				}
				

				positions[j] = currentPoint;
			}
		} else {
			//Print("Front -> Back");
			//Ignore first point
			for (int j = 1; j < positions.Count(); j++){
				vector previousPoint = positions[j-1];
				vector currentPoint = positions[j];
				
				float distanceXZ = vector.DistanceXZ(previousPoint, currentPoint);
				
				float actualHeight = previousPoint[1] - currentPoint[1];
				if(goingUphill) actualHeight = currentPoint[1] - previousPoint[1];
				
				float minHeight = Math.Tan(Math.DEG2RAD * m_minSlope) * distanceXZ;
				float maxHeight = Math.Tan(Math.DEG2RAD * m_maxSlope) * distanceXZ;
				
				if(actualHeight > minHeight && actualHeight < maxHeight) continue;
				
				didChangeHeights = true;
				adjustedPointCount++;
				
				actualHeight = Math.Clamp(actualHeight, minHeight, maxHeight);
				currentPoint[1] = previousPoint[1] - actualHeight;
				
				if(!goingUphill){
					currentPoint[1] = previousPoint[1] - actualHeight;
				} else {
					currentPoint[1] = previousPoint[1] + actualHeight;
				}
			
				positions[j] = currentPoint;
			}
		}
		
		array<vector> tangents = {};
		for (int i = 0; i < positions.Count(); i++){
			vector inTan, outTan;
			spline.GetTangents(i, inTan, outTan);
			tangents.Insert(inTan);
			tangents.Insert(outTan);
		}
		

		m_API.BeginEditSequence(splineSrc);
		m_API.BeginEntityAction("CorrectingSplineElevation", "");
		
		DAB_ShapeHelper.ModifyPolyline(splineSrc, positions);
		if(!m_useHymanInstead)
		{
			ClampTangentsForMonotonicity(positions, tangents);
		} else
		{
			ApplyHymanMehtod(positions, tangents)
		}

		CopyTangents(splineSrc, tangents);
		
		m_API.EndEntityAction("CorrectingSplineElevation");
		m_API.EndEditSequence(splineSrc);
		
		DAB_ShapeHelper.FixTerrainAdjustmentGenerators(splineSrc); //This produces errors, but still works (Nothing we can do)
		
		
		Print("Adjusted " + adjustedPointCount + "/" + positions.Count() + " points of the Spline.");
	}
	
	protected void ClampTangentsForMonotonicity(array<vector> positions, out array<vector> tangents)
	{
		for (int j = 0; j < positions.Count() - 1; j++)
	    {
	       	float heightDiff = positions[j+1][1] - positions[j][1];
        	float maxTanYSum = 3.0 * Math.AbsFloat(heightDiff);
			
			vector curOutTan = tangents[j * 2 + 1];
        	vector nextInTan  = tangents[(j + 1) * 2]; 
			
			if (heightDiff <= 0) {
	            curOutTan[1] = Math.Min(curOutTan[1], 0.0);
	            nextInTan[1]  = Math.Min(nextInTan[1],  0.0);
	        } else {
	            curOutTan[1] = Math.Max(curOutTan[1], 0.0);
	            nextInTan[1]  = Math.Max(nextInTan[1],  0.0);
	        }
			
			//|curOutTan.Y| + |nextInTan.Y| <= 3 * |heightDiff|
			float tanYSum = Math.AbsFloat(curOutTan[1]) + Math.AbsFloat(nextInTan[1]);
	        if (tanYSum > maxTanYSum && tanYSum > 0.0001)
	        {
	            float scale = maxTanYSum / tanYSum;
	            curOutTan = curOutTan * scale;
	            nextInTan = nextInTan  * scale;
	        }
			
			// If we do the math directly in the brackets the compiler brakes
			int outIndex = j * 2 + 1;
			int inIndex = (j + 1) * 2;
			tangents[outIndex] = curOutTan;
        	tangents[inIndex] = nextInTan;
	    }
	}
	
	// Spline lookup => https://www.youtube.com/watch?v=jvPPXbo87ds
	protected void ApplyHymanMehtod(array<vector> positions, out array<vector> tangents)
	{
		array<float> slopes = {}; //slope on segment i -> i+1
		
		for(int i = 0; i < positions.Count() - 1; i++)
		{
			float distXZ = vector.DistanceXZ(positions[i+1], positions[i]);
    		float heightDiff = positions[i+1][1] - positions[i][1];
			
			float slope = 0.0;
		    if (distXZ > 0.0001)
		        slope = heightDiff / distXZ;
			
			slopes.Insert(slope);
		}
		
		
		//Catmull rom like
		array<float> tangentSlopes = {};
		tangentSlopes.Insert(slopes[0]); //slope at point i, first point is onesided
		
		for(int i = 1; i < positions.Count() - 1; i++)
		{
			float averageSlope = (slopes[i-1] + slopes[i]) * 0.5;
    		tangentSlopes.Insert(averageSlope);
		}
		
		tangentSlopes.Insert(slopes[slopes.Count() - 1]); //last point is onesided
		
		//Hymans Filter
		for(int i = 1; i < positions.Count() - 1; i++)
		{
			float prevSlope = slopes[i-1];
			float nextSlope = slopes[i];
			
			//opposite signs => flat
			if (prevSlope * nextSlope <= 0.0)
		    {
		        tangentSlopes[i] = 0.0;
		        continue;
		    }
			
			float maxAllowed = 3.0 * Math.Min(Math.AbsFloat(prevSlope), Math.AbsFloat(nextSlope));
			tangentSlopes[i] = Math.Clamp(tangentSlopes[i], -maxAllowed, maxAllowed);
		}
		
		
		for(int i = 0; i < positions.Count(); i++)
		{
			vector inTan = tangents[i * 2];
			vector outTan = tangents[i * 2 +1];
			
			if (i > 0)
		    {
		        float distPrev = vector.DistanceXZ(positions[i], positions[i-1]);
		        inTan[1] = tangentSlopes[i] * distPrev;
		    }
			if (i < positions.Count() - 1)
		    {
		        float distNext = vector.DistanceXZ(positions[i+1], positions[i]);
		        outTan[1] = tangentSlopes[i] * distNext;
		    }
			
			int inIndex = i * 2;
			int outIndex = i * 2 + 1;
			
			tangents[inIndex] = inTan;
        	tangents[outIndex] = outTan;
		}
	}
	
	protected void CopyTangents(IEntitySource splineSrc, array<vector> tangents)
	{
		BaseContainerList points = splineSrc.GetObjectArray("Points");
		
		for (int j = 0; j < points.Count(); j++){
			BaseContainer point = points[j];
			BaseContainerList pointData = point.GetObjectArray("Data");
			
			if (pointData.Count() == 0)
			{
				m_API.CreateObjectArrayVariableMember(point, null, "Data", "SplinePointData", 0);
			}

			array<ref ContainerIdPathEntry> containerPath = {ContainerIdPathEntry("Points", j), ContainerIdPathEntry("Data", 0)};
			
			vector inTangent = tangents[j * 2];
			vector outTangent = tangents[(j * 2) + 1];
			
			m_API.SetVariableValue(splineSrc, containerPath, "InTangent", inTangent.ToString(false));	
			m_API.SetVariableValue(splineSrc, containerPath, "OutTangent", outTangent.ToString(false));		
		}
	}

}
#endif // WORKBENCH
