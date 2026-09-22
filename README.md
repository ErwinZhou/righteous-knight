# (TODO: your game's title)

Author: (TODO: your name)

Design: (TODO: In two sentences or fewer, describe what is new and interesting about your game.)

Text Drawing: `TextLayout` wraps UTF-8 text with HarfBuzz and preserves explicit line breaks. `TextRenderer` rasterizes glyph IDs with FreeType and draws cached quads from shared grayscale OpenGL atlas pages. The opening passage uses 24 logical pixels with DPI-aware rasterization; resizing rebuilds layout and changing DPI replaces the old font-size cache. The reading area scrolls and clips content above a fixed controls footer. Choices wrap to their measured height and support mouse focus or Up/Down navigation. Enter or a click confirms the selected choice. R opens a restart confirmation; Escape cancels that prompt or exits during reading.

Choices: Authored in Twine (Harlowe) and exported to `assets/story/righteous-knight.twee`. `tools/compile_story.py` validates links and compiles UTF-8 prose and ordered choices into `dist/story.bin`; `Story.cpp` loads the binary and resolves choices by node ID. The reading UI resolves keyboard and mouse confirmations through the compiled choice targets, including ending-to-start links. See [asset pipeline instructions](docs/asset-pipeline.md).

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

Read each passage and choose how the knight responds.

- Up/Down selects an option; Enter confirms it, or click an option directly
- Scroll with the wheel, PageUp/PageDown, or Home/End
- R opens a restart prompt; choose Restart or Keep reading with the keyboard or mouse
- Escape cancels the restart prompt or exits during reading
- PrintScreen saves a screenshot

The good and bad ending screens also contain a link back to the beginning.

Sources: Noto Serif from [Google Fonts](https://github.com/google/fonts/tree/main/ofl/notoserif), licensed under SIL OFL 1.1. See `assets/fonts/OFL.txt` and `assets/fonts/README.md`.

This game was built with [NEST](NEST.md).

