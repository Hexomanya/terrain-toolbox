# Correct Spline Slope Tool 

> **Category:** Tool  
> **Removal Safety:** 🟢 Safe  
> **Main File:** [DAB_CorrectSelectedSplineSlopeTool.c](../Scripts/WorkbenchGame/WorldEditor/Tools/CorrectSplineTool/DAB_CorrectSelectedSplineSlopeTool.c)  
> **Dependencies:** `DAB_ShapeHelper`, `DAB_EntityHelper`  
> **Tool Symbol:** `slash`  
> **Hotkey:** None
---

## Overview

This tool clamps slope angles on selected splines within a configurable min/max range. It can be used to ensure that rivers only flow downhill.

[![Preview Video](https://img.youtube.com/vi/lmElxZc0Qtw/mqdefault.jpg)](https://youtu.be/lmElxZc0Qtw)

---

## Warnings
> ⚠️ The 'Correct Slopes' button will override your manually set tangents.

> ⚠️ After using this tool on spline that has a child that is marked as `Editor Only` the spline will become `Editor Only` too. You need to reset this flag manually.

> ⚠️ When you use this tool on a spline that holds an entity that manipulates the heightmap, you can an error for every point moved. These should be harmless and can be ignored.
---

## Usage

Both buttons work on every spline you've got selected.

**Analyze Spline:** This button will analyze the spline and return it's heighest, lowest and average slope. The spline is treated as a polyline for this.

**Correct Slopes:** This button clamps the slope of each section of the spline. It will either use the heighest or lowest startpint as a fixed point which will not be moved. It then uses either the `Fritsch–Carlson Monotonicity Filter` or the `Hyman’s Monotonicity Filter` to ensure, that the tangents do not generated unwanted dips or hills in the spline. 

//TODO Continue

---

## Attributes Reference

### [Rope Settings]

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `Rope Material` | `ResourceName` | `.../default_powerline_wire.emat` | MatPBRCable Material. This will determine the look and behaviour of the rope.|
| `Rope Prefab` | `ResourceName` | `.../Powerline.et` | Powerline prefab that will be spawned. Default normally doesn't need  to be changed. |

## Removal

Since it is a generator it will be written to the layer file. When removed, you should get an error, that it can not be found. It will then show up as a GenericEntity and can be deleted from the map. 

In some cases removing a generator did lead to the map no being able to load. In those cases you need to open the offending layer file with a text editor and remove the generator object manually.

---