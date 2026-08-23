class DAB_TerrainObstaclePoint
{
	#ifdef WORKBENCH
	
	protected bool m_bIsOutside;
	protected float m_fSlope;
	protected bool m_bIsOnBannedMaterial;
	
	//------------------------------------------------------------------------------------------------
	void DAB_TerrainObstaclePoint(bool isOutside, float slope, float isOnBannedMaterial)
	{
		m_bIsOutside = isOutside;
		m_fSlope = slope;
		m_bIsOnBannedMaterial = isOnBannedMaterial;
	}	
	
	bool IsOutside(){return m_bIsOutside; }
	float GetSlope(){return m_fSlope;}
	bool GetIsOnBannedMaterial(){return m_bIsOnBannedMaterial;}
	
	#endif
}