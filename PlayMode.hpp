#pragma once
#include "Mode.hpp"
#include "Story.hpp"
#include "FontAsset.hpp"

struct PlayMode : Mode {
    PlayMode();
    ~PlayMode() override = default;

    bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
    void update(float elapsed) override;
    void draw(glm::uvec2 const &drawable_size) override;

    void enter_node(uint32_t id);
    void select_choice(uint32_t option);

    Story story;
    FontAsset font;
    uint32_t current_node = 0;
    uint32_t selected_choice = 0;
    float scroll_y = 0.0f;
    bool layout_dirty = true;
};
