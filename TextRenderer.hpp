#pragma once
#include "FontAsset.hpp"
#include "TextProgram.hpp"
#include <hb.h>
#include <glm/glm.hpp>
#include <map>
#include <vector>

// one font and pixel size per renderer
class TextRenderer {
public:
    // creates shaping and graphics resources for a fixed pixel size with an active GL context
    TextRenderer(FontAsset &font, unsigned pixel_size);
    // frees owned resources before the GL context is destroyed
    ~TextRenderer();
    // prevents copies from sharing ownership of the same resource handles
    TextRenderer(TextRenderer const &) = delete;
    TextRenderer &operator=(TextRenderer const &) = delete;
    // shapes a UTF-8 line and uploads its geometry only when the text changes
    void set_line(std::string const &text);
    // draws the prepared line at a baseline measured in pixels from the viewport top left
    void draw(glm::uvec2 viewport, glm::vec2 baseline, glm::vec4 color);
    // counts unique glyphs cached for reuse across lines
    size_t cached_glyphs() const { return glyphs.size(); }
    // counts glyphs in the current shaped line including spaces
    size_t shaped_glyphs() const { return shaped_count; }
    // returns the horizontal pen movement in pixels rather than the visible ink width
    float advance() const { return line_advance; }
private:
    // stores a glyph bitmap size and placement relative to the baseline and atlas
    struct Glyph {
        int width = 0, height = 0, left = 0, top = 0;
        int x = 0, y = 0;
    };
    // pairs a baseline-relative pixel position with normalized atlas coordinates
    struct Vertex { float x, y, u, v; };
    // returns a cached glyph or rasterizes and uploads the missing glyph ID
    Glyph const &ensure_glyph(uint32_t id);
    // cleans up raw handles during destruction or failed initialization
    void release();
    // borrows a font that must outlive the renderer and keep its configured size
    FontAsset &font;
    // converts grayscale atlas coverage into colored text
    TextProgram shader;
    // connects HarfBuzz shaping to the FreeType face and its pixel metrics
    hb_font_t *hb_font = nullptr;

    // owns the atlas texture and the vertex layout and buffer used to draw the line
    GLuint texture = 0, vao = 0, vbo = 0;
    // sets the width and height of the single atlas page in texels
    static constexpr int atlas_size = 1024;
    // tracks the next packing position and tallest glyph in the current atlas row
    int next_x = 1, next_y = 1, row_height = 0;
    // counts uploaded triangle vertices for the next draw call
    GLsizei vertex_count = 0;
    // stores the current HarfBuzz output count which may differ from character count
    size_t shaped_count = 0;
    // sums horizontal glyph advances for measuring the line
    float line_advance = 0;

    // remembers the prepared text so repeated requests can reuse its geometry
    std::string current_text;
    // distinguishes an uninitialized renderer from a prepared empty line
    bool has_line = false;
    // indexes cached bitmap metadata by glyph ID for this font and size
    std::map<uint32_t, Glyph> glyphs;
};
