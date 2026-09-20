#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include <string>

// owns the packaged font
struct FontAsset {
    explicit FontAsset(std::string const &path);
    ~FontAsset();
    FontAsset(FontAsset const &) = delete;
    FontAsset &operator=(FontAsset const &) = delete;
    FT_Face face() const { return face_; }
private:
    FT_Library library_ = nullptr;
    FT_Face face_ = nullptr;
};
