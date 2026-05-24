class DAB_Outline
{
	ref array<ref DAB_OutlinePoint> m_aPoints = {};
	float m_fArea = -1;
	
	void ReverseOrientation()
	{
		int count = m_aPoints.Count();
		float iterNum = count * 0.5;
		DAB_OutlinePoint pointSave;
		
		for (int i = 0; i < iterNum; i++)
		{
			pointSave = m_aPoints[i];
			m_aPoints[i] = m_aPoints[count - 1 - i];
			m_aPoints[count - 1 - i] = pointSave;
		}
	}
	
	// Attention! This is still in world format, so if you use this for polygon checks, the polygon also needs to be worldspace
	array<float> ToXZFloatFormat()
	{
		array<float> formattedArray = {};
		
		foreach(DAB_OutlinePoint point : m_aPoints)
		{
			formattedArray.Insert(point.m_vWorldPosition[0]);
			formattedArray.Insert(point.m_vWorldPosition[2]);
		}
		
		return formattedArray;
	}
}

modded class SCR_ForestGeneratorLine
{
	bool m_bIsSlopeLine = false;
}