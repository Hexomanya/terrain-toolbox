modded class SCR_AreaGeneratorBaseEntity 
{
	//From LouMontana: https://feedback.bistudio.com/T199212, should be fixed in 1.7
	protected override void RefreshObstacles()
	{
		if (!m_ParentShapeSource)
			return;

		array<vector> vectorPoints = GetAnchorPoints(m_ParentShapeSource);
		SCR_AABB bbox = new SCR_AABB(vectorPoints);
		bbox.m_vMin[1] = bbox.m_vMin[1] - BBOX_CHECK_HEIGHT * 0.5;
		bbox.m_vMax[1] = bbox.m_vMax[1] + BBOX_CHECK_HEIGHT * 0.5;

		SetAvoidOptions();
		s_ObstacleDetector.RefreshObstaclesByAABB(CoordToParent(bbox.m_vMin), CoordToParent(bbox.m_vMax));
	}
}