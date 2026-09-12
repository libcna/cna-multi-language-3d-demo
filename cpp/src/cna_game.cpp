#include "starfield.hpp"

#include <cstdlib>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace xna = Microsoft::Xna::Framework;

namespace {

using xna::Color;
using xna::Vector3;
using xna::Graphics::VertexPositionColor;

std::vector<VertexPositionColor> cube(const Color& color) {
    const std::array<Vector3, 8> points = {
        Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, -0.5f, -0.5f),
        Vector3(0.5f, 0.5f, -0.5f), Vector3(-0.5f, 0.5f, -0.5f),
        Vector3(-0.5f, -0.5f, 0.5f), Vector3(0.5f, -0.5f, 0.5f),
        Vector3(0.5f, 0.5f, 0.5f), Vector3(-0.5f, 0.5f, 0.5f)};
    constexpr std::array<int, 36> indices = {
        0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6,
        0, 4, 5, 0, 5, 1, 3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2, 0, 3, 7, 0, 7, 4};
    std::vector<VertexPositionColor> result;
    result.reserve(indices.size());
    for (const int index : indices) {
        result.emplace_back(points[static_cast<std::size_t>(index)], color);
    }
    return result;
}

class StarfieldGame final : public xna::Game {
public:
    StarfieldGame() {
        setIsFixedTimeStepProperty(true);
        setTargetElapsedTimeProperty(System::TimeSpan::FromMilliseconds(16.666667));
        setIsMouseVisibleProperty(true);
    }

    [[nodiscard]] const starfield::Snapshot& snapshot() const { return snapshot_; }

protected:
    void Initialize() override {
        simulation_.reset();
        snapshot_ = simulation_.snapshot();
        xna::Game::Initialize();
        effect_ = std::make_unique<xna::Graphics::BasicEffect>(getGraphicsDeviceProperty());
    }

    void Update(xna::GameTime& game_time) override {
        const float seconds = static_cast<float>(
            game_time.getElapsedGameTimeProperty().getTotalSecondsProperty());

        const auto keys = xna::Input::Keyboard::GetState();
        starfield::Input input;
        input.move_x = (keys.IsKeyDown(xna::Input::Keys::D) || keys.IsKeyDown(xna::Input::Keys::Right) ? 1.0f : 0.0f) -
                       (keys.IsKeyDown(xna::Input::Keys::A) || keys.IsKeyDown(xna::Input::Keys::Left) ? 1.0f : 0.0f);
        input.move_z = (keys.IsKeyDown(xna::Input::Keys::S) || keys.IsKeyDown(xna::Input::Keys::Down) ? 1.0f : 0.0f) -
                       (keys.IsKeyDown(xna::Input::Keys::W) || keys.IsKeyDown(xna::Input::Keys::Up) ? 1.0f : 0.0f);
        input.boost = keys.IsKeyDown(xna::Input::Keys::Space);
        input.restart = keys.IsKeyDown(xna::Input::Keys::Enter);
        if (keys.IsKeyDown(xna::Input::Keys::Escape)) {
            Exit();
        }
        simulation_.update(seconds, input);
        snapshot_ = simulation_.snapshot();
        xna::Game::Update(game_time);
    }

    void Draw(const xna::GameTime& game_time) override {
        (void)game_time;
        auto& device = getGraphicsDeviceProperty();
        const xna::Color color = snapshot_.state == starfield::State::Won
                                     ? xna::Color::Green
                                     : snapshot_.state == starfield::State::Lost
                                           ? xna::Color::Red
                                           : xna::Color::MidnightBlue;
        device.Clear(color);
        if (!effect_) {
            xna::Game::Draw(game_time);
            return;
        }

        effect_->World = xna::Matrix::getIdentityProperty();
        effect_->View = xna::Matrix::CreateLookAt(
            Vector3(snapshot_.x, 7.0f, snapshot_.z + 10.0f),
            Vector3(snapshot_.x, 0.5f, snapshot_.z), Vector3(0.0f, 1.0f, 0.0f));
        effect_->Projection = xna::Matrix::CreatePerspectiveFieldOfView(
            0.785398163f, 16.0f / 9.0f, 0.1f, 100.0f);
        effect_->VertexColorEnabled = true;
        auto& pass = effect_->getTechniquesProperty()[0].getPassesProperty()[0];

        drawCube(device, pass, Vector3(0.0f, -0.25f, 0.0f), Vector3(20.0f, 0.5f, 20.0f), Color(25, 45, 90));
        drawCube(device, pass, Vector3(snapshot_.x, 0.6f, snapshot_.z), Vector3(1.2f, 0.6f, 1.8f), Color(0, 220, 255));
        for (const auto& cell : cells_) {
            if ((snapshot_.collected_mask & cell.mask) == 0) {
                drawCube(device, pass, cell.position, Vector3(0.8f, 1.4f, 0.8f), Color(255, 210, 0));
            }
        }
        const float hazard = std::sin(snapshot_.elapsed * 0.285714f) * 7.0f;
        drawCube(device, pass, Vector3(hazard, 0.8f, 3.0f), Vector3(2.0f, 1.6f, 1.0f), Color(230, 40, 50));
        drawCube(device, pass, Vector3(-hazard, 0.6f, -3.5f), Vector3(1.2f, 1.2f, 1.2f), Color(255, 100, 20));
        drawCube(device, pass, Vector3(0.0f, 1.0f, -9.0f), Vector3(3.0f, 2.0f, 0.5f),
                 snapshot_.collected_mask == 7 ? Color(40, 255, 80) : Color(20, 100, 50));
        xna::Game::Draw(game_time);
    }

private:
    struct Cell { std::uint32_t mask; Vector3 position; };

    void drawCube(xna::Graphics::GraphicsDevice& device,
                  xna::Graphics::EffectPass& pass, Vector3 position, Vector3 scale, const Color& color) {
        const auto vertices = cube(color);
        effect_->World = xna::Matrix::CreateScale(scale) * xna::Matrix::CreateTranslation(position);
        pass.Apply();
        device.DrawUserPrimitives(xna::Graphics::PrimitiveType::TriangleList,
                                  vertices.data(), 0, static_cast<int>(vertices.size() / 3));
    }

    starfield::Game simulation_;
    starfield::Snapshot snapshot_{};
    std::unique_ptr<xna::Graphics::BasicEffect> effect_;
    const std::array<Cell, 3> cells_ = {
        Cell{1u, Vector3(-6.0f, 0.7f, 0.0f)},
        Cell{2u, Vector3(0.0f, 0.7f, -5.0f)},
        Cell{4u, Vector3(6.0f, 0.7f, 0.0f)}};
};

int parse_frame_count(int argc, char** argv) {
    if (argc == 1) {
        return -1;
    }
    if (argc == 3 && std::string_view(argv[1]) == "--frames") {
        return std::atoi(argv[2]);
    }
    std::cerr << "usage: starfield_cna [--frames COUNT]\n";
    return -2;
}

} // namespace

int main(int argc, char** argv) {
    const int frame_count = parse_frame_count(argc, argv);
    if (frame_count == -2 || frame_count < -1) {
        return 2;
    }

    StarfieldGame game;
    if (frame_count < 0) {
        game.Run();
    } else {
        for (int frame = 0; frame < frame_count; ++frame) {
            game.RunOneFrame();
        }
        game.Exit();
    }

    const starfield::Snapshot snapshot = game.snapshot();
    std::cout << "cna-cpp state=" << static_cast<unsigned>(snapshot.state)
              << " collected=" << snapshot.collected_mask << " score=" << snapshot.score
              << "\n";
    return 0;
}