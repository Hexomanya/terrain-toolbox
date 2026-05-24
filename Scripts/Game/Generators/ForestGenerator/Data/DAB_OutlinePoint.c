class DAB_OutlinePoint
{
	// TODO: Make this protected with getter
	int m_iKey;
	vector m_vWorldPosition;
	
	protected int m_iEdgePointKey1 = - 1;
	protected int m_iEdgePointKey2 = - 1;
	
	void DAB_OutlinePoint(int pointKey, vector pointWorldPosition)
	{
		m_iKey = pointKey;
		m_vWorldPosition = pointWorldPosition;
	}
	
	bool InsertEdgePoint(int edgePointKey)
	{
		if(m_iEdgePointKey1 == -1) 
		{
			m_iEdgePointKey1 = edgePointKey;
			return true;
		}
		if(m_iEdgePointKey2 == -1) 
		{
			m_iEdgePointKey2 = edgePointKey;
			return true;
		}
		
		return false;
	}
	
	int GetEdgeKey(int edge)
	{
		if(edge != 1 && edge != 2)
		{
			PrintFormat("Edge needs to be either 1 or 2, not %1", edge, LogLevel.ERROR);
			return -1;
		}
		
		if(edge == 1) return m_iEdgePointKey1;
		else if (edge == 2) return m_iEdgePointKey2;
		
		Print("Something wen very wrong!",LogLevel.ERROR);
		return -1;
	}
	
	void SetEdgeKey(int edge, int key)
	{
		if(edge != 1 && edge != 2)
		{
			PrintFormat("Edge needs to be either 1 or 2, not %1", edge, LogLevel.ERROR);
			return;
		}
		
		if(key == -1)
		{
			Print("Trying to set the key to -1, This is not allowed!", LogLevel.ERROR);
			return;
		}
		
		if(edge == 1) m_iEdgePointKey1 = key;
		else if (edge == 2) m_iEdgePointKey2 = key;
	}
}