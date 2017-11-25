# Project-Marshmallow
Vulkan-based implementation of clouds from Decima Engine

This project is built for 64bit Windows and uses precompiled libs.

# Overview

# Features

# Performance

# Milestone Progress

## Milestone 1

- Made a basic Vulkan codebase from scratch.
- 3 Working pipelines. Screen space background, graphics pipeline, and compute pipeline.
- Compute pipeline writes an image to be displayed in the background
- Notes, research, planning from papers.

Presentation slides: https://docs.google.com/presentation/d/1VIR9ZQW38At9B_MwrqZS0Uuhs5h2Mxhj84UH62NDxYU/edit?usp=sharing

## Milestone 2

- Transitioned to 64bit.
- Reorganized codebase. Better abstractions, encapsulated classes. Generally moved away from initial tutorial.
- Obj loading with Tinyobj
- Began raymarching in compute pipeline

# Credits: 
https://vulkan-tutorial.com/Introduction - Base code creation / explanation for the graphics pipeline

https://github.com/SaschaWillems/Vulkan - Additional Vulkan reference code, heavily relied upon for compute pipeline

https://github.com/PacktPublishing/Vulkan-Cookbook/ - Even more Vulkan reference that helped with rendering to texture

https://github.com/moneimne and https://github.com/byumjin - Significant help on learning and properly using Vulkan. Check out their stuff!

## Libraries:
https://github.com/syoyo/tinyobjloader - OBJ loading in a single header

http://www.glfw.org/ - Vulkan application utilities for Windows

https://github.com/nothings/stb - Image loading in a single header