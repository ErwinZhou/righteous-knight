#include "Story.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace {
struct Reader {
    std::vector<unsigned char> data;
    size_t offset = 0;
    void require(size_t size) const {
        if (size > data.size() - offset) throw std::runtime_error("Truncated story asset");
    }
    uint32_t integer() {
        require(4);
        uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i) value |= uint32_t(data[offset++]) << (i * 8);
        return value;
    }
    std::string string() {
        uint32_t size = integer();
        require(size);
        std::string value(data.begin() + offset, data.begin() + offset + size);
        offset += size;
        return value;
    }
};
}

Story Story::load(std::string const &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Cannot open story: " + filename);
    auto size = file.tellg();
    // generous for thousands of passages and bound allocations for damaged assets
    if (size < 0 || size > 64 * 1024 * 1024) throw std::runtime_error("Invalid story asset size");
    file.seekg(0);
    Reader reader;
    reader.data.assign(std::istreambuf_iterator<char>(file), {});
    if (reader.data.size() != static_cast<size_t>(size)) throw std::runtime_error("Failed to read story");
    reader.require(4);
    if (std::string(reader.data.begin(), reader.data.begin() + 4) != "RKST")
        throw std::runtime_error("Invalid story magic");
    reader.offset = 4;
    if (reader.integer() != 1) throw std::runtime_error("Unsupported story version");
    Story result;
    result.start = reader.integer();
    uint32_t count = reader.integer();
    if (count == 0 || result.start >= count) throw std::runtime_error("Invalid story start/count");
    result.title = reader.string();
    reader.require(size_t(count) * 12); // minimum bytes per node
    result.nodes.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        Node node;
        node.name = reader.string();
        node.text = reader.string();
        uint32_t choices = reader.integer();
        reader.require(size_t(choices) * 8);
        node.choices.reserve(choices);
        for (uint32_t j = 0; j < choices; ++j) {
            Choice choice;
            choice.label = reader.string();
            choice.target = reader.integer();
            if (choice.target >= count) throw std::runtime_error("Invalid story link target");
            node.choices.emplace_back(std::move(choice));
        }
        result.nodes.emplace_back(std::move(node));
    }
    if (reader.offset != reader.data.size()) throw std::runtime_error("Trailing story asset data");
    return result;
}

uint32_t Story::choose(uint32_t current, uint32_t option) const {
    return nodes.at(current).choices.at(option).target;
}
