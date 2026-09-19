#pragma once
#include <cstdint>
#include <string>
#include <vector>

// compiled narratives, leave rendering and input to the caller
struct Story {
    struct Choice {
        std::string label;
        uint32_t target;
    };
    struct Node {
        std::string name;
        std::string text; // UTF-8 preserving paragraph and line breaks
        std::vector<Choice> choices;
    };
    std::string title;
    uint32_t start = 0;
    std::vector<Node> nodes;

    static Story load(std::string const &filename);
    // returns the next node ID
    // throws for invalid current node and option index
    uint32_t choose(uint32_t current, uint32_t option) const;
};
