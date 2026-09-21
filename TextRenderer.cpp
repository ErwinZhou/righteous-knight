#include "TextRenderer.hpp"
#include <hb-ft.h>
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>

// https://harfbuzz.github.io/shaping-and-shape-plans.html
// https://freetype.org/freetype2/docs/tutorial/step1.html
TextRenderer::TextRenderer(FontAsset &font_, unsigned pixel_size) : font(font_) {
    try {
        if (!pixel_size || FT_Set_Pixel_Sizes(font.face(), 0, pixel_size))
            throw std::runtime_error("Cannot set font pixel size");
        hb_font = hb_ft_font_create_referenced(font.face());
        if (!hb_font) throw std::runtime_error("Cannot create HarfBuzz font");
        hb_ft_font_set_load_flags(hb_font, FT_LOAD_DEFAULT);
        // hb-ft uses the face size in 26.6 pixel units
        hb_ft_font_changed(hb_font);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        std::vector<unsigned char> empty(atlas_size * atlas_size, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlas_size, atlas_size, 0,
                     GL_RED, GL_UNSIGNED_BYTE, empty.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, u)));
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    } catch (...) {
        release();
        throw;
    }
}

void TextRenderer::release() {
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (texture) glDeleteTextures(1, &texture);
    if (hb_font) hb_font_destroy(hb_font);
}

TextRenderer::~TextRenderer() { release(); }

TextRenderer::Glyph const &TextRenderer::ensure_glyph(uint32_t id) {
    auto found = glyphs.find(id);
    if (found != glyphs.end()) return found->second;
    if (FT_Load_Glyph(font.face(), id, FT_LOAD_DEFAULT) ||
        FT_Render_Glyph(font.face()->glyph, FT_RENDER_MODE_NORMAL))
        throw std::runtime_error("Cannot rasterize glyph " + std::to_string(id));
    auto const &slot = *font.face()->glyph;
    auto const &bitmap = slot.bitmap;
    Glyph glyph;
    glyph.width = int(bitmap.width);
    glyph.height = int(bitmap.rows);
    glyph.left = slot.bitmap_left;
    glyph.top = slot.bitmap_top;
    if (glyph.width && glyph.height) {
        if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
            throw std::runtime_error("Expected a grayscale glyph bitmap");
        if (glyph.width + 2 > atlas_size || glyph.height + 2 > atlas_size)
            throw std::runtime_error("Glyph exceeds atlas size");
        if (next_x + glyph.width + 1 > atlas_size) {
            next_x = 1;
            next_y += row_height + 2;
            row_height = 0;
        }
        if (next_y + glyph.height + 1 > atlas_size)
            throw std::runtime_error("Stage 2 glyph atlas is full");
        glyph.x = next_x;
        glyph.y = next_y;
        std::vector<unsigned char> pixels(size_t(glyph.width) * glyph.height);
        for (int y = 0; y < glyph.height; ++y) {
            auto row = bitmap.buffer + ptrdiff_t(y) * bitmap.pitch;
            std::copy(row, row + glyph.width, pixels.begin() + size_t(y) * glyph.width);
        }
        GLint alignment;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, glyph.x, glyph.y, glyph.width, glyph.height,
                        GL_RED, GL_UNSIGNED_BYTE, pixels.data());
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glBindTexture(GL_TEXTURE_2D, 0);
        next_x += glyph.width + 2;
        row_height = std::max(row_height, glyph.height);
    }
    return glyphs.emplace(id, glyph).first->second;
}

void TextRenderer::set_line(std::string const &text) {
    if (has_line && current_text == text) return;
    if (text.size() > size_t(std::numeric_limits<int>::max()) || text.find_first_of("\r\n") != std::string::npos)
        throw std::runtime_error("Expected a single line of text");
    std::unique_ptr<hb_buffer_t, decltype(&hb_buffer_destroy)> buffer(hb_buffer_create(), hb_buffer_destroy);
    hb_buffer_add_utf8(buffer.get(), text.data(), int(text.size()), 0, int(text.size()));
    hb_buffer_guess_segment_properties(buffer.get());
    hb_shape(hb_font, buffer.get(), nullptr, 0);
    if (!hb_buffer_allocation_successful(buffer.get())) throw std::runtime_error("HarfBuzz allocation failed");
    unsigned count;
    auto infos = hb_buffer_get_glyph_infos(buffer.get(), &count);
    auto positions = hb_buffer_get_glyph_positions(buffer.get(), nullptr);
    std::vector<Vertex> vertices;
    float pen_x = 0, pen_y = 0;
    for (unsigned i = 0; i < count; ++i) {
        auto const &g = ensure_glyph(infos[i].codepoint);
        float x = pen_x + positions[i].x_offset / 64.0f + g.left;
        float y = -pen_y - positions[i].y_offset / 64.0f - g.top;
        if (g.width && g.height) {
            float right = x + g.width, bottom = y + g.height;
            float u = float(g.x) / atlas_size, v = float(g.y) / atlas_size;
            float ur = float(g.x + g.width) / atlas_size, vb = float(g.y + g.height) / atlas_size;
            vertices.insert(vertices.end(), {{x,y,u,v}, {right,y,ur,v}, {x,bottom,u,vb},
                                            {x,bottom,u,vb}, {right,y,ur,v}, {right,bottom,ur,vb}});
        }
        pen_x += positions[i].x_advance / 64.0f;
        pen_y += positions[i].y_advance / 64.0f;
    }
    if (vertices.size() > size_t(std::numeric_limits<GLsizei>::max()))
        throw std::runtime_error("Text line exceeds draw limit");
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    vertex_count = GLsizei(vertices.size());
    shaped_count = count;
    line_advance = pen_x;
    current_text = text;
    has_line = true;
}

void TextRenderer::draw(glm::uvec2 viewport, glm::vec2 baseline, glm::vec4 color) {
    if (!viewport.x || !viewport.y || !vertex_count) return;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(shader.program);
    glUniform2f(shader.viewport, float(viewport.x), float(viewport.y));
    glUniform2f(shader.origin, baseline.x, baseline.y);
    glUniform4f(shader.color, color.r, color.g, color.b, color.a);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glDisable(GL_BLEND);
}
