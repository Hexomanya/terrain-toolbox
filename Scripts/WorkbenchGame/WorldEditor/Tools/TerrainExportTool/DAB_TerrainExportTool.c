#ifdef WORKBENCH

// WARNING! The vanilla class does NOT use the correct nomenclatur! e.g. memeber variables are not prefixed with 'm_'
// THIS CLASS WAS/IS A MESS! 
// Was modded version of TerrainToBlender.c which was removed https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff-Experimental/commit/cce8dcba8274bf8d2bb4f3e884d456d45a6dc8d7#diff-c01762be9f03b40e0a0d35f19357645a1f2d88438df2a421ed7c0bed8b6c58abL1-L230
class TerrainTile
{
	Shape shapeTile;
	vector coordsTile;
	vector minTile;
	vector maxTile;
	
	
	void TerrainTile(Shape square, vector coords, vector min, vector max)
	{
		shapeTile = square;
		coordsTile = coords;
		minTile = min;
		maxTile = max;
	}
}



[WorkbenchToolAttribute(name: "[Fixed]: Send Terrain To Blender", description: "Sends terrain selection to Blender for advanced terrain modifications.", wbModules: { "WorldEditor" }, shortcut: "Ctrl+P", awesomeFontCode: 0xf517)]
class TerrainExportTool : WorldEditorTool
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Brush radius", params: string.Format("%1 %2 %3", SIZE_MIN, SIZE_MAX, SIZE_STEP), category: "Brush")]
	protected float m_iSelectionSize;
	
	protected static const float SIZE_MIN = 1;
	protected static const float SIZE_MAX = 20;
	protected static const float SIZE_STEP = 1;
	
	
	vector previousPoint3D;
	ref Shape square;
	 
	ref array<ref TerrainTile> terrainTileVisual = {};
	ref array<ref TerrainTile> terrainTileSelection = {};
	
	
	ref array<ref Shape> visualTiles = {};
	ref array<vector> visualCoords = {};
	
	ref array<vector> selectedCoords = {};
	ref array<ref Shape> selectedTiles = {};
	
	
	ref array<ref Shape> shapeTiles = {};
	//ref array<vector> selectedTiles = {};
	
	//Remove
	vector currentPoint3D;
	vector line[8];
	//------
	
	
	vector startPosition;
	bool clicked = false;
	
	[ButtonAttribute("Export To Blender")]
	protected void Blender()
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
	
	
	
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Clear Selection")]
	protected void ClearSelection()
	{
		visualTiles.Clear();
		selectedTiles.Clear();
		selectedCoords.Clear();
		shapeTiles.Clear();
	}
	
	

	//------------------------------------------------------------------------------------------------
	override void OnMouseMoveEvent(float x, float y)
	{
		terrainTileVisual.Clear();
		visualTiles.Clear();
		visualCoords.Clear();
		vector traceStart;
		vector traceEnd;
		vector traceDir;

		if (m_API.TraceWorldPos(x, y, TraceFlags.WORLD, traceStart, traceEnd, traceDir))
		{
			float tileResX = (m_API.GetTerrainResolutionX(0) * m_API.GetTerrainUnitScale(0)) / m_API.GetTerrainTilesX(0);
			float tileResY = (m_API.GetTerrainResolutionY(0) * m_API.GetTerrainUnitScale(0)) / m_API.GetTerrainTilesY(0);
			vector tile = Vector(Math.Floor(traceEnd[0] / Math.Floor(tileResX)), 0, Math.Floor(traceEnd[2] / Math.Floor(tileResY)));			
			vector tileCornerMin = Vector(tile[0] * Math.Floor(tileResX),0, tile[2] * Math.Floor(tileResY));
			vector tileCornerMax = Vector(tileCornerMin[0] + Math.Floor(tileResX),0,tileCornerMin[2] + Math.Floor(tileResY));
			
			vector coords = traceEnd;
			for(int w = 1; w <= m_iSelectionSize; w++)
			{
				// now just deleting when click and clicking
				for(int h = 1; h <= m_iSelectionSize; h++)
				{
					tile = Vector(Math.Floor(coords[0] / Math.Floor(tileResX)),0,Math.Floor(coords[2] / Math.Floor(tileResY)));
					
					tileCornerMin = Vector(tile[0] * Math.Floor(tileResX),0, tile[2] * Math.Floor(tileResY));
					tileCornerMax = Vector(tileCornerMin[0] + Math.Floor(tileResX),0,tileCornerMin[2] + Math.Floor(tileResY));
					Shape tileShape = Shape.Create(ShapeType.BBOX, ARGB(50,100,100,100), ShapeFlags.NOZBUFFER|ShapeFlags.TRANSP, tileCornerMin, tileCornerMax);
					visualTiles.Insert(tileShape);
					visualCoords.Insert(tile);
					
					coords[0] = coords[0] + tileResX;
				}
				coords[0] = coords[0] - tileResX * m_iSelectionSize;
				coords[2] = coords[2] + tileResY;
			}
		}
	}


	void UpdateSelection(vector mouseCoords)
	{
		float tileResX = (m_API.GetTerrainResolutionX(0) * m_API.GetTerrainUnitScale(0)) / m_API.GetTerrainTilesX(0);
		float tileResY = (m_API.GetTerrainResolutionY(0) * m_API.GetTerrainUnitScale(0)) / m_API.GetTerrainTilesY(0);
		vector tile = Vector(Math.Floor(mouseCoords[0] / Math.Floor(tileResX)), 0, Math.Floor(mouseCoords[2] / Math.Floor(tileResY)));
		
		for(int i = 0; i < visualCoords.Count(); i++)
		{
			if(!selectedCoords.Contains(visualCoords[i]))
			{
				vector tileCornerMin = Vector(visualCoords[i][0] * Math.Floor(tileResX),0, visualCoords[i][2] * Math.Floor(tileResY));
				vector tileCornerMax = Vector(tileCornerMin[0] + Math.Floor(tileResX),0,tileCornerMin[2] + Math.Floor(tileResY));
				Shape selectedShape = Shape.Create(ShapeType.BBOX, ARGB(50,100,100,0), ShapeFlags.NOZBUFFER|ShapeFlags.TRANSP, tileCornerMin, tileCornerMax);
				selectedTiles.Insert(selectedShape);
				selectedCoords.Insert(visualCoords[i]);
			}
			else
			{
				int index = selectedCoords.Find(visualCoords[i]);
				selectedCoords.Remove(index);
				selectedTiles.Remove(index);
			}
		}
	}
	
	override void OnMousePressEvent(float x, float y, WETMouseButtonFlag buttons)
	{
		vector traceStart;
		vector traceEnd;
		vector traceDir;
		if (m_API.TraceWorldPos(x, y, TraceFlags.WORLD, traceStart, traceEnd, traceDir))
			{
				UpdateSelection(traceEnd);
			}
	}

	override void OnActivate()
	{
		EBTConfigPlugin.UpdateRegisteredBlenderExecutables();

	}
	override void OnDeActivate()
	{
		selectedTiles.Clear();
		selectedCoords.Clear();
		shapeTiles.Clear();
		visualTiles.Clear();
		square = null;
		startPosition = "0 0 0";
	}
}
#endif //Workbench
