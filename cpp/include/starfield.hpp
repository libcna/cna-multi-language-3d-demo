#pragma once

#include <cstdint>
#include <string_view>

namespace starfield {

enum class State : std::uint32_t { Title = 0, Playing = 1, Won = 2, Lost = 3 };

struct Input {
    float turn = 0.0f;
    float forward = 0.0f;
    bool boost = false;
    bool restart = false;
};

struct Snapshot {
    State state = State::Title;
    std::uint32_t collected_mask = 0;
    float x = 0.0f;
    float z = 0.0f;
    float elapsed = 0.0f;
    float score = 0.0f;
    float heading = 0.0f;
    float hazard_x = 0.0f;
    float secondary_hazard_x = 0.0f;
};

class Game {
public:
    Game() = default;

    void reset();
    void update(float seconds, Input input);
    [[nodiscard]] Snapshot snapshot() const;

private:
    State state_ = State::Title;
    std::uint32_t collected_mask_ = 0;
    float x_ = 0.0f;
    float z_ = 0.0f;
    float elapsed_ = 0.0f;
    float score_ = 0.0f;
    float heading_ = 0.0f;
    float hazard_x_ = 0.0f;
    float hazard_direction_ = 1.0f;
};

// Runs one of the deterministic verification scenarios from the shared spec.
// Returns false without modifying the game when the name is unknown.
bool run_scenario(Game& game, std::string_view name);

class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void draw(const Snapshot& snapshot) = 0;
};

} // namespace starfield
