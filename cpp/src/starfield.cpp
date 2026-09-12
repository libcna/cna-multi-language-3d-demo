#include "starfield.hpp"

#include <algorithm>
#include <cmath>

namespace starfield {
namespace {

constexpr float ArenaLimit = 10.0f;
constexpr float CellRadius = 0.9f;
constexpr float HazardRadius = 1.75f;
constexpr float ExtractionRadius = 1.4f;
constexpr float Pi = 3.14159265358979323846f;
constexpr float TurnSpeed = Pi;
constexpr float Cells[][2] = {{-6.0f, 0.0f}, {0.0f, -5.0f}, {6.0f, 0.0f}};

float clamp_axis(float value) {
    return std::clamp(value, -1.0f, 1.0f);
}

float distance_squared(float ax, float az, float bx, float bz) {
    const float dx = ax - bx;
    const float dz = az - bz;
    return dx * dx + dz * dz;
}

void step(Game& game, float turn, float forward, bool boost = true) {
    Input input;
    input.turn = turn;
    input.forward = forward;
    input.boost = boost;
    game.update(0.25f, input);
}

void repeat(Game& game, int count, float turn, float forward, bool boost = true) {
    for (int i = 0; i < count; ++i) {
        step(game, turn, forward, boost);
    }
}

void collect_all(Game& game) {
    repeat(game, 2, 1.0f, 0.0f);
    repeat(game, 3, 0.0f, 1.0f);
    repeat(game, 2, -1.0f, 0.0f);
    repeat(game, 3, 0.0f, 1.0f);
    repeat(game, 2, -1.0f, 0.0f);
    repeat(game, 3, 0.0f, 1.0f);
    repeat(game, 2, 1.0f, 0.0f);
    repeat(game, 2, 0.0f, 1.0f);
    repeat(game, 2, -1.0f, 0.0f);
    repeat(game, 3, 0.0f, 1.0f);
    repeat(game, 2, -1.0f, 0.0f);
    repeat(game, 5, 0.0f, 1.0f);
}

} // namespace

void Game::reset() {
    state_ = State::Title;
    collected_mask_ = 0;
    x_ = 0.0f;
    z_ = 0.0f;
    elapsed_ = 0.0f;
    score_ = 0.0f;
    heading_ = 0.0f;
    hazard_x_ = 0.0f;
    hazard_direction_ = 1.0f;
}

void Game::update(float seconds, Input input) {
    if (input.restart) {
        reset();
        return;
    }
    if (state_ == State::Won || state_ == State::Lost) {
        return;
    }
    if (state_ == State::Title) {
        state_ = State::Playing;
    }

    const float dt = std::clamp(seconds, 0.0f, 0.25f);
    const float speed = input.boost ? 7.0f : 4.0f;
    heading_ = std::remainder(heading_ + clamp_axis(input.turn) * TurnSpeed * dt, 2.0f * Pi);
    const float movement = clamp_axis(input.forward) * speed * dt;
    x_ = std::clamp(x_ + std::sin(heading_) * movement, -ArenaLimit, ArenaLimit);
    z_ = std::clamp(z_ - std::cos(heading_) * movement, -ArenaLimit, ArenaLimit);
    elapsed_ += dt;

    hazard_x_ += hazard_direction_ * 2.0f * dt;
    if (hazard_x_ >= 7.0f) {
        hazard_x_ = 7.0f;
        hazard_direction_ = -1.0f;
    } else if (hazard_x_ <= -7.0f) {
        hazard_x_ = -7.0f;
        hazard_direction_ = 1.0f;
    }

    for (std::uint32_t i = 0; i < 3; ++i) {
        const std::uint32_t bit = 1u << i;
        if ((collected_mask_ & bit) == 0 &&
            distance_squared(x_, z_, Cells[i][0], Cells[i][1]) <= CellRadius * CellRadius) {
            collected_mask_ |= bit;
            score_ += 100.0f;
        }
    }

    const float secondary_hazard_x = -hazard_x_;
    if (distance_squared(x_, z_, hazard_x_, 3.0f) <= HazardRadius * HazardRadius ||
        distance_squared(x_, z_, secondary_hazard_x, -3.5f) <= HazardRadius * HazardRadius ||
        elapsed_ >= 60.0f) {
        state_ = State::Lost;
        return;
    }
    if (collected_mask_ == 7 && distance_squared(x_, z_, 0.0f, -9.0f) <=
                                      ExtractionRadius * ExtractionRadius) {
        state_ = State::Won;
        score_ += 1000.0f;
    }
}

Snapshot Game::snapshot() const {
    Snapshot result;
    result.state = state_;
    result.collected_mask = collected_mask_;
    result.x = x_;
    result.z = z_;
    result.elapsed = elapsed_;
    result.score = score_;
    result.heading = heading_;
    result.hazard_x = hazard_x_;
    result.secondary_hazard_x = -hazard_x_;
    return result;
}

bool run_scenario(Game& game, std::string_view name) {
    if (name == "startup") {
        return true;
    }
    if (name == "collection" || name == "win") {
        collect_all(game);
        if (name == "win") {
            repeat(game, 2, -1.0f, 0.0f);
            repeat(game, 3, 0.0f, 1.0f);
            repeat(game, 2, -1.0f, 0.0f);
            repeat(game, 5, 0.0f, 1.0f);
        }
        return true;
    }
    if (name == "hazard") {
        // Reverse toward the primary hazard lane. The first boosted step puts
        // the player at Z=1.75 and the hazard at X=0.5, inside the collision radius.
        repeat(game, 2, 0.0f, -1.0f);
        return true;
    }
    if (name == "loss") {
        repeat(game, 240, 0.0f, 0.0f, false);
        return true;
    }
    if (name == "restart") {
        step(game, 1.0f, 1.0f, false);
        Input restart;
        restart.restart = true;
        game.update(0.0f, restart);
        return true;
    }
    return false;
}

} // namespace starfield
