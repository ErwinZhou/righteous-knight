#pragma once
#include "TextLayout.hpp"
#include "TextProgram.hpp"
#include <glm/glm.hpp>
#include <map>
#include <vector>

// a renderer caches one font at one physical size and density
class TextRenderer {
public:
    TextRenderer(FontAsset &font, unsigned pixel_size, float density = 1.0f, int atlas_side = 1024);
    ~TextRenderer();
    TextRenderer(TextRenderer const &) = delete;
    TextRenderer &operator=(TextRenderer const &) = delete;
    void set_line(std::string const &text);
    void set_text(std::string const &text, float width);
    // single lines use a baseline origin and wrapped blocks use a top-left origin
    void draw(glm::uvec2 logical_viewport, glm::vec2 origin, glm::vec4 color);
    size_t cached_glyphs() const { return glyphs.size(); }
    size_t shaped_glyphs() const { return shaped_count; }
    size_t atlas_pages() const { return pages.size(); }
    size_t shape_calls() const { return layout.shape_calls(); }
    size_t uploads() const { return upload_count; }
    float advance() const { return block.width; }
    TextBlock const &text_block() const { return block; }
private:
    struct Glyph {
        int width = 0, height = 0, left = 0, top = 0, x = 0, y = 0;
        size_t page = 0;
    };
    struct Page { GLuint texture = 0; int x = 1, y = 1, row_height = 0; };
    struct Vertex { float x, y, u, v; };
    struct Batch { size_t page; GLint first; GLsizei count; };
    Glyph const &ensure_glyph(uint32_t id);
    void add_page();
    void upload(TextBlock const &prepared);
    void release();
    FontAsset &font;
    TextLayout layout;
    TextProgram shader;
    GLuint vao = 0, vbo = 0;
    int atlas_size;
    size_t shaped_count = 0, upload_count = 0;
    std::string current_text;
    float current_width = 0;
    bool has_text = false;
    TextBlock block;
    std::vector<Page> pages;
    std::vector<Batch> batches;
    std::map<uint32_t, Glyph> glyphs;
};
