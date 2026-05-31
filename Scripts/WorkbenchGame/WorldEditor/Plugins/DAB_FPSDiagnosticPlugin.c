#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "FPS Diagnostic - Extended",
	description: "Collect FPS all over the terrain and create a heatmap of it",
	shortcut: "",
	category: "DAB - Extensions",
	wbModules: { "WorldEditor" },
	awesomeFontCode: 0xF625)]
class DAB_FPSDiagnosticPlugin : SCR_FPSDiagnosticPlugin
{
	[Attribute(defvalue: "0 0 0", desc: "Used if != vector.zero and m_bSize is > 0", category: "Overwrite Area")]
	protected vector m_vBottomLeftCorner;
	
	[Attribute(defvalue: "-1", desc: "Width & Height of the area. Used if > 0", category: "Overwrite Area")]
	protected float m_fSize;

	protected override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			SCR_WorkbenchHelper.PrintDialog("World Editor is not available", level: LogLevel.ERROR);
			return;
		}

		WorldEditorAPI worldEditorAPI = worldEditor.GetApi();
		if (!worldEditorAPI)
		{
			SCR_WorkbenchHelper.PrintDialog("World Editor API is not available", level: LogLevel.ERROR);
			return;
		}

		if (!SCR_WorldEditorToolHelper.IsWorldLoaded())
		{
			SCR_WorkbenchHelper.PrintDialog("Be sure to load a world (or have it saved) before using the FPS Diagnostic plugin.");
			return;
		}

		if (worldEditor.IsPrefabEditMode())
		{
			SCR_WorkbenchHelper.PrintDialog("Be sure to load a world (or have it saved) before using the FPS Diagnostic plugin.");
			return;
		}

		IEntitySource terrainEntity = SCR_WorldEditorToolHelper.GetTerrainEntitySource();
		if (!terrainEntity)
		{
			SCR_WorkbenchHelper.PrintDialog("Be sure to have a terrain entity (of type GenericTerrainEntity) before using the FPS Diagnostic plugin.");
			return;
		}

		if (Workbench.ScriptDialog("FPS Diagnostic", "", this) == 0)
			return;

		vector areaOrigin;
		if (!terrainEntity.Get("coords", areaOrigin))
			return;
		
		vector terrainDimensions =  SCR_WorldEditorToolHelper.GetTerrainDimensions();
		vector areaDimensions;
		if(m_fSize <= 0)
			areaDimensions = terrainDimensions;
		else 
			areaDimensions = Vector(m_fSize, 0, m_fSize);
			
		
		if(vector.DistanceSqXZ(m_vBottomLeftCorner, areaOrigin) > DAB_Constants.EPSILONSQ && m_fSize > 0){
			areaOrigin = areaOrigin + m_vBottomLeftCorner;
		} 
		
		if (m_iHeatmapDefinition < 1)
			m_iHeatmapDefinition = DEFINITION_DEFAULT;

		float pixelWidth = areaDimensions[0] / m_iHeatmapDefinition;
		float pixelHeight = areaDimensions[2] / m_iHeatmapDefinition;

		if (pixelWidth < 0.1 || pixelHeight < 0.1)
		{
			SCR_WorkbenchHelper.PrintFormatDialog("Invalid pixel size (%1×%2) - try setting a lower definition.", pixelWidth.ToString(), pixelHeight.ToString(), level: LogLevel.WARNING);
			return;
		}

		BaseWorld world = worldEditorAPI.GetWorld();

		bool isOceanEnabled = world.IsOcean();
		float oceanBaseHeight;
		if (isOceanEnabled)
			oceanBaseHeight = world.GetOceanBaseHeight();

		array<int> oceanIndices;
		if (!m_bAnalyseOnOcean && isOceanEnabled)
			oceanIndices = {};

		
		map<int, vector> positionsMap = new map<int, vector>();
		int pixelIndex;
		// row-major order
		for (int z = m_iHeatmapDefinition - 1; z >= 0; z--) //Backwards for image flip?
		{
			float zPos = areaOrigin[2] + (z + 0.5) * pixelHeight;
			for (int x; x < m_iHeatmapDefinition; x++)
			{
				float xPos = areaOrigin[0] + (x + 0.5) * pixelWidth;
				if(IsOutOfWorldBounds(xPos, zPos, terrainDimensions))
				{
					++pixelIndex;
					continue;
				}
				
				float yPos = worldEditorAPI.GetTerrainSurfaceY(xPos, zPos);

				if (!m_bAnalyseOnOcean && isOceanEnabled && yPos < oceanBaseHeight) // allow ocean's base height
					oceanIndices.Insert(pixelIndex);

				positionsMap.Insert(pixelIndex, { xPos, yPos, zPos });
				++pixelIndex;
			}
		}

		int positionsCount = positionsMap.Count();
		
		// Impossible
		/*if (positionsCount < 1)
		{
			SCR_WorkbenchHelper.PrintDialog("No relevant positions were found - try setting a higher definition.");
			return;
		}*/

		int oceanIndicesCount;
		if (oceanIndices)
			oceanIndicesCount = oceanIndices.Count();

		int relevantPositionsCount = positionsCount - oceanIndicesCount;
		if (relevantPositionsCount < 1)
		{
			SCR_WorkbenchHelper.PrintDialog("No non-water positions were found - try setting a higher definition.");
			return;
		}

		int orientationsCount = m_aOrientations.Count();
		if (orientationsCount < 1)
		{
			if (CAMERA_DEFAULT_ANGLE_COUNT < 1)
			{
				SCR_WorkbenchHelper.PrintDialog("No orientations provided - add at least one camera orientation.");
				return;
			}

			for (int i; i < CAMERA_DEFAULT_ANGLE_COUNT; ++i)
			{
				m_aOrientations.Insert({ CAMERA_DEFAULT_PITCH, i * 360 / CAMERA_DEFAULT_ANGLE_COUNT, 0 });
				++orientationsCount;
			}
		}

		int relevantScenesCount = relevantPositionsCount * orientationsCount;
		if (relevantScenesCount < 1)
		{
			SCR_WorkbenchHelper.PrintDialog("No scenes created - no proper positions found.");
			return; // safety
		}

		string captionPrefix;
		if (m_bUseFakeData)
			captionPrefix = "[DEBUG] ";

		if (Workbench.ScriptDialog(
			captionPrefix + "FPS Diagnostic",
			string.Format(
				"The plugin will process %1 positions in %2 angle(s) for a total of %3 scenes (every %4m)",
				relevantPositionsCount,
				orientationsCount,
				relevantScenesCount,
				pixelWidth.ToString(-1, 1))
			+ string.Format(
				"\nHeatmap definition: %1×%1 stretched ×%2 to %3×%3",
				m_iHeatmapDefinition,
				m_iHeatmapResolutionFactor,
				m_iHeatmapDefinition * m_iHeatmapResolutionFactor)
			+ string.Format(
				"\n\n%1 (hh:ii:ss)estimated duration with %2s waiting time per scene (theoretical duration ×%3)"
				+ "\n\nDo NOT touch or close the Workbench during that time"
				+ "\nDo NOT unfocus the Workbench (unless -forceUpdate is used or you want to abort the benchmark)"
				+ "\n\nA waiting time of %4s will happen before the benchmark begins.",
				SCR_FormatHelper.FormatTime(m_fStartDelay + relevantScenesCount * m_fScenePause * ESTIMATE_FACTOR),
				m_fScenePause,
				ESTIMATE_FACTOR,
				m_fStartDelay),
				new WorkbenchDialog_OKCancel()) == 0)
			return;

		float lowestFPS = float.MAX;
		float highestFPS;
		float addedFPS;
		array<float> fpsArray = {};
		int valueCount = m_iHeatmapDefinition * m_iHeatmapDefinition;
		fpsArray.Resize(valueCount * orientationsCount);
		
		for(int i = 0; i < fpsArray.Count(); i++)
			fpsArray[i] = -1;

		int width, height;
		array<int> resolutionList = {};
		resolutionList.Reserve(relevantScenesCount * 2); // not resize

		int validPositionsDone;

		// randomisation
		array<int> keys = SCR_MapHelperT<int, vector>.GetKeys(positionsMap);
		if (m_bRandomisePositions)
		{
			 if (positionsCount <= SCR_Math.MAX_RANDOM)
				SCR_ArrayHelperT<int>.Shuffle(keys);
			else
				PrintFormat("Too many (%1 > %2) positions to randomise", positionsCount, SCR_Math.MAX_RANDOM, level: LogLevel.WARNING);
		}

		// progress and estimate
		string estimatedDuration = SCR_FormatHelper.FormatTime(relevantScenesCount * m_fScenePause * ESTIMATE_FACTOR);
		Print("Estimated duration: " + estimatedDuration, LogLevel.NORMAL);

		float prevProgress, currProgress;
		string progressText;
		if (!m_bInternalUseGameMode)
		{
			if (m_bUseFakeData)
				progressText = "[DEBUG] Generating scenes's performance...\nEstimated time left: %1";
			else
				progressText = "Capturing scenes's performance...\nEstimated time left: %1";
		}

		WBProgressDialog progress;

		m_bInternalUseGameMode = m_bUseGameMode && !m_bUseFakeData;		

		vector cameraPos, traceEnd, cameraDir;
		int screenWidth = worldEditorAPI.GetScreenWidth();
		int screenHeight = worldEditorAPI.GetScreenHeight();;
		if (!m_bInternalUseGameMode) // WorkbenchCamera
			worldEditorAPI.TraceWorldPos(screenWidth * 0.5, screenHeight * 0.5, TraceFlags.WORLD, cameraPos, traceEnd, cameraDir);

		CameraBase camera;

		// enter game mode
		if (m_bInternalUseGameMode)
		{
			worldEditor.SwitchToGameMode(fullScreen: m_bUseFullScreen);

			while (!worldEditorAPI.IsGameMode())
			{
				if (float.AlmostEqual(System.GetFPS(), FPS_UNFOCUSED))
				{
					Print("Switching to Game mode got unfocused - capture cancelled", LogLevel.ERROR);
					worldEditor.SwitchToEditMode();
					return;
				}

				Sleep(100); // needed
			}

			SCR_CameraManager cameraManager = SCR_CameraManager.Cast(GetGame().GetCameraManager());
			if (!cameraManager)
			{
				SCR_WorkbenchHelper.PrintDialog("Camera Manager cannot be found", level: LogLevel.ERROR);
				worldEditor.SwitchToEditMode();
				return;
			}

			camera = CameraBase.Cast(GetGame().SpawnEntity(CameraBase));
			if (!camera)
			{
				SCR_WorkbenchHelper.PrintDialog("Camera cannot be created", level: LogLevel.ERROR);
				worldEditor.SwitchToEditMode();
				return;
			}

			cameraManager.SetCamera(camera);
		}
		else
		{
			progress = new WBProgressDialog(string.Format(progressText, estimatedDuration), worldEditor);
		}

		if (!m_bUseFakeData && m_fStartDelay > 0)
		{
			PlaceCameraCorrect(worldEditorAPI, camera, areaOrigin + m_vPositionOffset, -vector.Up);

			Print("Starting in " + m_fStartDelay + "s, unfocus the Workbench to cancel", LogLevel.NORMAL);
			if (!WaitFocused(m_fStartDelay))
			{
				PlaceCameraCorrect(worldEditorAPI, camera, cameraPos, cameraDir);
				Sleep(1);
				SCR_WorkbenchHelper.PrintDialog("Workbench was unfocused - capture cancelled");
				if (m_bInternalUseGameMode)
					worldEditor.SwitchToEditMode();

				return;
			}

			Print("Starting", LogLevel.NORMAL);
		}

		if (float.AlmostEqual(System.GetFPS(), FPS_UNFOCUSED, FPS_UNFOCUSED_DELTA))
		{
			SCR_WorkbenchHelper.PrintDialog("Workbench is unfocused on start - capture cancelled");
			if (m_bInternalUseGameMode)
				worldEditor.SwitchToEditMode();

			return;
		}

		const int startTime = System.GetTickCount();
		m_iLastEstimateTick = startTime;

		Debug.BeginTimeMeasure();

		// foreach (int index, vector position : positionsMap)
		foreach (int index : keys)
		{
			vector position = positionsMap.Get(index);
			
			if(position[0] < areaOrigin[0] || position[2] < areaOrigin[2]) PrintFormat("Position %1 is behind areaOrigin: %2!", position, areaOrigin);
			else PrintFormat("Normal position: %1", position);

			if (m_iTimeEstimateFrequency > 0 && System.GetTickCount(m_iLastEstimateTick) >= m_iTimeEstimateFrequency * 1000)
			{
				int now = System.GetTickCount();
				int spentTime = now - startTime;

				if (currProgress > 0)
				{
					float timePerPercentage = (spentTime - m_fStartDelay) / currProgress;
					float estimatedRemaining = timePerPercentage * (1 - currProgress);

					if (progress) // refresh text - currently the only way is to regreate the progress dialog
					{
						progress = new WBProgressDialog(string.Format(progressText, SCR_FormatHelper.FormatTime(estimatedRemaining * 0.001 * ESTIMATE_FACTOR)), worldEditor);
						progress.SetProgress(currProgress);
					}

					PrintFormat(
						"Progress: %1%%; Spent time: %2 Estimated remaining time: %3",
						(currProgress * 100).ToString(-1, 2),
						SCR_FormatHelper.FormatTime(spentTime * 0.001),
						SCR_FormatHelper.FormatTime(estimatedRemaining * 0.001 * ESTIMATE_FACTOR),
						level: LogLevel.NORMAL);

					m_iLastEstimateTick = now;
				}
			}

			// avoid ocean if needed, is already set to -1
			if (oceanIndices && oceanIndices.Contains(index)) continue;

			++validPositionsDone;
			currProgress = validPositionsDone / relevantPositionsCount;
			if (currProgress - prevProgress >= 0.01)		// min 1%
			{
				if (progress)
				{
					if (currProgress > 0)
					{
						int now = System.GetTickCount();
						int spentTime = now - startTime;
						float timePerPercentage = (spentTime - m_fStartDelay) / currProgress;
						float estimatedRemaining = timePerPercentage * (1 - currProgress);
						progress = new WBProgressDialog(string.Format(progressText, SCR_FormatHelper.FormatTime(estimatedRemaining * 0.001 * ESTIMATE_FACTOR)), worldEditor);
					}

					progress.SetProgress(currProgress);		// expensive
				}

				prevProgress = currProgress;
			}

			foreach (int orientationIndex, vector orientation : m_aOrientations)
			{
				float sceneFPS;
				if (m_bUseFakeData)
				{
					if (float.AlmostEqual(System.GetFPS(), FPS_UNFOCUSED))
					{
						SCR_WorkbenchHelper.PrintDialog("Workbench was unfocused - capture cancelled");
						worldEditor.SwitchToEditMode();
						return;
					}

					sceneFPS = SCR_Math.RandomGaussFloat(FPS_RANDOM_MIN, FPS_RANDOM_MID, FPS_RANDOM_MAX);
				}
				else
				{
					if (m_bInternalUseGameMode && !worldEditorAPI.IsGameMode())
					{
						SCR_WorkbenchHelper.PrintDialog("Workbench Game Mode was exited - capture cancelled");
						return;
					}

					PlaceCameraCorrect(worldEditorAPI, camera, position + m_vPositionOffset, orientation);

					if (!WaitFocused(m_fScenePause))
					{
						PlaceCameraCorrect(worldEditorAPI, camera, position + m_vPositionOffset, orientation);
						Sleep(1);
						progress = null;
						SCR_WorkbenchHelper.PrintDialog("Workbench was unfocused - capture cancelled");
						if (m_bInternalUseGameMode)
							worldEditor.SwitchToEditMode();

						return;
					}

					PlaceCameraCorrect(worldEditorAPI, camera, position + m_vPositionOffset, orientation);
					sceneFPS = System.GetFPS();
					int waitTime = FPS_HIGH_DURATION_MS;
					while (sceneFPS > MAX_FPS)
					{
						Sleep(FPS_CHECK_FREQUENCY_MS);
						PlaceCameraCorrect(worldEditorAPI, camera, position + m_vPositionOffset, orientation);
						sceneFPS = System.GetFPS();
						waitTime -= FPS_CHECK_FREQUENCY_MS;
						if (waitTime < 1)
						{
							worldEditorAPI.SetCamera(cameraPos, cameraDir);
							Sleep(1);
							SCR_WorkbenchHelper.PrintFormatDialog("FPS > %1 for at least %2! Use VSync if needed", MAX_FPS.ToString(), (FPS_HIGH_DURATION_MS * 0.001).ToString(), level: LogLevel.ERROR);
							if (m_bInternalUseGameMode)
								worldEditor.SwitchToEditMode();

							return;
						}
					}

					waitTime = FPS_UNFOCUSED_DURATION_MS;
					while (float.AlmostEqual(sceneFPS, FPS_UNFOCUSED, FPS_UNFOCUSED_DELTA))
					{
						Sleep(FPS_CHECK_FREQUENCY_MS);
						PlaceCameraCorrect(worldEditorAPI, camera, position + m_vPositionOffset, orientation);
						sceneFPS = System.GetFPS();
						waitTime -= FPS_CHECK_FREQUENCY_MS;
						if (waitTime < 1)
						{
							PlaceCameraCorrect(worldEditorAPI, camera, cameraPos, cameraDir);
							Sleep(1);
							progress = null;
							SCR_WorkbenchHelper.PrintDialog("Workbench was unfocused - capture cancelled");							
							if (m_bInternalUseGameMode)
								worldEditor.SwitchToEditMode();

							return;
						}
					}
				}

				fpsArray[index * orientationsCount + orientationIndex] = sceneFPS;
				addedFPS += sceneFPS;

				if (sceneFPS < lowestFPS)
					lowestFPS = sceneFPS;
				else
				if (sceneFPS > highestFPS)
					highestFPS = sceneFPS;

				resolutionList.Insert(worldEditorAPI.GetScreenWidth());
				resolutionList.Insert(worldEditorAPI.GetScreenHeight());
			}
		}

		Debug.EndTimeMeasure("Measuring " + relevantScenesCount + " scenes");

		if (m_bInternalUseGameMode)
			worldEditor.SwitchToEditMode();
		else
			PlaceCameraCorrect(worldEditorAPI, camera, cameraPos, cameraDir);

		array<float> medianArray = {};
		medianArray.Copy(fpsArray);
		medianArray.RemoveItem(-1);
		medianArray.Sort();

		float medianFPS;
		if (!medianArray.IsEmpty())
			medianFPS = medianArray[medianArray.Count() * 0.5];

		Print("Average FPS: " + addedFPS / relevantScenesCount, LogLevel.NORMAL);
		Print("Median  FPS: " + medianFPS, LogLevel.NORMAL);
		Print("Highest FPS: " + highestFPS, LogLevel.NORMAL);
		Print("Lowest  FPS: " + lowestFPS, LogLevel.NORMAL);

		int resolutionListCount = resolutionList.Count();

		bool differentResolutions;
		for (int i; i < resolutionListCount; i += 2) // step 2
		{
			if (i == 0)
			{
				width = resolutionList[0];
				height = resolutionList[1];
			}
			else // compare
			{
				if (width != resolutionList[i] || height != resolutionList[i + 1])
				{
					differentResolutions = true;
					break;
				}
			}
		}

		if (differentResolutions)
		{
			int avgWidth, avgHeight;
			for (int i; i < resolutionListCount; i += 2) // step 2
			{
				avgWidth += resolutionList[i];
				avgHeight += resolutionList[i + 1];
			}

			PrintFormat("Resolution was changed during the FPS test! Started with %1×%2. Average resolution: %3×%4", screenWidth, screenHeight, avgWidth / relevantScenesCount, avgHeight / relevantScenesCount, level: LogLevel.WARNING);
		}
		else
		{
			PrintFormat("Resolution: %1×%2", width, height, level: LogLevel.NORMAL);
		}

		string colourMode;
		if (m_iHeatmapColourMode == SCR_HeatmapHelper.COLOUR_MODE_ALPHA)
			colourMode = "Alpha";
		else
		if (m_iHeatmapColourMode == SCR_HeatmapHelper.COLOUR_MODE_THERMAL)
			colourMode = "RGB";
		else
//		if (m_iHeatmapColourMode == SCR_HeatmapHelper.COLOUR_MODE_GREYSCALE) // default
			colourMode = "BW";

		if (m_bHeatmapValueInversion)
			colourMode += "inv";

		string worldName = SCR_WorldEditorToolHelper.GetWorldName();
		string fileName = string.Format(OUTPUT_HEATMAP_NAME, worldName, colourMode, m_iHeatmapDefinition);
		string absoluteFileName;
		if (!Workbench.GetAbsolutePath(fileName, absoluteFileName, false) || !CreateImage(absoluteFileName, m_iHeatmapDefinition, fpsArray))
		{
			Print("Heatmap cannot be created at " + absoluteFileName, LogLevel.ERROR);
			return;
		}

		Print("Heatmap successfully created at " + absoluteFileName, LogLevel.NORMAL);
		absoluteFileName.Replace("/", "\\");
		if (m_iOpenHeatmap == 1 || m_iOpenHeatmap == 2)
			Workbench.RunCmd("explorer \"" + FilePath.StripFileName(absoluteFileName) + "\"");

		if (m_iOpenHeatmap == 0 || m_iOpenHeatmap == 2)
			Workbench.RunCmd("explorer \"file:/" + SCR_StringHelper.DOUBLE_SLASH + absoluteFileName + "\"");
	}
	
	protected bool IsOutOfWorldBounds(float xPos, float zPos, vector terrainDimensions)
	{
		// We asume that the terrain is 'correctly' setup and does not go into negative.
		return xPos < 0 || zPos < 0 || xPos > terrainDimensions[0] || zPos > terrainDimensions[2];
	}
	
	//angles = Pitch, Roll, Yaw
	protected void PlaceCameraCorrect(notnull WorldEditorAPI worldEditorAPI, CameraBase camera, vector position, vector angles)
	{
		if (m_bInternalUseGameMode)
		{
			World world = GetGame().GetWorld();
			if (!world)
				return; // leaving game mode

			camera.SetOrigin(position + m_vPositionOffset);
			camera.SetAngles(angles);
		}
		else
		{
			worldEditorAPI.SetCamera(position + m_vPositionOffset, angles.AnglesToVector());
		}
	}
}
#endif //Workbench