#include "TextLayout.hpp"
#include <hb-ft.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {
void validate_utf8(std::string const &s) {
    for (size_t i = 0; i < s.size();) {
        auto c = static_cast<unsigned char>(s[i++]);
        if (c < 128) continue;
        unsigned count, cp, minimum;
        if (c >= 0xc2 && c <= 0xdf) { count = 1; cp = c & 31; minimum = 128; }
        else if (c >= 0xe0 && c <= 0xef) { count = 2; cp = c & 15; minimum = 2048; }
        else if (c >= 0xf0 && c <= 0xf4) { count = 3; cp = c & 7; minimum = 65536; }
        else throw std::runtime_error("Invalid UTF-8 text");
        if (count > s.size() - i) throw std::runtime_error("Truncated UTF-8 text");
        while (count--) {
            auto next = static_cast<unsigned char>(s[i++]);
            if ((next & 0xc0) != 0x80) throw std::runtime_error("Invalid UTF-8 continuation");
            cp = (cp << 6) | (next & 63);
        }
        if (cp < minimum || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
            throw std::runtime_error("Invalid Unicode scalar");
    }
}
}

TextLayout::TextLayout(FontAsset &font_, unsigned pixel_size_, float density_)
    : font(font_), pixel_size(pixel_size_), density(density_) {
    if (!pixel_size || !std::isfinite(density) || density <= 0)
        throw std::runtime_error("Invalid font size or pixel density");
    activate_size();
    hb_font = hb_ft_font_create_referenced(font.face());
    if (!hb_font) throw std::runtime_error("Cannot create HarfBuzz font");
    hb_ft_font_set_load_flags(hb_font, FT_LOAD_DEFAULT);
    hb_ft_font_changed(hb_font);
    height = font.face()->size->metrics.height / (64.0f * density);
    ascent = font.face()->size->metrics.ascender / (64.0f * density);
    if (height <= 0) height = pixel_size / density;
}

TextLayout::~TextLayout() { if (hb_font) hb_font_destroy(hb_font); }

void TextLayout::activate_size() {
    if (FT_Set_Pixel_Sizes(font.face(), 0, pixel_size))
        throw std::runtime_error("Cannot set font pixel size");
    if (hb_font) hb_ft_font_changed(hb_font);
}

TextLine TextLayout::shape(std::string const &text) {
    validate_utf8(text);
    if (text.size() > size_t(std::numeric_limits<int>::max()) || text.find_first_of("\r\n") != std::string::npos)
        throw std::runtime_error("Expected a single UTF-8 line");
    activate_size();
    std::unique_ptr<hb_buffer_t, decltype(&hb_buffer_destroy)> buffer(hb_buffer_create(), hb_buffer_destroy);
    std::string input = text;
    std::replace(input.begin(), input.end(), '\t', ' ');
    hb_buffer_add_utf8(buffer.get(), input.data(), int(input.size()), 0, int(input.size()));
    hb_buffer_set_cluster_level(buffer.get(), HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES);
    hb_buffer_guess_segment_properties(buffer.get());
    hb_shape(hb_font, buffer.get(), nullptr, 0);
    ++calls;
    if (!hb_buffer_allocation_successful(buffer.get())) throw std::runtime_error("HarfBuzz allocation failed");
    unsigned count;
    auto info = hb_buffer_get_glyph_infos(buffer.get(), &count);
    auto positions = hb_buffer_get_glyph_positions(buffer.get(), nullptr);
    TextLine line;
    line.end = text.size();
    // convert hb-ft metrics from 1/64 physical pixels to logical pixels
    float unit = 64.0f * density;
    for (unsigned i = 0; i < count; ++i) {
        auto const &p = positions[i];
        line.glyphs.push_back({info[i].codepoint, info[i].cluster,
            p.x_advance / unit, p.y_advance / unit, p.x_offset / unit, p.y_offset / unit});
        line.width += p.x_advance / unit;
    }
    return line;
}

TextBlock TextLayout::wrap(std::string const &text, float width) {
    validate_utf8(text);
    if (text.size() > size_t(std::numeric_limits<int>::max())) throw std::runtime_error("Text block too large");
    if (!std::isfinite(width) || width <= 0) throw std::runtime_error("Invalid text width");
    TextBlock block;
    auto append = [&](size_t begin, size_t end) { // reshape the final line and keep its source offsets
        auto line = shape(text.substr(begin, end - begin));
        line.begin = begin;
        line.end = end;
        line.baseline = ascent + block.lines.size() * height;
        for (auto &g : line.glyphs) g.cluster += uint32_t(begin);
        block.width = std::max(block.width, line.width);
        block.lines.emplace_back(std::move(line));
    };
    for (size_t paragraph = 0;;) {
        size_t newline = text.find('\n', paragraph);
        size_t end = newline == std::string::npos ? text.size() : newline;
        if (end > paragraph && text[end - 1] == '\r') --end;
        size_t begin = paragraph;
        if (begin == end) append(begin, end);
        while (begin < end) {
            size_t chosen = begin, candidate = begin;
            // try longer lines at whitespace boundaries until the next one is too wide
            while (candidate < end) {
                size_t boundary = text.find_first_of(" \t", candidate);
                boundary = boundary == std::string::npos || boundary >= end ? end : boundary + 1;
                auto measured = shape(text.substr(begin, boundary - begin));
                if (measured.width > width) break;
                chosen = boundary;
                candidate = boundary;
            }
            if (chosen == begin) { // split a long word at cluster boundaries and always consume at least one
                size_t word_end = text.find_first_of(" \t", begin);
                if (word_end == std::string::npos || word_end >= end) word_end = end;
                else ++word_end;
                auto word = shape(text.substr(begin, word_end - begin));
                std::vector<size_t> boundaries;
                for (auto const &g : word.glyphs)
                    if (g.cluster) boundaries.push_back(begin + g.cluster);
                boundaries.push_back(word_end);
                std::sort(boundaries.begin(), boundaries.end());
                boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());
                chosen = boundaries.front();
                for (auto boundary : boundaries) {
                    if (shape(text.substr(begin, boundary - begin)).width > width) break;
                    chosen = boundary;
                }
            }
            append(begin, chosen);
            begin = chosen;
        }
        if (newline == std::string::npos) break;
        paragraph = newline + 1;
    }
    block.height = block.lines.size() * height;
    return block;
}
