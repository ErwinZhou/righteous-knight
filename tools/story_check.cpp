#include "../Story.hpp"
#include <iostream>
#include <set>
#include <stdexcept>

// Independent of SDL/OpenGL: validates loading and traverses every reachable edge.
int main(int argc, char **argv) {
    try {
        if (argc != 2) throw std::runtime_error("Usage: story-check path/to/story.bin");
        Story story = Story::load(argv[1]);
        std::set<uint32_t> visited;
        std::vector<uint32_t> pending{story.start};
        size_t choices = 0;
        while (!pending.empty()) {
            uint32_t id = pending.back();
            pending.pop_back();
            if (!visited.insert(id).second) continue;
            for (uint32_t i = 0; i < story.nodes.at(id).choices.size(); ++i) {
                pending.push_back(story.choose(id, i));
                ++choices;
            }
        }
        std::cout << story.title << ": " << story.nodes.size() << " nodes, "
                  << visited.size() << " reachable, " << choices << " reachable choices\n";
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
