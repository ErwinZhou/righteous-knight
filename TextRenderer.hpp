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
    TextRenderer(FontAsset &font, unsigned pixel_size);
    ~TextRenderer();
    TextRenderer(TextRenderer const &) = delete;
    TextRenderer &operator=(TextRenderer const &) = delete;
    void set_line(std::string const &text);
    void draw(glm::uvec2 viewport, glm::vec2 baseline, glm::vec4 color);
    size_t cached_glyphs() const { return glyphs.size(); }
    size_t shaped_glyphs() const { return shaped_count; }
    float advance() const { return line_advance; }
private:
    struct Glyph {
        int width = 0, height = 0, left = 0, top = 0;
        int x = 0, y = 0;
    };
    struct Vertex { float x, y, u, v; };
    Glyph const &ensure_glyph(uint32_t id);
    void release();
    FontAsset &font;
    TextProgram shader;
    hb_font_t *hb_font = nullptr;
    GLuint texture = 0, vao = 0, vbo = 0;
    static constexpr int atlas_size = 1024;
    int next_x = 1, next_y = 1, row_height = 0;
    GLsizei vertex_count = 0;
    size_t shaped_count = 0;
    float line_advance = 0;
    std::string current_text;
    bool has_line = false;
    std::map<uint32_t, Glyph> glyphs;
};
