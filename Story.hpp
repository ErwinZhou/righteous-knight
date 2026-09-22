#pragma once
#include <cstdint>
#include <string>
#include <vector>

// compiled story data
struct Story {
    struct Choice {
        std::string label;
        uint32_t target;
    };
    struct Node {
        std::string name;
        std::string text; // text in UTF-8 with original line breaks
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
