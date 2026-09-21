#include "TextRenderer.hpp"
#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>

// https://freetype.org/freetype2/docs/tutorial/step1.html
TextRenderer::TextRenderer(FontAsset &font_, unsigned pixel_size, float density, int atlas_side)
    : font(font_), layout(font_, pixel_size, density), atlas_size(atlas_side) {
    try {
        GLint maximum;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum);
        if (atlas_size < 4 || atlas_size > maximum) throw std::runtime_error("Invalid atlas size");
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
    } catch (...) { release(); throw; }
}

void TextRenderer::release() {
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    for (auto const &page : pages) glDeleteTextures(1, &page.texture);
}

TextRenderer::~TextRenderer() { release(); }

void TextRenderer::add_page() {
    std::vector<unsigned char> empty(size_t(atlas_size) * atlas_size, 0);
    pages.emplace_back();
    glGenTextures(1, &pages.back().texture);
    glBindTexture(GL_TEXTURE_2D, pages.back().texture);
    GLint alignment;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlas_size, atlas_size, 0, GL_RED, GL_UNSIGNED_BYTE, empty.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

TextRenderer::Glyph const &TextRenderer::ensure_glyph(uint32_t id) { // rasterize only glyphs missing from the cache
    auto found = glyphs.find(id);
    if (found != glyphs.end()) return found->second;
    layout.activate_size();
    if (FT_Load_Glyph(font.face(), id, FT_LOAD_DEFAULT) || FT_Render_Glyph(font.face()->glyph, FT_RENDER_MODE_NORMAL))
        throw std::runtime_error("Cannot rasterize glyph " + std::to_string(id));
    auto const &slot = *font.face()->glyph;
    auto const &bitmap = slot.bitmap;
    Glyph glyph;
    glyph.width = int(bitmap.width);
    glyph.height = int(bitmap.rows);
    glyph.left = slot.bitmap_left;
    glyph.top = slot.bitmap_top;
    if (glyph.width && glyph.height) {
        if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) throw std::runtime_error("Expected a grayscale glyph bitmap");
        if (glyph.width + 2 > atlas_size || glyph.height + 2 > atlas_size)
            throw std::runtime_error("Glyph exceeds atlas size");
        // fill rows and append pages without moving cached glyphs
        if (pages.empty()) add_page();
        if (pages.back().x + glyph.width + 1 > atlas_size) {
            pages.back().x = 1;
            pages.back().y += pages.back().row_height + 2;
            pages.back().row_height = 0;
        }
        if (pages.back().y + glyph.height + 1 > atlas_size) add_page();
        auto &page = pages.back();
        glyph.x = page.x;
        glyph.y = page.y;
        glyph.page = pages.size() - 1;
        std::vector<unsigned char> pixels(size_t(glyph.width) * glyph.height);
        for (int y = 0; y < glyph.height; ++y) {
            auto row = bitmap.buffer + ptrdiff_t(y) * bitmap.pitch;
            std::copy(row, row + glyph.width, pixels.begin() + size_t(y) * glyph.width);
        }
        GLint alignment;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, page.texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, glyph.x, glyph.y, glyph.width, glyph.height,
                       GL_RED, GL_UNSIGNED_BYTE, pixels.data());
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glBindTexture(GL_TEXTURE_2D, 0);
        ++upload_count;
        page.x += glyph.width + 2;
        page.row_height = std::max(page.row_height, glyph.height);
    }
    return glyphs.emplace(id, glyph).first->second;
}

void TextRenderer::upload(TextBlock const &prepared) {
    std::vector<Vertex> vertices;
    std::vector<Batch> pending;
    size_t count = 0;
    float density = layout.pixel_density();
    // turn glyph positions into logical-pixel quads grouped by atlas page
    for (auto const &line : prepared.lines) {
        float pen_x = 0, pen_y = 0;
        count += line.glyphs.size();
        for (auto const &position : line.glyphs) {
            auto const &g = ensure_glyph(position.id);
            float x = pen_x + position.x_offset + g.left / density;
            float y = line.baseline - pen_y - position.y_offset - g.top / density;
            if (g.width && g.height) {
                if (vertices.size() > size_t(std::numeric_limits<GLsizei>::max()) - 6)
                    throw std::runtime_error("Text block exceeds draw limit");
                if (pending.empty() || pending.back().page != g.page)
                    pending.push_back({g.page, GLint(vertices.size()), 0});
                pending.back().count += 6;
                float right = x + g.width / density, bottom = y + g.height / density;
                float u = float(g.x) / atlas_size, v = float(g.y) / atlas_size;
                float ur = float(g.x + g.width) / atlas_size, vb = float(g.y + g.height) / atlas_size;
                vertices.insert(vertices.end(), {{x,y,u,v}, {right,y,ur,v}, {x,bottom,u,vb},
                                                {x,bottom,u,vb}, {right,y,ur,v}, {right,bottom,ur,vb}});
            }
            pen_x += position.x_advance;
            pen_y += position.y_advance;
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    ++upload_count;
    batches = std::move(pending);
    shaped_count = count;
}

void TextRenderer::set_line(std::string const &text) {
    if (has_text && current_width == 0 && current_text == text) return;
    TextBlock prepared;
    prepared.lines.push_back(layout.shape(text));
    prepared.width = prepared.lines.front().width;
    prepared.height = layout.line_height();
    upload(prepared);
    block = std::move(prepared);
    current_text = text;
    current_width = 0;
    has_text = true;
}

void TextRenderer::set_text(std::string const &text, float width) {
    if (has_text && current_width == width && current_text == text) return;
    auto prepared = layout.wrap(text, width);
    upload(prepared);
    block = std::move(prepared);
    current_text = text;
    current_width = width;
    has_text = true;
}

void TextRenderer::draw(glm::uvec2 viewport, glm::vec2 origin, glm::vec4 color) {
    if (!viewport.x || !viewport.y || batches.empty()) return;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(shader.program);
    glUniform2f(shader.viewport, float(viewport.x), float(viewport.y));
    glUniform2f(shader.origin, origin.x, origin.y);
    glUniform4f(shader.color, color.r, color.g, color.b, color.a);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    // draw cached geometry in text order using each batch's atlas page
    for (auto const &batch : batches) {
        glBindTexture(GL_TEXTURE_2D, pages[batch.page].texture);
        glDrawArrays(GL_TRIANGLES, batch.first, batch.count);
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glDisable(GL_BLEND);
}
