#pragma once
#include "Mode.hpp"
#include "Story.hpp"
#include "FontAsset.hpp"
#include "TextRenderer.hpp"
#include <memory>

struct PlayMode : Mode {
    PlayMode();
    ~PlayMode() override = default;
    bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
    void update(float elapsed) override;
    void draw(glm::uvec2 const &drawable_size) override;
    void enter_node(uint32_t id);
    void select_choice(uint32_t option);
    void confirm_selection();
    void request_restart();
    void cancel_restart();
    enum class State { Reading, ConfirmRestart };
    State state = State::Reading;
    float saved_scroll = 0;
    uint32_t saved_choice = 0;

    struct Rect {
        float x = 0, y = 0, width = 0, height = 0;
        bool contains(glm::vec2 p) const {
            return p.x >= x && p.y >= y && p.x < x + width && p.y < y + height;
        }
    };
    int hit_choice(glm::vec2 mouse) const;
    void scroll_to(float position);
    void reveal_choice();
    void rebuild_layout(glm::uvec2 logical_size);

    Story story;
    FontAsset font;
    std::unique_ptr<TextRenderer> text_renderer, footer_renderer;
    uint32_t current_node = 0, selected_choice = 0;
    float scroll_y = 0, content_height = 0, max_scroll = 0;
    bool layout_dirty = true;
    glm::uvec2 layout_window = {0, 0};
    float font_density = 0;
    unsigned font_pixels = 0;
    Rect reading_viewport;
    std::vector<Rect> choice_rects;
    std::vector<glm::vec2> block_origins;
    glm::vec2 mouse_position = {-1, -1};
    int hovered_choice = -1;
    float footer_y = 0;
};
