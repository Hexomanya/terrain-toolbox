class DAB_TerrainObstaclePoint
{
	#ifdef WORKBENCH
	
	protected bool m_bIsOutside;
	protected float m_fSlope = -1;
	protected bool m_bIsOnBannedMaterial;
	
	//------------------------------------------------------------------------------------------------
	void DAB_TerrainObstaclePoint(bool isOutside)
	{
		m_bIsOutside = isOutside;
	}	
	
	void SetData(float slope, bool isOnBannedMaterial)
	{
		m_fSlope = slope;
		m_bIsOnBannedMaterial = isOnBannedMaterial;
	}

	bool IsOutside(){return m_bIsOutside; }
	float GetSlope(){return m_fSlope;}
	bool GetIsOnBannedMaterial(){return m_bIsOnBannedMaterial;}
	
	#endif
}