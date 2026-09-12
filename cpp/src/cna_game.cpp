#include "starfield.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RenderTarget2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/RenderTargetUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
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

constexpr int ReferenceWidth = 1280;
constexpr int ReferenceHeight = 720;

struct Options {
    int frame_count = -1;
    std::string scenario;
    std::string screenshot_path;
    bool validate_frame = false;
};

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
    explicit StarfieldGame(Options options) : options_(std::move(options)) {
        graphics_ = std::make_unique<xna::GraphicsDeviceManager>(this);
        graphics_->setPreferredBackBufferWidthProperty(ReferenceWidth);
        graphics_->setPreferredBackBufferHeightProperty(ReferenceHeight);
        setIsFixedTimeStepProperty(true);
        setTargetElapsedTimeProperty(System::TimeSpan::FromMilliseconds(16.666667));
        setIsMouseVisibleProperty(true);
    }

    [[nodiscard]] const starfield::Snapshot& snapshot() const { return snapshot_; }
    [[nodiscard]] bool frame_valid() const { return frame_valid_; }

protected:
    void Initialize() override {
        simulation_.reset();
        if (!options_.scenario.empty() &&
            !starfield::run_scenario(simulation_, options_.scenario)) {
            throw std::invalid_argument("unknown scenario: " + options_.scenario);
        }
        snapshot_ = simulation_.snapshot();
        xna::Game::Initialize();
        effect_ = std::make_unique<xna::Graphics::BasicEffect>(getGraphicsDeviceProperty());
        if (!options_.screenshot_path.empty() || options_.validate_frame) {
            capture_target_ = std::make_unique<xna::Graphics::RenderTarget2D>(
                getGraphicsDeviceProperty(), ReferenceWidth, ReferenceHeight, false,
                xna::Graphics::SurfaceFormat::Color, xna::Graphics::DepthFormat::Depth24,
                0, xna::Graphics::RenderTargetUsage::PreserveContents);
        }
    }

    void Update(xna::GameTime& game_time) override {
        const float seconds = static_cast<float>(
            game_time.getElapsedGameTimeProperty().getTotalSecondsProperty());

        if (options_.scenario.empty()) {
            const auto keys = xna::Input::Keyboard::GetState();
            starfield::Input input;
            input.turn = (keys.IsKeyDown(xna::Input::Keys::D) || keys.IsKeyDown(xna::Input::Keys::Right) ? 1.0f : 0.0f) -
                         (keys.IsKeyDown(xna::Input::Keys::A) || keys.IsKeyDown(xna::Input::Keys::Left) ? 1.0f : 0.0f);
            input.forward = (keys.IsKeyDown(xna::Input::Keys::W) || keys.IsKeyDown(xna::Input::Keys::Up) ? 1.0f : 0.0f) -
                            (keys.IsKeyDown(xna::Input::Keys::S) || keys.IsKeyDown(xna::Input::Keys::Down) ? 1.0f : 0.0f);
            input.boost = keys.IsKeyDown(xna::Input::Keys::Space);
            input.restart = keys.IsKeyDown(xna::Input::Keys::R) ||
                            keys.IsKeyDown(xna::Input::Keys::Enter);
            if (keys.IsKeyDown(xna::Input::Keys::Escape)) {
                Exit();
            }
            simulation_.update(seconds, input);
            snapshot_ = simulation_.snapshot();
        }
        xna::Game::Update(game_time);
    }

    void Draw(const xna::GameTime& game_time) override {
        (void)game_time;
        auto& device = getGraphicsDeviceProperty();
        const bool capture_this_frame = capture_target_ && !captured_;
        if (capture_this_frame) {
            device.SetRenderTarget(capture_target_.get());
        }
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

        device.setDepthStencilStateProperty(xna::Graphics::DepthStencilState::Default);
        const float forward_x = std::sin(snapshot_.heading);
        const float forward_z = -std::cos(snapshot_.heading);
        effect_->World = xna::Matrix::getIdentityProperty();
        effect_->View = xna::Matrix::CreateLookAt(
            Vector3(snapshot_.x - forward_x * 10.0f, 7.0f, snapshot_.z - forward_z * 10.0f),
            Vector3(snapshot_.x, 0.5f, snapshot_.z),
            Vector3(0.0f, 1.0f, 0.0f));
        effect_->Projection = xna::Matrix::CreatePerspectiveFieldOfView(
            0.785398163f, 16.0f / 9.0f, 0.1f, 100.0f);
        effect_->VertexColorEnabled = true;
        auto& pass = effect_->getTechniquesProperty()[0].getPassesProperty()[0];

        drawCube(device, pass, Vector3(0.0f, -0.25f, 0.0f), Vector3(20.0f, 0.5f, 20.0f), Color(25, 45, 90));
        for (int offset = -10; offset <= 10; offset += 2) {
            drawCube(device, pass, Vector3(static_cast<float>(offset), 0.02f, 0.0f),
                     Vector3(0.035f, 0.035f, 20.0f), Color(55, 90, 135));
            drawCube(device, pass, Vector3(0.0f, 0.02f, static_cast<float>(offset)),
                     Vector3(20.0f, 0.035f, 0.035f), Color(55, 90, 135));
        }
        const Color boundary_color(255, 120, 20);
        const Color marker_color(255, 180, 50);
        drawCube(device, pass, Vector3(-10.1f, 0.35f, 0.0f),
                 Vector3(0.2f, 0.7f, 20.4f), boundary_color);
        drawCube(device, pass, Vector3(10.1f, 0.35f, 0.0f),
                 Vector3(0.2f, 0.7f, 20.4f), boundary_color);
        drawCube(device, pass, Vector3(0.0f, 0.35f, -10.1f),
                 Vector3(20.4f, 0.7f, 0.2f), boundary_color);
        drawCube(device, pass, Vector3(0.0f, 0.35f, 10.1f),
                 Vector3(20.4f, 0.7f, 0.2f), boundary_color);
        for (const float edge : {-10.0f, 10.0f}) {
            for (const float offset : {-10.0f, -5.0f, 0.0f, 5.0f, 10.0f}) {
                drawCube(device, pass, Vector3(edge, 1.0f, offset),
                         Vector3(0.35f, 2.0f, 0.35f), marker_color);
                drawCube(device, pass, Vector3(offset, 1.0f, edge),
                         Vector3(0.35f, 2.0f, 0.35f), marker_color);
            }
        }
        drawCube(device, pass, Vector3(snapshot_.x, 0.6f, snapshot_.z),
                 Vector3(1.2f, 0.6f, 1.8f), Color(0, 220, 255), -snapshot_.heading);
        drawCube(device, pass,
                 Vector3(snapshot_.x + forward_x * 0.95f, 0.65f,
                         snapshot_.z + forward_z * 0.95f),
                 Vector3(0.5f, 0.35f, 0.45f), Color(100, 245, 255), -snapshot_.heading);
        for (const auto& cell : cells_) {
            if ((snapshot_.collected_mask & cell.mask) == 0) {
                drawCube(device, pass, cell.position, Vector3(0.8f, 1.4f, 0.8f), Color(255, 210, 0));
            }
        }
        drawCube(device, pass, Vector3(snapshot_.hazard_x, 0.8f, 3.0f),
                 Vector3(2.0f, 1.6f, 1.0f), Color(230, 40, 50));
        drawCube(device, pass, Vector3(snapshot_.secondary_hazard_x, 0.6f, -3.5f),
                 Vector3(1.2f, 1.2f, 1.2f), Color(255, 55, 70));
        const Color gate_color = snapshot_.collected_mask == 7
                                     ? Color(40, 255, 80)
                                     : Color(20, 100, 50);
        drawCube(device, pass, Vector3(-1.35f, 1.0f, -9.0f),
                 Vector3(0.3f, 2.0f, 0.5f), gate_color);
        drawCube(device, pass, Vector3(1.35f, 1.0f, -9.0f),
                 Vector3(0.3f, 2.0f, 0.5f), gate_color);
        drawCube(device, pass, Vector3(0.0f, 1.9f, -9.0f),
                 Vector3(3.0f, 0.3f, 0.5f), gate_color);

        drawHud(device, pass);
        if (capture_this_frame) {
            device.SetRenderTarget(nullptr);
            captureFrame();
            captured_ = true;
        }
        xna::Game::Draw(game_time);
    }

private:
    struct Cell { std::uint32_t mask; Vector3 position; };

    void drawCube(xna::Graphics::GraphicsDevice& device,
                  xna::Graphics::EffectPass& pass, Vector3 position, Vector3 scale, const Color& color,
                  float yaw = 0.0f) {
        const auto vertices = cube(color);
        effect_->World = xna::Matrix::CreateScale(scale) * xna::Matrix::CreateRotationY(yaw) *
                         xna::Matrix::CreateTranslation(position);
        pass.Apply();
        device.DrawUserPrimitives(xna::Graphics::PrimitiveType::TriangleList,
                                  vertices.data(), 0, static_cast<int>(vertices.size() / 3));
    }

    void drawHud(xna::Graphics::GraphicsDevice& device, xna::Graphics::EffectPass& pass) {
        device.setDepthStencilStateProperty(xna::Graphics::DepthStencilState::None);
        effect_->View = xna::Matrix::getIdentityProperty();
        effect_->Projection = xna::Matrix::CreateOrthographicOffCenter(
            0.0f, 1280.0f, 720.0f, 0.0f, 0.0f, 10.0f);

        drawCube(device, pass, Vector3(170.0f, 42.0f, 1.0f),
                 Vector3(300.0f, 64.0f, 0.1f), Color(8, 18, 42));
        for (int index = 0; index < 3; ++index) {
            const bool collected = (snapshot_.collected_mask & (1u << index)) != 0;
            drawCube(device, pass, Vector3(55.0f + static_cast<float>(index) * 42.0f, 42.0f, 0.0f),
                     Vector3(28.0f, 28.0f, 0.1f),
                     collected ? Color(255, 215, 0) : Color(45, 60, 85));
        }

        const float remaining = std::max(0.0f, 60.0f - snapshot_.elapsed) / 60.0f;
        drawCube(device, pass, Vector3(205.0f, 42.0f, 0.0f),
                 Vector3(110.0f, 14.0f, 0.1f), Color(45, 60, 85));
        if (remaining > 0.0f) {
            drawCube(device, pass, Vector3(150.0f + remaining * 55.0f, 42.0f, -0.1f),
                     Vector3(remaining * 110.0f, 14.0f, 0.1f),
                     remaining > 0.25f ? Color(60, 230, 130) : Color(255, 80, 50));
        }

        const Color state_color = snapshot_.state == starfield::State::Won
                                      ? Color(40, 255, 80)
                                      : snapshot_.state == starfield::State::Lost
                                            ? Color(255, 50, 50)
                                            : Color(0, 200, 255);
        drawCube(device, pass, Vector3(300.0f, 42.0f, 0.0f),
                 Vector3(24.0f, 24.0f, 0.1f), state_color);
    }

    void captureFrame() {
        const int width = capture_target_->getWidthProperty();
        const int height = capture_target_->getHeightProperty();
        std::vector<Color> pixels(static_cast<std::size_t>(width) *
                                  static_cast<std::size_t>(height));
        capture_target_->GetData(pixels.data(), static_cast<int>(pixels.size()));

        std::size_t non_clear_pixels = 0;
        std::size_t floor_pixels = 0;
        std::size_t grid_pixels = 0;
        std::size_t boundary_pixels = 0;
        std::size_t player_pixels = 0;
        std::size_t hazard_pixels = 0;
        std::size_t hud_pixels = 0;
        const Color clear = snapshot_.state == starfield::State::Won
                                ? Color::Green
                                : snapshot_.state == starfield::State::Lost
                                      ? Color::Red
                                      : Color::MidnightBlue;
        for (const Color& pixel : pixels) {
            if (pixel != clear) {
                ++non_clear_pixels;
            }
            floor_pixels += pixel == Color(25, 45, 90);
            grid_pixels += pixel == Color(55, 90, 135);
            boundary_pixels += pixel == Color(255, 120, 20);
            player_pixels += pixel == Color(0, 220, 255);
            hazard_pixels += pixel == Color(230, 40, 50) || pixel == Color(255, 55, 70);
            hud_pixels += pixel == Color(8, 18, 42);
        }
        frame_valid_ = width == ReferenceWidth && height == ReferenceHeight &&
                       non_clear_pixels > pixels.size() / 100 &&
                       floor_pixels > pixels.size() / 10 && grid_pixels > 5000 &&
                       boundary_pixels > 5000 && player_pixels > 500 &&
                       hazard_pixels > 500 && hud_pixels > 5000;
        if (options_.validate_frame && !frame_valid_) {
            throw std::runtime_error(
                "captured frame is incomplete, blank, or not 1280x720");
        }

        if (options_.screenshot_path.empty()) {
            return;
        }
        std::ofstream output(options_.screenshot_path, std::ios::binary);
        if (!output) {
            throw std::runtime_error("cannot open screenshot: " + options_.screenshot_path);
        }
        output << "P6\n" << width << ' ' << height << "\n255\n";
        for (const Color& pixel : pixels) {
            const std::array<char, 3> rgb = {
                static_cast<char>(pixel.getRProperty()),
                static_cast<char>(pixel.getGProperty()),
                static_cast<char>(pixel.getBProperty())};
            output.write(rgb.data(), static_cast<std::streamsize>(rgb.size()));
        }
        if (!output) {
            throw std::runtime_error("failed to write screenshot: " + options_.screenshot_path);
        }
    }

    starfield::Game simulation_;
    starfield::Snapshot snapshot_{};
    Options options_;
    bool captured_ = false;
    bool frame_valid_ = true;
    std::unique_ptr<xna::GraphicsDeviceManager> graphics_;
    std::unique_ptr<xna::Graphics::BasicEffect> effect_;
    std::unique_ptr<xna::Graphics::RenderTarget2D> capture_target_;
    const std::array<Cell, 3> cells_ = {
        Cell{1u, Vector3(-6.0f, 0.7f, 0.0f)},
        Cell{2u, Vector3(0.0f, 0.7f, -5.0f)},
        Cell{4u, Vector3(6.0f, 0.7f, 0.0f)}};
};

bool parse_nonnegative(const char* text, int& value) {
    if (text == nullptr || *text == '\0') {
        return false;
    }
    char* end = nullptr;
    errno = 0;
    const long parsed = std::strtol(text, &end, 10);
    if (errno != 0 || *end != '\0' || parsed < 0 ||
        parsed > std::numeric_limits<int>::max()) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

bool parse_options(int argc, char** argv, Options& options) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--frames" && index + 1 < argc) {
            if (!parse_nonnegative(argv[++index], options.frame_count)) {
                return false;
            }
        } else if (argument == "--scenario" && index + 1 < argc) {
            options.scenario = argv[++index];
        } else if (argument == "--screenshot" && index + 1 < argc) {
            options.screenshot_path = argv[++index];
        } else if (argument == "--validate-frame") {
            options.validate_frame = true;
        } else {
            return false;
        }
    }
    if (options.frame_count < 0 &&
        (!options.scenario.empty() || !options.screenshot_path.empty() || options.validate_frame)) {
        options.frame_count = 1;
    }
    const bool known_scenario = options.scenario.empty() || options.scenario == "startup" ||
                                options.scenario == "collection" || options.scenario == "hazard" ||
                                options.scenario == "win" || options.scenario == "loss" ||
                                options.scenario == "restart";
    const bool needs_frame = !options.scenario.empty() || !options.screenshot_path.empty() ||
                             options.validate_frame;
    return known_scenario && (!needs_frame || options.frame_count > 0);
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse_options(argc, argv, options)) {
        std::cerr << "usage: starfield_cna [--frames COUNT] "
                     "[--scenario startup|collection|hazard|win|loss|restart] "
                     "[--validate-frame] [--screenshot FILE.ppm]\n";
        return 2;
    }

    const int frame_count = options.frame_count;
    StarfieldGame game(std::move(options));
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
              << " x=" << snapshot.x << " z=" << snapshot.z
              << " heading=" << snapshot.heading << " elapsed=" << snapshot.elapsed << "\n";
    return game.frame_valid() ? 0 : 1;
}
