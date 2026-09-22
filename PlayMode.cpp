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
    try { return Story::load(path); }
    catch (std::exception const &e) {
        throw std::runtime_error("Cannot load story '" + path + "': " + e.what());
    }
}

void scissor(PlayMode::Rect r, glm::uvec2 logical, glm::uvec2 physical) {
    float sx = float(physical.x) / logical.x, sy = float(physical.y) / logical.y;
    int left = int(std::ceil(r.x * sx)), right = int(std::floor((r.x + r.width) * sx));
    int top = int(std::ceil(r.y * sy)), bottom = int(std::floor((r.y + r.height) * sy));
    glScissor(left, int(physical.y) - bottom, std::max(0, right - left), std::max(0, bottom - top));
}
} // namespace

PlayMode::PlayMode() : story(load_story()), font(data_path("fonts/NotoSerif.ttf")) {
    enter_node(story.start);
    std::cout << "Ready; Enter or click chooses, R restarts, Escape exits\n";
}

void PlayMode::enter_node(uint32_t id) {
    auto const &node = story.nodes.at(id);
    state = State::Reading;
    current_node = id;
    selected_choice = 0;
    scroll_y = 0;
    hovered_choice = -1;
    mouse_position = {-1, -1};
    choice_rects.clear();
    block_origins.clear();
    layout_dirty = true;
    std::cout << "Story node " << id << ": " << node.name << " (" << node.choices.size() << " choices)\n";
}

void PlayMode::select_choice(uint32_t option) { enter_node(story.choose(current_node, option)); }

void PlayMode::request_restart() {
    saved_scroll = scroll_y;
    saved_choice = selected_choice;
    state = State::ConfirmRestart;
    selected_choice = 0;
    scroll_y = 0;
    hovered_choice = -1;
    choice_rects.clear();
    layout_dirty = true;
}

void PlayMode::cancel_restart() {
    state = State::Reading;
    scroll_y = saved_scroll;
    selected_choice = saved_choice;
    hovered_choice = -1;
    choice_rects.clear();
    layout_dirty = true;
}

void PlayMode::confirm_selection() {
    // one confirmation changes one screen and invalidates its old hit regions
    if (layout_dirty) return;
    if (state == State::ConfirmRestart) {
        if (selected_choice == 0) enter_node(story.start);
        else cancel_restart();
    } else if (selected_choice < story.nodes.at(current_node).choices.size()) {
        select_choice(selected_choice);
    }
}

int PlayMode::hit_choice(glm::vec2 mouse) const {
    if (layout_dirty || !reading_viewport.contains(mouse)) return -1;
    glm::vec2 content = mouse - glm::vec2(reading_viewport.x, reading_viewport.y) + glm::vec2(0, scroll_y);
    for (size_t i = 0; i < choice_rects.size(); ++i)
        if (choice_rects[i].contains(content)) return int(i);
    return -1;
}

void PlayMode::scroll_to(float position) {
    scroll_y = std::clamp(position, 0.0f, max_scroll);
    hovered_choice = hit_choice(mouse_position);
}

void PlayMode::reveal_choice() {
    if (selected_choice >= choice_rects.size()) return;
    auto const &r = choice_rects[selected_choice];
    if (r.height > reading_viewport.height || r.y < scroll_y) scroll_to(r.y);
    else if (r.y + r.height > scroll_y + reading_viewport.height)
        scroll_to(r.y + r.height - reading_viewport.height);
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
    if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.repeat) return true;
    if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_ESCAPE) {
        if (state == State::ConfirmRestart) cancel_restart();
        else Mode::set_current(nullptr);
        return true;
    }
    if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_R) {
        if (state == State::Reading && !layout_dirty) request_restart();
        return true;
    }
    if (window_size != layout_window || evt.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) layout_dirty = true;
    if (evt.type == SDL_EVENT_MOUSE_MOTION) mouse_position = {evt.motion.x, evt.motion.y};
    if (evt.type == SDL_EVENT_WINDOW_MOUSE_LEAVE) mouse_position = {-1, -1};
    if (layout_dirty) return false;
    if (evt.type == SDL_EVENT_MOUSE_MOTION || evt.type == SDL_EVENT_WINDOW_MOUSE_LEAVE) {
        hovered_choice = hit_choice(mouse_position);
        return true;
    }
    if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_LEFT) {
        mouse_position = {evt.button.x, evt.button.y};
        hovered_choice = hit_choice(mouse_position);
        if (evt.button.clicks > 1) return true;
        if (hovered_choice >= 0) {
            selected_choice = uint32_t(hovered_choice);
            confirm_selection();
        }
        return true;
    }
    if (evt.type == SDL_EVENT_MOUSE_WHEEL) {
        float direction = evt.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
        scroll_to(scroll_y - evt.wheel.y * direction * 48.0f);
        return true;
    }
    if (evt.type == SDL_EVENT_KEY_DOWN) {
        switch (evt.key.key) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER: confirm_selection(); return true;
        case SDLK_HOME: scroll_to(0); return true;
        case SDLK_END: scroll_to(max_scroll); return true;
        case SDLK_PAGEUP: scroll_to(scroll_y - reading_viewport.height * 0.85f); return true;
        case SDLK_PAGEDOWN: scroll_to(scroll_y + reading_viewport.height * 0.85f); return true;
        case SDLK_UP:
        case SDLK_DOWN:
            if (!choice_rects.empty()) {
                if (evt.key.key == SDLK_UP && selected_choice > 0) --selected_choice;
                if (evt.key.key == SDLK_DOWN && selected_choice + 1 < choice_rects.size()) ++selected_choice;
                reveal_choice();
            }
            return true;
        default: break;
        }
    }
    return false;
}

void PlayMode::update(float) {}

void PlayMode::rebuild_layout(glm::uvec2 logical_size) {
    float width = std::max(1.0f, std::min(800.0f, float(logical_size.x) - 48.0f));
    auto const &node = story.nodes.at(current_node);
    std::string controls = state == State::ConfirmRestart
        ? "Enter/click: confirm   Up/Down: select   Esc: cancel"
        : node.choices.empty()
            ? "End of story   R: restart   Esc: exit   Wheel/Page: scroll"
            : "Enter/click: choose   Up/Down: focus   Wheel/Page: scroll   Home/End: top/bottom   R: restart   Esc: exit";
    footer_renderer->set_text(controls, width);
    footer_y = std::max(0.0f, float(logical_size.y) - footer_renderer->text_block().height - 16.0f);
    reading_viewport = {24, 24, width, std::max(1.0f, footer_y - 40.0f)};
    std::vector<std::string> text;
    if (state == State::ConfirmRestart) {
        text = {"Restart story?", "Your current progress will be lost.", "Restart", "Keep reading"};
    } else {
        text = {"A Rightesous Knight", node.text};
        for (size_t i = 0; i < node.choices.size(); ++i)
            text.push_back(std::to_string(i + 1) + ". " + node.choices[i].label);
    }
    text_renderer->set_blocks(text, std::max(1.0f, width - 24));
    block_origins.clear();
    choice_rects.clear();
    float y = 0;
    // keep layout and hit regions in the same unscrolled content coordinates
    for (size_t i = 0; i < text.size(); ++i) {
        float height = text_renderer->text_block(i).height;
        if (i >= 2) {
            choice_rects.push_back({0, y, width, height + 24});
            block_origins.push_back({12, y + 12});
            y += height + 36;
        } else {
            block_origins.push_back({12, y});
            y += height + 24;
        }
    }
    content_height = y;
    max_scroll = std::max(0.0f, content_height - reading_viewport.height);
    layout_window = logical_size;
    layout_dirty = false;
    scroll_to(scroll_y);
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
    if (!drawable_size.x || !drawable_size.y) return;
    int w, h;
    SDL_GetWindowSize(Mode::window, &w, &h);
    if (w <= 0 || h <= 0) return;
    glm::uvec2 logical_size(w, h);
    float density = float(drawable_size.y) / h;
    unsigned pixels = std::max(1u, unsigned(std::lround(24.0f * density)));
    if (!text_renderer || pixels != font_pixels || density != font_density) {
        text_renderer.reset();
        footer_renderer.reset();
        text_renderer = std::make_unique<TextRenderer>(font, pixels, density);
        footer_renderer = std::make_unique<TextRenderer>(font, std::max(1u, unsigned(std::lround(14 * density))), density);
        font_pixels = pixels;
        font_density = density;
        layout_dirty = true;
    }
    if (layout_window != logical_size) layout_dirty = true;
    if (layout_dirty) rebuild_layout(logical_size);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.035f, 0.03f, 0.025f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    // convert the clipped logical rectangle to OpenGL's bottom-left physical coordinates
    for (size_t i = 0; i < block_origins.size(); ++i) {
        if (i >= 2 && (i - 2 == selected_choice || int(i - 2) == hovered_choice)) {
            Rect r = choice_rects[i - 2];
            float top = std::max(0.0f, r.y - scroll_y);
            float bottom = std::min(reading_viewport.height, r.y + r.height - scroll_y);
            if (bottom > top) {
                scissor({reading_viewport.x, reading_viewport.y + top, r.width, bottom - top}, logical_size, drawable_size);
                glClearColor(0.12f, 0.09f, 0.055f, 1);
                glClear(GL_COLOR_BUFFER_BIT);
            }
        }
        scissor(reading_viewport, logical_size, drawable_size);
        glm::vec2 origin = glm::vec2(reading_viewport.x, reading_viewport.y - scroll_y) + block_origins[i];
        float height = text_renderer->text_block(i).height;
        if (origin.y + height < reading_viewport.y || origin.y > reading_viewport.y + reading_viewport.height) continue;
        text_renderer->draw(logical_size, origin, {0.95f, 0.88f, 0.72f, 1}, i);
    }
    glDisable(GL_SCISSOR_TEST);
    footer_renderer->draw(logical_size, {24, footer_y}, {0.65f, 0.61f, 0.53f, 1});
}
