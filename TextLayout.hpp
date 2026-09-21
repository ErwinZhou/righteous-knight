#pragma once
#include "FontAsset.hpp"
#include <hb.h>
#include <cstdint>
#include <string>
#include <vector>

struct PositionedGlyph {
    uint32_t id, cluster;
    float x_advance, y_advance, x_offset, y_offset;
};

struct TextLine {
    size_t begin = 0, end = 0;
    float baseline = 0, width = 0;
    std::vector<PositionedGlyph> glyphs;
};

struct TextBlock {
    std::vector<TextLine> lines;
    float width = 0, height = 0;
};

// layout uses logical pixels and owns no graphics resources
class TextLayout {
public:
    TextLayout(FontAsset &font, unsigned pixel_size, float density = 1.0f);
    ~TextLayout();
    TextLayout(TextLayout const &) = delete;
    TextLayout &operator=(TextLayout const &) = delete;
    TextLine shape(std::string const &text);
    TextBlock wrap(std::string const &text, float width);
    void activate_size();
    float line_height() const { return height; }
    float ascender() const { return ascent; }
    float pixel_density() const { return density; }
    size_t shape_calls() const { return calls; }
private:
    FontAsset &font;
    unsigned pixel_size;
    float density, height, ascent;
    hb_font_t *hb_font = nullptr;
    size_t calls = 0;
};
