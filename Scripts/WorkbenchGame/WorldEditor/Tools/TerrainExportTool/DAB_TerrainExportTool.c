// WARNING! The vanilla class does NOT use the correct nomenclatur! e.g. memeber variables are not prefixed with 'm_'
modded class TerrainExportTool
{
	[ButtonAttribute("Export To Blender")]
	protected override void Blender()
	{	
		if (!EBTConfigPlugin.HasBlenderRegistered())
			return;
		WorldEditor we = Workbench.GetModule(WorldEditor);
		auto api = we.GetApi();
		array<float> heightMap = {};
	
		
		string path;
		// creating temp bin file to pass the coords
		Workbench.GetAbsolutePath("$profile:", path);
		path = path + "/BlendTerrain.bin";

		float tileResX = (m_API.GetTerrainResolutionX(0) * m_API.GetTerrainUnitScale(0)) / m_API.GetTerrainTilesX(0) / m_API.GetTerrainUnitScale();
		float tileResY = (m_API.GetTerrainResolutionY(0) * m_API.GetTerrainUnitScale(0)) / m_API.GetTerrainTilesY(0) / m_API.GetTerrainUnitScale();
		
		// FIX 1: Add +1 to account for vertices instead of cells
		int verticesX = Math.Floor(tileResX) + 1;
		int verticesY = Math.Floor(tileResY) + 1;
		int area = verticesX * verticesY; 
		
		int tileCount = 0;
		FileHandle bin = FileIO.OpenFile(path, FileMode.WRITE);
		api.BeginTerrainAction(TerrainToolType.HEIGHT_EXACT);
		
		// Area of one tile
		bin.Write(area);

		for(int i = 0; i < selectedCoords.Count(); i++)
		{
			// FIX 2: Clear the array before fetching new tile data
			heightMap.Clear(); 
			
			tileCount += 1;
			int coordsX = Math.Round(selectedCoords[i][0] * (Math.Floor(tileResX) * m_API.GetTerrainUnitScale()));
			int coordsY = Math.Round(selectedCoords[i][2] * (Math.Floor(tileResY) * m_API.GetTerrainUnitScale()));
			
			int indexX = selectedCoords[i][0];
			int indexY = selectedCoords[i][2];
			bin.Write(indexX);
			bin.Write(indexY);
			
			bin.Write(coordsX);
			bin.Write(coordsY);

			if (api.GetTerrainSurfaceTile(0, selectedCoords[i][0], selectedCoords[i][2], heightMap))
			{
				bin.WriteArray(heightMap);
			}
		}
		api.EndTerrainAction();		
		bin.Close();
				
		string pathToExecutable;
		if (!EBTConfigPlugin.GetDefaultBlenderPath(pathToExecutable))
			return;

		string worldpath;
		m_API.GetWorldPath(worldpath);
		
		BlenderOperatorDescription operatorDescription = new BlenderOperatorDescription("terrain");
		operatorDescription.blIDName = "ebt.import_terrain";
		operatorDescription.AddParam("bin_path", path);
		operatorDescription.AddParam("cell_size", m_API.GetTerrainUnitScale(0));
		operatorDescription.AddParam("world_path",worldpath);
		operatorDescription.AddParam("tile_count", tileCount);
		
		StartBlenderWithOperator(operatorDescription, false);
	}
}