#include "starfield.hpp"

#include <algorithm>
#include <cmath>

namespace starfield {
namespace {

constexpr float ArenaLimit = 10.0f;
constexpr float CellRadius = 0.9f;
constexpr float HazardRadius = 1.75f;
constexpr float ExtractionRadius = 1.4f;
constexpr float Cells[][2] = {{-6.0f, 0.0f}, {0.0f, -5.0f}, {6.0f, 0.0f}};

float clamp_axis(float value) {
    return std::clamp(value, -1.0f, 1.0f);
}

float distance_squared(float ax, float az, float bx, float bz) {
    const float dx = ax - bx;
    const float dz = az - bz;
    return dx * dx + dz * dz;
}

} // namespace

void Game::reset() {
    state_ = State::Title;
    collected_mask_ = 0;
    x_ = 0.0f;
    z_ = 0.0f;
    elapsed_ = 0.0f;
    score_ = 0.0f;
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
    x_ = std::clamp(x_ + clamp_axis(input.move_x) * speed * dt, -ArenaLimit, ArenaLimit);
    z_ = std::clamp(z_ + clamp_axis(input.move_z) * speed * dt, -ArenaLimit, ArenaLimit);
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

    if (distance_squared(x_, z_, hazard_x_, 3.0f) <= HazardRadius * HazardRadius ||
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
    return {state_, collected_mask_, x_, z_, elapsed_, score_};
}

} // namespace starfield