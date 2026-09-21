#include "PlayMode.hpp"
#include "GL.hpp"
#include "data_path.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
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
              << "\nStage 3 ready; the opening text wraps with the window. Escape exits.\n";
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

    int window_width, window_height;
    SDL_GetWindowSize(Mode::window, &window_width, &window_height);

    if (window_width <= 0 || window_height <= 0) return;
    glm::uvec2 logical_size(window_width, window_height);

    float density = float(drawable_size.y) / window_height;
    unsigned pixels = std::max(1u, unsigned(std::lround(24.0f * density)));

    if (!text_renderer || pixels != font_pixels || density != font_density) {
        // reset if the text render is missing or pixels/density changed
        text_renderer.reset();
        text_renderer = std::make_unique<TextRenderer>(font, pixels, density);
        font_pixels = pixels;
        font_density = density;
        layout_dirty = true;
    }

    if (layout_window != logical_size) layout_dirty = true;
    if (layout_dirty) {
        float width = std::max(1.0f, std::min(800.0f, window_width - 64.0f));
        text_renderer->set_text("A Rightesous Knight\n\n" + story.nodes.at(current_node).text, width);
        layout_window = logical_size;
        layout_dirty = false;
    }
    // actually draw the text or redener it
    text_renderer->draw(logical_size, {32.0f, 32.0f}, {0.95f, 0.88f, 0.72f, 1.0f});
}
