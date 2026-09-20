# (TODO: your game's title)

Author: (TODO: your name)

Design: (TODO: In two sentences or fewer, describe what is new and interesting about your game.)

Text Drawing: `TextRenderer` shapes a UTF-8 sample line with HarfBuzz, rasterizes glyph IDs with FreeType, and draws cached quads using `TextProgram` and a shared grayscale OpenGL atlas. The current Stage 2 build shows a sample line at 48 physical pixels; story layout, wrapping, and DPI scaling are still pending. Escape exits.

Choices: Authored in Twine (Harlowe) and exported to `assets/story/righteous-knight.twee`. `tools/compile_story.py` validates links and compiles UTF-8 prose and ordered choices into `dist/story.bin`; `Story.cpp` loads the binary and resolves choices by node ID. Gameplay UI integration is still pending. See [asset pipeline instructions](docs/asset-pipeline.md).

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

(TODO: describe the controls and (if needed) goals/strategy.)

Sources: Noto Serif from [Google Fonts](https://github.com/google/fonts/tree/main/ofl/notoserif), licensed under SIL OFL 1.1. See `assets/fonts/OFL.txt` and `assets/fonts/README.md`.

This game was built with [NEST](NEST.md).

