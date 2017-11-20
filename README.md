# Project-Marshmallow
Vulkan-based implementation of clouds from Decima Engine

Note to contributors: clone this repo with the --recursive flag

# Milestone 1

The goal for the first week of this project was to get a (simple?) Vulkan base code up and running. At the moment, we have a couple of graphics pipelines working
as well as a compute pipeline, which renders to texture and will ultimately be used for the raymarching of clouds, which is our goal for this coming week. Lastly, 
there are some basic camera controls implemented which are bound to the keyboard and eventually, also the mouse.

# Credits: 
https://vulkan-tutorial.com/Introduction - Base code creation / explanation for the graphics pipeline
https://github.com/SaschaWillems/Vulkan - Additional Vulkan reference code, heavily relied upon for compute pipeline
https://github.com/PacktPublishing/Vulkan-Cookbook/ - Even more Vulkan reference that helped with rendering to texture