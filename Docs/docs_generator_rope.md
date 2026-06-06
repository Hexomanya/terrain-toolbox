# Rope Generator
> **Category:** Generator
> **Removal Safety:** 📋 Layer  
> **Main File:** [DAB_RopeGeneratorEntity.c](../Scripts/Game/Generators/RopeGenerator/DAB_RopeGeneratorEntity.c)  
> **Dependencies:** None  

---

## Overview

This is a simple generator, that creates powerline cables connecting the points on a polyline. It is mainly used to immitate rope for detailing.

[![Preview Video](https://img.youtube.com/vi/IP-P1bp2PBc/mqdefault.jpg)](https://youtu.be/IP-P1bp2PBc)

---

## Warnings
> ⚠️ This generator does not take any sourounding objects or intended poles into account. So make sure that if it connects with an object (like a pole) that it is indestructible.

> ⚠️ The powerline material does not work well on steep inclines. This is the normal behaviour of the vanilla material, nothing we can do about it.

> ⚠️This generator might show up on the map as a powerline (more testing needed after recent update).

---

## Usage

Use as any other generator. This generator treats splines as polylines. If you want the cable to look like rope you have to change the textures in the rope material to 'ST_Rope_BW_' or similar and adjust the color.

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