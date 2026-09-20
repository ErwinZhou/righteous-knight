#include "PlayMode.hpp"
#include "GL.hpp"
#include "data_path.hpp"
#include <iostream>
#include <stdexcept>

namespace {
Story load_story() {
    auto path = data_path("story.bin");
    try {
        return Story::load(path);
    } catch (std::exception const &e) {
        throw std::runtime_error("Cannot load story '" + path + "': " + e.what());
    }
}
}

PlayMode::PlayMode() : story(load_story()), font(data_path("fonts/NotoSerif.ttf")) {
    enter_node(story.start);
    std::cout << "Loaded font: " << data_path("fonts/NotoSerif.ttf")
              << "\nStage 1 ready; text rendering is not implemented yet. Escape exits.\n";
}

void PlayMode::enter_node(uint32_t id) {
    auto const &node = story.nodes.at(id); // validate before changing state
    current_node = id;
    selected_choice = 0;
    scroll_y = 0.0f;
    layout_dirty = true;
    std::cout << "Story node " << id << ": " << node.name
              << " (" << node.choices.size() << " choices)\n";
}

void PlayMode::select_choice(uint32_t option) {
    enter_node(story.choose(current_node, option));
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &) {
    if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_ESCAPE) {
        Mode::set_current(nullptr);
        return true;
    }
    return false; // leave quit and screenshots to main
}

void PlayMode::update(float) {
    // choices advance the story
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
    if (drawable_size.x == 0 || drawable_size.y == 0) return;
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.035f, 0.03f, 0.025f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    // text rendering comes in stage 2
}
