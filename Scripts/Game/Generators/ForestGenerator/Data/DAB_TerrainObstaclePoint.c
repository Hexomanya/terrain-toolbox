class DAB_TerrainObstaclePoint
{
	#ifdef WORKBENCH
	
	protected bool m_bIsOutside;
	protected float m_fSlope;
	
	//------------------------------------------------------------------------------------------------
	void DAB_TerrainObstaclePoint(bool isOutside, float slope)
	{
		m_bIsOutside = isOutside;
		m_fSlope = slope;
	}	
	
	bool IsOutside(){return m_bIsOutside; }
	float GetSlope(){return m_fSlope;}
	
	#endif
}