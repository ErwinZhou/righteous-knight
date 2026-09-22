#include "FontAsset.hpp"
#include <stdexcept>

FontAsset::FontAsset(std::string const &path) {
    if (FT_Init_FreeType(&library_))
        throw std::runtime_error("FreeType initialization failed for font: " + path);
    FT_Error error = FT_New_Face(library_, path.c_str(), 0, &face_);
    if (error) {
        FT_Done_FreeType(library_);
        library_ = nullptr;
        throw std::runtime_error("Cannot load font: " + path + " (FreeType error " + std::to_string(error) + ")");
    }
}

FontAsset::~FontAsset() {
    if (face_) FT_Done_Face(face_);
    if (library_) FT_Done_FreeType(library_);
}
