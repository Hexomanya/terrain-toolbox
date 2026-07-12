#ifdef WORKBENCH

// WARNING! The vanilla class does NOT use the correct nomenclatur! e.g. memeber variables are not prefixed with 'm_'
// THIS CLASS WAS/IS A MESS! 
[WorkbenchToolAttribute(name: "[Fixed]: Send Terrain To Blender", description: "Sends terrain selection to Blender for advanced terrain modifications.", wbModules: { "WorldEditor" }, shortcut: "Ctrl+P", awesomeFontCode: 0xf517)]
modded class TerrainExportTool
{
	[ButtonAttribute("Export To Blender")]
	protected override void Blender()
	{	
		if (!EBTConfigPlugin.HasBlenderRegistered())
			return;
		
		// Command parameters
		string path;
		float cellSize = m_API.GetTerrainUnitScale();;
		string worldpath;
		int tileCount = 0;
		
		// Fill command parameters
		Workbench.GetAbsolutePath("$profile:", path);
		path = path + "/BlendTerrain.bin";
		
		m_API.GetWorldPath(worldpath);

		vector terrainDimensions =  SCR_WorldEditorToolHelper.GetTerrainDimensions();
		float tileSizeX = terrainDimensions[0] / m_API.GetTerrainTilesX();
		float tileSizeZ = terrainDimensions[2] / m_API.GetTerrainTilesY(); // We change name to Z because this is the actual coord direction
		
		if(!float.AlmostEqual(tileSizeX, tileSizeZ))
		{
			PrintFormat("Found non square tile with x: %1, z: %2. This should not be possible!", tileSizeX, tileSizeZ, LogLevel.ERROR);
			return;
		}
		
		int verticesX = Math.Floor(tileSizeX / cellSize) + 1;
		int vertexCount = verticesX * verticesX;  // We assume that the tile is square
	
		FileHandle bin = FileIO.OpenFile(path, FileMode.WRITE);
		bin.Write(vertexCount); // Header: VertexCount of one tile

		m_API.BeginTerrainAction(TerrainToolType.HEIGHT_EXACT); // To ensure that the terrain data is loaded?
		array<float> heightMap = {};
		
		for(int i = 0; i < selectedCoords.Count(); i++)
		{
			heightMap.Clear(); 
			
			PrintFormat("Selected coords at %1 are %2", i, selectedCoords[i]);
			int tileIndexX = selectedCoords[i][0]; //Blender expects int not float
			int tileIndexZ = selectedCoords[i][2];
			
			bin.Write(tileIndexX); 
			bin.Write(tileIndexZ);
			
			int lowLeftCornerX = tileIndexX * tileSizeX;
			int lowLeftCornerZ = tileIndexZ * tileSizeZ;
			bin.Write(lowLeftCornerX);
			bin.Write(lowLeftCornerZ);

			if (m_API.GetTerrainSurfaceTile(0, selectedCoords[i][0], selectedCoords[i][2], heightMap))
				bin.WriteArray(heightMap);
			
			tileCount += 1;
		}
		m_API.EndTerrainAction();		
		bin.Close();
				
		string pathToExecutable;
		if (!EBTConfigPlugin.GetDefaultBlenderPath(pathToExecutable))
			return;

		BlenderOperatorDescription operatorDescription = new BlenderOperatorDescription("terrain");
		operatorDescription.blIDName = "ebt.import_terrain";
		operatorDescription.AddParam("bin_path", path);
		operatorDescription.AddParam("cell_size", cellSize);
		operatorDescription.AddParam("world_path", worldpath);
		operatorDescription.AddParam("tile_count", tileCount);
		
		StartBlenderWithOperator(operatorDescription, false);
	}
}
#endif //Workbench
