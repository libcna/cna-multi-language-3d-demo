#include "StarfieldGame.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RenderTarget2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/RenderTargetUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "System/TimeSpan.hpp"

namespace starfield
{
    namespace xna = Microsoft::Xna::Framework;
    using xna::Color;
    using xna::Matrix;
    using xna::Vector3;
    using xna::Graphics::VertexPositionColor;

    namespace
    {
        constexpr float Pi = 3.14159265358979323846f;
        constexpr float ArenaLimit = 10.0f;
        constexpr float TurnSpeed = Pi;
        constexpr float NormalSpeed = 4.0f;
        constexpr float BoostSpeed = 7.0f;
        constexpr float HazardSpeed = 2.0f;
        constexpr float CellRadius = 0.9f;
        constexpr float HazardCollisionRadius = 1.75f;
        constexpr float ExtractionRadius = 1.4f;
        constexpr float TimeLimit = 60.0f;

        float DistanceSquaredXZ(const Vector3& left, const Vector3& right)
        {
            const float dx = left.X - right.X;
            const float dz = left.Z - right.Z;
            return dx * dx + dz * dz;
        }

        std::vector<VertexPositionColor> MakeCube(const Color& color)
        {
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
            for (const int index : indices)
            {
                result.emplace_back(points[static_cast<std::size_t>(index)], color);
            }
            return result;
        }

        constexpr std::array<std::uint8_t, 10> DigitSegments = {
            0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011,
            0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011};
    }

    StarfieldGame::StarfieldGame(RuntimeOptions options)
        : options_(std::move(options))
    {
        graphics_ = std::make_unique<xna::GraphicsDeviceManager>(this);
        graphics_->setPreferredBackBufferWidthProperty(ReferenceWidth);
        graphics_->setPreferredBackBufferHeightProperty(ReferenceHeight);
        graphics_->setPreferredDepthStencilFormatProperty(xna::Graphics::DepthFormat::Depth24);
        setIsFixedTimeStepProperty(true);
        setTargetElapsedTimeProperty(System::TimeSpan::FromMilliseconds(1000.0 / 60.0));
        setIsMouseVisibleProperty(true);
        getWindowProperty().setTitleProperty("CNA Starfield Courier");
        ResetGameplay();
    }

    StarfieldGame::~StarfieldGame() = default;

    RunState StarfieldGame::runState() const noexcept
    {
        return runState_;
    }

    const Player& StarfieldGame::player() const noexcept
    {
        return player_;
    }

    const std::array<Hazard, 2>& StarfieldGame::hazards() const noexcept
    {
        return hazards_;
    }

    const std::array<Collectible, 3>& StarfieldGame::collectibles() const noexcept
    {
        return collectibles_;
    }

    const Vector3& StarfieldGame::cameraPosition() const noexcept
    {
        return cameraPosition_;
    }

    const Vector3& StarfieldGame::cameraTarget() const noexcept
    {
        return cameraTarget_;
    }

    float StarfieldGame::elapsedSeconds() const noexcept
    {
        return elapsedSeconds_;
    }

    int StarfieldGame::score() const noexcept
    {
        return score_;
    }

    int StarfieldGame::collectedCount() const noexcept
    {
        return static_cast<int>(std::count_if(
            collectibles_.begin(), collectibles_.end(),
            [](const Collectible& collectible) { return collectible.collected; }));
    }

    bool StarfieldGame::frameValid() const noexcept
    {
        return frameValid_;
    }

    void StarfieldGame::Initialize()
    {
        ResetGameplay();
        xna::Game::Initialize();
    }

    void StarfieldGame::LoadContent()
    {
        auto& device = getGraphicsDeviceProperty();
        effect_ = std::make_unique<xna::Graphics::BasicEffect>(device);
        effect_->VertexColorEnabled = true;

        if (!options_.screenshotPath.empty() || options_.validateFrame)
        {
            captureTarget_ = std::make_unique<xna::Graphics::RenderTarget2D>(
                device, ReferenceWidth, ReferenceHeight, false,
                xna::Graphics::SurfaceFormat::Color,
                xna::Graphics::DepthFormat::Depth24,
                0, xna::Graphics::RenderTargetUsage::PreserveContents);
        }
    }

    void StarfieldGame::UnloadContent()
    {
        captureTarget_.reset();
        effect_.reset();
    }

    void StarfieldGame::Update(xna::GameTime& gameTime)
    {
        const auto keyboard = xna::Input::Keyboard::GetState();
        if (keyboard.IsKeyDown(xna::Input::Keys::Escape))
        {
            Exit();
            return;
        }

        const float turn =
            (keyboard.IsKeyDown(xna::Input::Keys::D) ||
             keyboard.IsKeyDown(xna::Input::Keys::Right) ? 1.0f : 0.0f) -
            (keyboard.IsKeyDown(xna::Input::Keys::A) ||
             keyboard.IsKeyDown(xna::Input::Keys::Left) ? 1.0f : 0.0f);
        const float throttle =
            (keyboard.IsKeyDown(xna::Input::Keys::W) ||
             keyboard.IsKeyDown(xna::Input::Keys::Up) ? 1.0f : 0.0f) -
            (keyboard.IsKeyDown(xna::Input::Keys::S) ||
             keyboard.IsKeyDown(xna::Input::Keys::Down) ? 1.0f : 0.0f);
        const bool restart = keyboard.IsKeyDown(xna::Input::Keys::R) ||
                             keyboard.IsKeyDown(xna::Input::Keys::Enter);
        const bool boost = keyboard.IsKeyDown(xna::Input::Keys::Space);
        const float seconds = static_cast<float>(
            gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());

        AdvanceGameplay(seconds, turn, throttle, boost, restart);
        xna::Game::Update(gameTime);
    }

    void StarfieldGame::Draw(const xna::GameTime& gameTime)
    {
        auto& device = getGraphicsDeviceProperty();
        const bool captureThisFrame = captureTarget_ && !captured_;
        if (captureThisFrame)
        {
            device.SetRenderTarget(captureTarget_.get());
        }

        device.Clear(Color(4, 10, 30));
        device.setRasterizerStateProperty(xna::Graphics::RasterizerState::CullNone);
        DrawWorld();
        DrawHud();

        if (captureThisFrame)
        {
            device.SetRenderTarget(nullptr);
            CaptureFrame();
            captured_ = true;
        }

        xna::Game::Draw(gameTime);
        ++drawnFrames_;
        if (options_.smokeFrames > 0 && drawnFrames_ >= options_.smokeFrames)
        {
            Exit();
        }
    }

    void StarfieldGame::ResetGameplay()
    {
        runState_ = RunState::Title;
        player_ = Player{Vector3(0.0f, 0.6f, 0.0f), 0.0f};
        hazards_ = {
            Hazard{Vector3(0.0f, 0.8f, 3.0f), HazardSpeed},
            Hazard{Vector3(0.0f, 0.6f, -3.5f), -HazardSpeed}};
        collectibles_ = {
            Collectible{Vector3(-6.0f, 0.7f, 0.0f), false},
            Collectible{Vector3(0.0f, 0.7f, -5.0f), false},
            Collectible{Vector3(6.0f, 0.7f, 0.0f), false}};
        elapsedSeconds_ = 0.0f;
        score_ = 0;
        UpdateCamera();
    }

    void StarfieldGame::AdvanceGameplay(float seconds, float turn, float throttle,
                                        bool boost, bool restart)
    {
        if (restart)
        {
            ResetGameplay();
            return;
        }
        if (runState_ == RunState::Won || runState_ == RunState::Lost)
        {
            return;
        }
        if (runState_ == RunState::Title)
        {
            runState_ = RunState::Playing;
        }

        const float dt = std::clamp(seconds, 0.0f, 0.25f);
        player_.heading = std::remainder(
            player_.heading + std::clamp(turn, -1.0f, 1.0f) * TurnSpeed * dt,
            2.0f * Pi);

        const float speed = boost ? BoostSpeed : NormalSpeed;
        const float movement = std::clamp(throttle, -1.0f, 1.0f) * speed * dt;
        player_.position.X = std::clamp(
            player_.position.X + std::sin(player_.heading) * movement,
            -ArenaLimit, ArenaLimit);
        player_.position.Z = std::clamp(
            player_.position.Z - std::cos(player_.heading) * movement,
            -ArenaLimit, ArenaLimit);

        elapsedSeconds_ += dt;
        for (Hazard& hazard : hazards_)
        {
            MoveHazard(hazard, dt);
        }

        for (Collectible& collectible : collectibles_)
        {
            if (!collectible.collected &&
                DistanceSquaredXZ(player_.position, collectible.position) <=
                    CellRadius * CellRadius)
            {
                collectible.collected = true;
                score_ += 100;
            }
        }

        const bool hitHazard = std::any_of(
            hazards_.begin(), hazards_.end(),
            [this](const Hazard& hazard)
            {
                return DistanceSquaredXZ(player_.position, hazard.position) <=
                       HazardCollisionRadius * HazardCollisionRadius;
            });
        if (hitHazard || elapsedSeconds_ >= TimeLimit)
        {
            runState_ = RunState::Lost;
            UpdateCamera();
            return;
        }

        const Vector3 extractionPosition(0.0f, 0.0f, -9.0f);
        if (collectedCount() == static_cast<int>(collectibles_.size()) &&
            DistanceSquaredXZ(player_.position, extractionPosition) <=
                ExtractionRadius * ExtractionRadius)
        {
            runState_ = RunState::Won;
            score_ += 1000;
        }
        UpdateCamera();
    }

    void StarfieldGame::UpdateCamera()
    {
        const float forwardX = std::sin(player_.heading);
        const float forwardZ = -std::cos(player_.heading);
        cameraPosition_ = Vector3(
            player_.position.X - forwardX * 10.0f,
            7.0f,
            player_.position.Z - forwardZ * 10.0f);
        cameraTarget_ = Vector3(player_.position.X, 0.5f, player_.position.Z);
    }

    void StarfieldGame::MoveHazard(Hazard& hazard, float seconds)
    {
        hazard.position.X += hazard.velocityX * seconds;
        if (hazard.position.X >= 7.0f)
        {
            hazard.position.X = 7.0f;
            hazard.velocityX = -HazardSpeed;
        }
        else if (hazard.position.X <= -7.0f)
        {
            hazard.position.X = -7.0f;
            hazard.velocityX = HazardSpeed;
        }
    }

    void StarfieldGame::DrawWorld()
    {
        auto& device = getGraphicsDeviceProperty();
        device.setDepthStencilStateProperty(xna::Graphics::DepthStencilState::Default);

        effect_->World = Matrix::getIdentityProperty();
        effect_->View = Matrix::CreateLookAt(cameraPosition_, cameraTarget_, Vector3::Up);
        effect_->Projection = Matrix::CreatePerspectiveFieldOfView(
            Pi / 4.0f,
            static_cast<float>(ReferenceWidth) / static_cast<float>(ReferenceHeight),
            0.1f, 100.0f);
        auto& pass = effect_->getTechniquesProperty()[0].getPassesProperty()[0];

        DrawCube(device, pass, Vector3(0.0f, -0.25f, 0.0f),
                 Vector3(20.0f, 0.5f, 20.0f), Color(25, 45, 90));
        for (int offset = -10; offset <= 10; offset += 2)
        {
            DrawCube(device, pass, Vector3(static_cast<float>(offset), 0.02f, 0.0f),
                     Vector3(0.035f, 0.035f, 20.0f), Color(55, 90, 135));
            DrawCube(device, pass, Vector3(0.0f, 0.02f, static_cast<float>(offset)),
                     Vector3(20.0f, 0.035f, 0.035f), Color(55, 90, 135));
        }

        const Color boundaryColor(255, 120, 20);
        const Color markerColor(255, 180, 50);
        DrawCube(device, pass, Vector3(-10.1f, 0.35f, 0.0f),
                 Vector3(0.2f, 0.7f, 20.4f), boundaryColor);
        DrawCube(device, pass, Vector3(10.1f, 0.35f, 0.0f),
                 Vector3(0.2f, 0.7f, 20.4f), boundaryColor);
        DrawCube(device, pass, Vector3(0.0f, 0.35f, -10.1f),
                 Vector3(20.4f, 0.7f, 0.2f), boundaryColor);
        DrawCube(device, pass, Vector3(0.0f, 0.35f, 10.1f),
                 Vector3(20.4f, 0.7f, 0.2f), boundaryColor);
        for (const float edge : {-10.0f, 10.0f})
        {
            for (const float offset : {-10.0f, -5.0f, 0.0f, 5.0f, 10.0f})
            {
                DrawCube(device, pass, Vector3(edge, 1.0f, offset),
                         Vector3(0.35f, 2.0f, 0.35f), markerColor);
                DrawCube(device, pass, Vector3(offset, 1.0f, edge),
                         Vector3(0.35f, 2.0f, 0.35f), markerColor);
            }
        }

        const float forwardX = std::sin(player_.heading);
        const float forwardZ = -std::cos(player_.heading);
        DrawCube(device, pass, player_.position, Vector3(1.2f, 0.6f, 1.8f),
                 Color(0, 220, 255), -player_.heading);
        DrawCube(device, pass,
                 Vector3(player_.position.X + forwardX * 0.95f, 0.65f,
                         player_.position.Z + forwardZ * 0.95f),
                 Vector3(0.5f, 0.35f, 0.45f), Color(100, 245, 255),
                 -player_.heading);

        for (const Collectible& collectible : collectibles_)
        {
            if (!collectible.collected)
            {
                DrawCube(device, pass, collectible.position, Vector3(0.8f, 1.4f, 0.8f),
                         Color(255, 210, 0));
            }
        }

        DrawCube(device, pass, hazards_[0].position, Vector3(2.0f, 1.6f, 1.0f),
                 Color(230, 40, 50));
        DrawCube(device, pass, hazards_[1].position, Vector3(1.2f, 1.2f, 1.2f),
                 Color(255, 55, 70));

        const Color gateColor = collectedCount() == static_cast<int>(collectibles_.size())
                                    ? Color(40, 255, 80)
                                    : Color(20, 100, 50);
        DrawCube(device, pass, Vector3(-1.35f, 1.0f, -9.0f),
                 Vector3(0.3f, 2.0f, 0.5f), gateColor);
        DrawCube(device, pass, Vector3(1.35f, 1.0f, -9.0f),
                 Vector3(0.3f, 2.0f, 0.5f), gateColor);
        DrawCube(device, pass, Vector3(0.0f, 1.9f, -9.0f),
                 Vector3(3.0f, 0.3f, 0.5f), gateColor);
    }

    void StarfieldGame::DrawHud()
    {
        auto& device = getGraphicsDeviceProperty();
        device.setDepthStencilStateProperty(xna::Graphics::DepthStencilState::None);
        effect_->View = Matrix::getIdentityProperty();
        effect_->Projection = Matrix::CreateOrthographicOffCenter(
            0.0f, static_cast<float>(ReferenceWidth),
            static_cast<float>(ReferenceHeight), 0.0f, 0.0f, 10.0f);
        auto& pass = effect_->getTechniquesProperty()[0].getPassesProperty()[0];

        DrawCube(device, pass, Vector3(225.0f, 48.0f, 1.0f),
                 Vector3(410.0f, 76.0f, 0.1f), Color(8, 18, 42));
        for (std::size_t index = 0; index < collectibles_.size(); ++index)
        {
            DrawCube(device, pass,
                     Vector3(48.0f + static_cast<float>(index) * 42.0f, 48.0f, 0.0f),
                     Vector3(28.0f, 28.0f, 0.1f),
                     collectibles_[index].collected ? Color(255, 215, 0)
                                                    : Color(45, 60, 85));
        }

        const float remaining = std::max(0.0f, TimeLimit - elapsedSeconds_) / TimeLimit;
        DrawCube(device, pass, Vector3(238.0f, 48.0f, 0.0f),
                 Vector3(140.0f, 16.0f, 0.1f), Color(45, 60, 85));
        if (remaining > 0.0f)
        {
            DrawCube(device, pass,
                     Vector3(168.0f + remaining * 70.0f, 48.0f, -0.1f),
                     Vector3(remaining * 140.0f, 16.0f, 0.1f),
                     remaining > 0.25f ? Color(60, 230, 130) : Color(255, 80, 50));
        }

        const Color stateColor = runState_ == RunState::Won
                                     ? Color(40, 255, 80)
                                     : runState_ == RunState::Lost
                                           ? Color(255, 50, 50)
                                           : Color(0, 200, 255);
        DrawCube(device, pass, Vector3(365.0f, 48.0f, 0.0f),
                 Vector3(28.0f, 28.0f, 0.1f), stateColor);

        int displayScore = score_;
        for (int digitIndex = 3; digitIndex >= 0; --digitIndex)
        {
            DrawDigit(device, pass, displayScore % 10,
                      1195.0f - static_cast<float>(3 - digitIndex) * 28.0f,
                      48.0f, Color(100, 245, 255));
            displayScore /= 10;
        }
    }

    void StarfieldGame::DrawCube(xna::Graphics::GraphicsDevice& device,
                                 xna::Graphics::EffectPass& pass,
                                 const Vector3& position, const Vector3& scale,
                                 const Color& color, float yaw)
    {
        const auto vertices = MakeCube(color);
        effect_->World = Matrix::CreateScale(scale) * Matrix::CreateRotationY(yaw) *
                         Matrix::CreateTranslation(position);
        pass.Apply();
        device.DrawUserPrimitives(xna::Graphics::PrimitiveType::TriangleList,
                                  vertices.data(), 0,
                                  static_cast<int>(vertices.size() / 3));
    }

    void StarfieldGame::DrawDigit(xna::Graphics::GraphicsDevice& device,
                                  xna::Graphics::EffectPass& pass,
                                  int digit, float x, float y, const Color& color)
    {
        const std::uint8_t segments = DigitSegments[static_cast<std::size_t>(digit)];
        const auto horizontal = [&](std::uint8_t bit, float offsetY)
        {
            if ((segments & bit) != 0)
            {
                DrawCube(device, pass, Vector3(x, y + offsetY, 0.0f),
                         Vector3(16.0f, 4.0f, 0.1f), color);
            }
        };
        const auto vertical = [&](std::uint8_t bit, float offsetX, float offsetY)
        {
            if ((segments & bit) != 0)
            {
                DrawCube(device, pass, Vector3(x + offsetX, y + offsetY, 0.0f),
                         Vector3(4.0f, 16.0f, 0.1f), color);
            }
        };

        horizontal(0b1000000, -18.0f);
        vertical(0b0100000, 8.0f, -9.0f);
        vertical(0b0010000, 8.0f, 9.0f);
        horizontal(0b0001000, 18.0f);
        vertical(0b0000100, -8.0f, 9.0f);
        vertical(0b0000010, -8.0f, -9.0f);
        horizontal(0b0000001, 0.0f);
    }

    void StarfieldGame::CaptureFrame()
    {
        const int width = captureTarget_->getWidthProperty();
        const int height = captureTarget_->getHeightProperty();
        std::vector<Color> pixels(static_cast<std::size_t>(width) *
                                  static_cast<std::size_t>(height));
        captureTarget_->GetData(pixels.data(), static_cast<int>(pixels.size()));

        std::size_t nonClearPixels = 0;
        std::size_t floorPixels = 0;
        std::size_t gridPixels = 0;
        std::size_t boundaryPixels = 0;
        std::size_t playerPixels = 0;
        std::size_t collectiblePixels = 0;
        std::size_t hazardPixels = 0;
        std::size_t hudPixels = 0;
        const Color clear(4, 10, 30);
        for (const Color& pixel : pixels)
        {
            nonClearPixels += pixel != clear;
            floorPixels += pixel == Color(25, 45, 90);
            gridPixels += pixel == Color(55, 90, 135);
            boundaryPixels += pixel == Color(255, 120, 20);
            playerPixels += pixel == Color(0, 220, 255);
            collectiblePixels += pixel == Color(255, 210, 0);
            hazardPixels += pixel == Color(230, 40, 50) || pixel == Color(255, 55, 70);
            hudPixels += pixel == Color(8, 18, 42);
        }

        frameValid_ = width == ReferenceWidth && height == ReferenceHeight &&
                      nonClearPixels > pixels.size() / 100 &&
                      floorPixels > pixels.size() / 10 && gridPixels > 5000 &&
                      boundaryPixels > 5000 && playerPixels > 500 &&
                      collectiblePixels > 300 && hazardPixels > 500 && hudPixels > 5000;
        if (options_.validateFrame && !frameValid_)
        {
            throw std::runtime_error(
                "captured game frame is incomplete, blank, or not 1280x720");
        }

        if (options_.screenshotPath.empty())
        {
            return;
        }

        std::ofstream output(options_.screenshotPath, std::ios::binary);
        if (!output)
        {
            throw std::runtime_error("cannot open screenshot: " + options_.screenshotPath);
        }
        output << "P6\n" << width << ' ' << height << "\n255\n";
        for (const Color& pixel : pixels)
        {
            const std::array<char, 3> rgb = {
                static_cast<char>(pixel.getRProperty()),
                static_cast<char>(pixel.getGProperty()),
                static_cast<char>(pixel.getBProperty())};
            output.write(rgb.data(), static_cast<std::streamsize>(rgb.size()));
        }
        if (!output)
        {
            throw std::runtime_error("failed to write screenshot: " + options_.screenshotPath);
        }
    }
}
