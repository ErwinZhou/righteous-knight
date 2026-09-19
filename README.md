# (TODO: your game's title)

Author: (TODO: your name)

Design: (TODO: In two sentences or fewer, describe what is new and interesting about your game.)

Text Drawing: (TODO: how does the text drawing in this game work? Is text precomputed? Rendered at runtime? What files or utilities are involved?)

Choices: Authored in Twine (Harlowe) and exported to `assets/story/righteous-knight.twee`. `tools/compile_story.py` validates links and compiles UTF-8 prose and ordered choices into `dist/story.bin`; `Story.cpp` loads the binary and resolves choices by node ID. Gameplay UI integration is still pending. See [asset pipeline instructions](docs/asset-pipeline.md).

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

(TODO: describe the controls and (if needed) goals/strategy.)

Sources: (TODO: list a source URL for any assets you did not create yourself. Make sure you have a license for the asset.)

This game was built with [NEST](NEST.md).

