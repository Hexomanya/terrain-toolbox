class DAB_ForestOutline
{
	ref array<ref SCR_ForestGeneratorPoint> m_aAllOutlinePoints = {};
	ref array<ref SCR_ForestGeneratorLine> m_aAllOutlineLines = {};
	
	ref array<ref SCR_ForestGeneratorPoint> m_aSmallOutlinePoints = {};
	ref array<ref SCR_ForestGeneratorPoint> m_aMiddleOutlinePoints = {}; 
	ref array<ref SCR_ForestGeneratorLine> m_aSmallOutlineLines = {};
	ref array<ref SCR_ForestGeneratorLine> m_aMiddleOutlineLines = {};
	
	void AddPoint(SCR_ForestGeneratorPoint point, bool isSmallOutline, bool isMiddleOutline)
	{
		m_aAllOutlinePoints.Insert(point);
		if(isSmallOutline) m_aSmallOutlinePoints.Insert(point);
		if(isMiddleOutline) m_aMiddleOutlinePoints.Insert(point);
	}
	
	void AddLine(SCR_ForestGeneratorLine line, bool isSmallOutline, bool isMiddleOutline)
	{
		m_aAllOutlineLines.Insert(line);
		if(isSmallOutline) m_aSmallOutlineLines.Insert(line);
		if(isMiddleOutline) m_aMiddleOutlineLines.Insert(line);
	}
}