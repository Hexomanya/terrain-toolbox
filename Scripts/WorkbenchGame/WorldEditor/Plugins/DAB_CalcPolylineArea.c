#ifdef WORKBENCH

[WorkbenchPluginAttribute( name: "Calculate Polyline Area", description: "Calculate the area of selected polylines", category: "DAB - Misc", wbModules: {"WorldEditor"}, awesomeFontCode: 0xF1FE)]
class DAB_CalcPolylineArea: WorkbenchPlugin
{	
	const float areaDivider = 1000000;
	const string areaUnit = "km^2";
	
    override void Run()
    {
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;

		WorldEditorAPI m_API = worldEditor.GetApi();
		if (!m_API)
			return;
		
		m_API.BeginEntityAction();

		array<IEntitySource> selectedPolylines = {};
		DAB_EntityHelper.GetAllSelectedEntitiesOfType(PolylineShapeEntity, selectedPolylines);
		
		PrintFormat("%1 Polylines selected", selectedPolylines.Count());
	
		for (int i; i < selectedPolylines.Count(); i++)
		{
			IEntitySource source = selectedPolylines[i];
			int area = this.CalculateArea(source, m_API);
			PrintFormat("Area for entity %1 is %2 %3", source.GetName(), (area / areaDivider), areaUnit);
		}
		
		m_API.EndEntityAction();
	}
	
	protected int CalculateArea(IEntitySource polylineSource, WorldEditorAPI m_API)
	{
		PolylineShapeEntity polyline = PolylineShapeEntity.Cast(m_API.SourceToEntity(polylineSource));
		if(!polyline) {
			Print("Something went wrong when converting a EntitySource to PolylineShapeEntity. Skipping...", LogLevel.WARNING);
			return -1;
		}
		
		array<vector> positions = {};
		polyline.GetPointsPositions(positions);
		int posCount = positions.Count();
		if(posCount < 3) {
			Print("To calculate the area, the polyline needs at least three positions. Skipping...", LogLevel.WARNING);
			return -1;
		}
		
		float area = 0;
		for (int i = 0; i < positions.Count(); i++)
		{
			int j = (i + 1) % posCount;
			
			vector currentPoint = positions[i];
			vector nextPoint = positions[j];
			
			area += currentPoint[0] * nextPoint[2];
			area -= nextPoint[0] * currentPoint[2];
		}
		
		return Math.AbsFloat(area) / 2.0;
	}
}

#endif // WORKBENCH