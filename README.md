# (TODO: your game's title)

Author: (TODO: your name)

Design: (TODO: In two sentences or fewer, describe what is new and interesting about your game.)

Text Drawing: `TextLayout` wraps UTF-8 text with HarfBuzz and preserves explicit line breaks. `TextRenderer` rasterizes glyph IDs with FreeType and draws cached quads from shared grayscale OpenGL atlas pages. The opening passage uses 24 logical pixels with DPI-aware rasterization; resizing rebuilds layout and changing DPI replaces the old font-size cache. Scrolling and choice interaction are still pending. Escape exits.

Choices: Authored in Twine (Harlowe) and exported to `assets/story/righteous-knight.twee`. `tools/compile_story.py` validates links and compiles UTF-8 prose and ordered choices into `dist/story.bin`; `Story.cpp` loads the binary and resolves choices by node ID. Gameplay UI integration is still pending. See [asset pipeline instructions](docs/asset-pipeline.md).

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

(TODO: describe the controls and (if needed) goals/strategy.)

Sources: Noto Serif from [Google Fonts](https://github.com/google/fonts/tree/main/ofl/notoserif), licensed under SIL OFL 1.1. See `assets/fonts/OFL.txt` and `assets/fonts/README.md`.

This game was built with [NEST](NEST.md).

