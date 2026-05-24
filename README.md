<p align="center">
  <img src="Docs/terrainToolbox_titleImage.png" alt="Terrain Toolbox Thumbnail" width="600"/>
</p>

# Terrain Toolbox – Arma Reforger Workbench Scripts

A collection of **generators**, **tools**, and **plugins** for the Arma Reforger Workbench World Editor.  

**Notes:** This toolbox is designed exclusively for the Arma Reforger Workbench. It operates strictly within the editor and has **no effect on gameplay**. Please ensure you read the removal warnings for each script before using them in your layer files.

Some plugins and tools depend on shared helper classes included in this repository. If you are extracting only specific scripts rather than downloading the entire package, please check the source code to ensure you also include any required dependencies.


> **Full Documentation:**  The full documentation for each element are being worked on. For this initial version, this README serves as the complete, standalone guide for all features, tools, and plugins.

---

## Removal Guide

| Mark | Meaning |
|------|---------|
| 🟢 **Safe** | Script files can be deleted at any time without side effects. |
| 🗂️ **Files** | Removing the scripts also requires deleting any generated asset files (e.g. config files) you created alongside them. No save files are touched. |
| 📋 **Layer** | The script stores custom classes or attributes on entities within your layer files. If you remove the scripts, orphaned data may be left behind. While this can be handled by the workbench, it could also lead to you needing to manually edit or clean the affected layer files. |

---

## Generators

### 📋 Forest Generator Extension
Adds slope-aware tree placement and automatic outline generation to the built-in Forest Generator.

[![Forest Generator Preview](https://img.youtube.com/vi/TCFK994eGjY/mqdefault.jpg)](https://youtu.be/TCFK994eGjY)
---

### 📋 Rope Generator
Creates rope / powerline cable segments between consecutive points on a polyline path.

[![Rope Generator Preview](https://img.youtube.com/vi/IP-P1bp2PBc/mqdefault.jpg)](https://youtu.be/IP-P1bp2PBc)

---

## Tools

### 🟢 Correct Spline Slope
Clamps slope angles on selected splines within a configurable min/max range. Can be used to ensure that rivers only flow downhill.

[![Correct Spline Slope Preview](https://img.youtube.com/vi/lmElxZc0Qtw/mqdefault.jpg)](https://youtu.be/lmElxZc0Qtw)

---

### 🗂️ Prefab Randomizer
It replaces prefabs with weighted-random or in-order alternatives. It is intended for a workflow in which you place basic props down first (e.g. outdoor tables) and then, if performance permits, quickly add details to them later.
Requires `.conf` asset files. Delete those alongside the scripts when removing.

[![Prefab Randomizer Preview](https://img.youtube.com/vi/lGOzpt-WFa0/mqdefault.jpg)](https://youtu.be/lGOzpt-WFa0)

> ⚠️ **Warning: "Relative_Y":** Since setting the Relative_Y flag on objects that have the flag set by default is bugged on BIs side, swapping such objects with non-Relative_Y objects may impair the functionality.

---

### 🟢 Select Objects over Surface
Finds objects placed on unwanted terrain surfaces (water, roads, custom materials) and optionally moves them to a valid position.

[![Select Objects over Surface Preview](https://img.youtube.com/vi/LFBXAONxYMA/mqdefault.jpg)](https://youtu.be/LFBXAONxYMA)

---

## Plugins

Plugins are one-click actions found under **World Editor → Plugins → DAB - Misc**.  
All plugins are 🟢 **Safe** — they only write to standard entity properties and leave no custom data behind.

### Copy Transform [`Ctrl+Shift+W`]
Copies the full transform (position, rotation, placement mode, Relative_Y flag) from the last selected object to all others. Used to manually build walls and sidewalks.

[![Copy Transform Preview](https://img.youtube.com/vi/N6hEQnocGwU/mqdefault.jpg)](https://youtu.be/N6hEQnocGwU)

> ⚠️ **Warning: "Relative_Y":** Since setting the Relative_Y flag on objects that have the flag set by default is bugged on BIs side, you will have to match the Relative_Y flag manually before using the plugin.

---

### Copy Rotation [`Shift+Alt+W`]
Copies the Y-rotation from the last selected object to all other selected objects.

[![Copy Rotation Preview](https://img.youtube.com/vi/C_d7pB7F2CY/mqdefault.jpg)](https://youtu.be/C_d7pB7F2CY)

---

### Calculate Polyline Area
Prints the XZ-plane area of each selected polyline to the Workbench log (in km²).

---

### Fix Road Mat Sort Settings
Migrates legacy `MatSortBias` values on road entities to the correct `RoadSort` field across the entire world.

---

### Switch Relative_Y [`Ctrl+Shift+Q`]
Toggles the Relative_Y flag and matching placement mode on selected entities.

> ⚠️ **Warning: "Relative_Y":** Since setting the Relative_Y flag on objects that have the flag set by default is bugged on BIs side, this will only works on objects that do not have the flag set by default.

---

## Installation

1. Copy the `Scripts/` folder into your Arma Reforger addon project so that the paths merge with your existing `Scripts/Game/` and `Scripts/WorkbenchGame/` directories.
2. Reload the Workbench (or compile scripts via **Script Editor → Compile All**).
3. Plugins appear under **World Editor → Plugins → DAB - Misc**. Tools are visible at the top.

---

## Development Notes

This project was written manually. AI tools were only used for:

* Bug identification
* Formatting and documentation

All core tool logic and codebase functionality were developed entirely by a human.