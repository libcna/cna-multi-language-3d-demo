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
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
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
#include "Microsoft/Xna/Framework/Media/MediaPlayer.hpp"
#include "Microsoft/Xna/Framework/Media/Song.hpp"
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
        constexpr int SectorBonus = 500;
        constexpr int FinalMissionBonus = 1000;

        struct SectorTheme
        {
            Color sky;
            Color floor;
            Color grid;
            Color boundary;
            Color accent;
        };

        SectorTheme ThemeForSector(int sectorIndex)
        {
            switch (sectorIndex)
            {
            case 1:
                return {Color(13, 5, 27), Color(55, 30, 85), Color(130, 65, 150),
                        Color(30, 220, 220), Color(205, 80, 255)};
            case 2:
                return {Color(25, 5, 8), Color(72, 27, 25), Color(150, 58, 35),
                        Color(255, 185, 35), Color(255, 75, 35)};
            default:
                return {Color(4, 10, 30), Color(25, 45, 90), Color(55, 90, 135),
                        Color(255, 120, 20), Color(0, 175, 225)};
            }
        }

        const char* SectorName(int sectorIndex)
        {
            switch (sectorIndex)
            {
            case 1: return "ION BASIN";
            case 2: return "SOLAR FORGE";
            default: return "STARPORT";
            }
        }

        float DistanceSquaredXZ(const Vector3& left, const Vector3& right)
        {
            const float dx = left.X - right.X;
            const float dz = left.Z - right.Z;
            return dx * dx + dz * dz;
        }

        Color Shade(const Color& color, float amount)
        {
            const auto component = [amount](int value)
            {
                return std::clamp(static_cast<int>(std::lround(value * amount)), 0, 255);
            };
            return Color(component(color.getRProperty()),
                         component(color.getGProperty()),
                         component(color.getBProperty()),
                         static_cast<int>(color.getAProperty()));
        }

        std::array<VertexPositionColor, 36> MakeCube(const Color& color)
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

            // Face tinting gives the procedural geometry readable form without
            // shaders or renderer-specific lighting extensions. The fourth face
            // is the upward face and retains the exact requested object color.
            constexpr std::array<float, 6> faceBrightness = {
                0.82f, 0.58f, 0.34f, 1.0f, 0.72f, 0.48f};
            std::array<VertexPositionColor, 36> result;
            for (std::size_t vertex = 0; vertex < indices.size(); ++vertex)
            {
                const std::size_t face = vertex / 6;
                result[vertex] = VertexPositionColor(
                    points[static_cast<std::size_t>(indices[vertex])],
                    Shade(color, faceBrightness[face]));
            }
            return result;
        }

        Vector3 LocalOffset(const Vector3& origin, float heading,
                            float right, float up, float forward)
        {
            return Vector3(origin.X + std::cos(heading) * right +
                               std::sin(heading) * forward,
                           origin.Y + up,
                           origin.Z + std::sin(heading) * right -
                               std::cos(heading) * forward);
        }

        std::array<std::uint8_t, 7> GlyphRows(char character)
        {
            switch (character)
            {
            case 'A': return {14, 17, 17, 31, 17, 17, 17};
            case 'B': return {30, 17, 17, 30, 17, 17, 30};
            case 'C': return {14, 17, 16, 16, 16, 17, 14};
            case 'D': return {30, 17, 17, 17, 17, 17, 30};
            case 'E': return {31, 16, 16, 30, 16, 16, 31};
            case 'F': return {31, 16, 16, 30, 16, 16, 16};
            case 'G': return {14, 17, 16, 23, 17, 17, 15};
            case 'H': return {17, 17, 17, 31, 17, 17, 17};
            case 'I': return {14, 4, 4, 4, 4, 4, 14};
            case 'J': return {7, 2, 2, 2, 18, 18, 12};
            case 'K': return {17, 18, 20, 24, 20, 18, 17};
            case 'L': return {16, 16, 16, 16, 16, 16, 31};
            case 'M': return {17, 27, 21, 21, 17, 17, 17};
            case 'N': return {17, 25, 21, 19, 17, 17, 17};
            case 'O': return {14, 17, 17, 17, 17, 17, 14};
            case 'P': return {30, 17, 17, 30, 16, 16, 16};
            case 'Q': return {14, 17, 17, 17, 21, 18, 13};
            case 'R': return {30, 17, 17, 30, 20, 18, 17};
            case 'S': return {15, 16, 16, 14, 1, 1, 30};
            case 'T': return {31, 4, 4, 4, 4, 4, 4};
            case 'U': return {17, 17, 17, 17, 17, 17, 14};
            case 'V': return {17, 17, 17, 17, 17, 10, 4};
            case 'W': return {17, 17, 17, 21, 21, 21, 10};
            case 'X': return {17, 17, 10, 4, 10, 17, 17};
            case 'Y': return {17, 17, 10, 4, 4, 4, 4};
            case 'Z': return {31, 1, 2, 4, 8, 16, 31};
            case '0': return {14, 17, 19, 21, 25, 17, 14};
            case '1': return {4, 12, 4, 4, 4, 4, 14};
            case '2': return {14, 17, 1, 2, 4, 8, 31};
            case '3': return {30, 1, 1, 14, 1, 1, 30};
            case '4': return {2, 6, 10, 18, 31, 2, 2};
            case '5': return {31, 16, 16, 30, 1, 1, 30};
            case '6': return {14, 16, 16, 30, 17, 17, 14};
            case '7': return {31, 1, 2, 4, 8, 8, 8};
            case '8': return {14, 17, 17, 14, 17, 17, 14};
            case '9': return {14, 17, 17, 15, 1, 1, 14};
            case '-': return {0, 0, 0, 31, 0, 0, 0};
            case '/': return {1, 1, 2, 4, 8, 16, 16};
            case ':': return {0, 4, 4, 0, 4, 4, 0};
            default: return {0, 0, 0, 0, 0, 0, 0};
            }
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

    const std::array<Hazard, 3>& StarfieldGame::hazards() const noexcept
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

    int StarfieldGame::sectorIndex() const noexcept
    {
        return sectorIndex_;
    }

    const Vector3& StarfieldGame::extractionPosition() const noexcept
    {
        return extractionPosition_;
    }

    bool StarfieldGame::frameValid() const noexcept
    {
        return frameValid_;
    }

    void StarfieldGame::Initialize()
    {
        ResetGameplay();
        if (options_.startSector > 0)
        {
            LoadSector(std::clamp(options_.startSector, 0, SectorCount - 1));
            runState_ = RunState::Title;
            score_ = 0;
        }
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

        collectSound_ = LoadSound("collect.wav");
        lossSound_ = LoadSound("loss.wav");
        restartSound_ = LoadSound("restart.wav");
        winSound_ = LoadSound("win.wav");

        const std::string musicPath = options_.contentRoot + "/audio/outer_space.mp3";
        backgroundMusic_.reset(
            xna::Media::Song::FromUri("Outer Space Loop", musicPath));
        xna::Media::MediaPlayer::setVolumeProperty(0.14f);
        xna::Media::MediaPlayer::setIsRepeatingProperty(true);
        xna::Media::MediaPlayer::Play(backgroundMusic_.get());
    }

    void StarfieldGame::UnloadContent()
    {
        xna::Media::MediaPlayer::Stop();
        backgroundMusic_.reset();
        winSound_.reset();
        restartSound_.reset();
        lossSound_.reset();
        collectSound_.reset();
        captureTarget_.reset();
        effect_.reset();
    }

    void StarfieldGame::Update(xna::GameTime& gameTime)
    {
        presentationSeconds_ = static_cast<float>(
            gameTime.getTotalGameTimeProperty().getTotalSecondsProperty());
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
        const bool restartDown = keyboard.IsKeyDown(xna::Input::Keys::R) ||
                                 keyboard.IsKeyDown(xna::Input::Keys::Enter);
        const bool restart = restartDown && !restartHeld_;
        restartHeld_ = restartDown;
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

        device.setBlendStateProperty(xna::Graphics::BlendState::Opaque);
        device.Clear(ThemeForSector(sectorIndex_).sky);
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
        score_ = 0;
        LoadSector(0);
    }

    void StarfieldGame::LoadSector(int sectorIndex)
    {
        sectorIndex_ = std::clamp(sectorIndex, 0, SectorCount - 1);
        elapsedSeconds_ = 0.0f;
        sectorBannerSeconds_ = 2.4f;

        const Hazard inactive{
            Vector3::Zero, Vector3::Zero, Vector3::Zero,
            HazardMotion::Horizontal, 0.0f, 0.0f, false};

        switch (sectorIndex_)
        {
        case 1:
            player_ = Player{Vector3(0.0f, 0.6f, 8.0f), 0.0f};
            extractionPosition_ = Vector3(8.0f, 0.0f, -8.0f);
            collectibles_ = {
                Collectible{Vector3(-7.0f, 0.7f, 6.0f), false},
                Collectible{Vector3(7.0f, 0.7f, -1.0f), false},
                Collectible{Vector3(-5.0f, 0.7f, -6.0f), false}};
            hazards_ = {
                Hazard{Vector3(-2.0f, 0.8f, 0.0f), Vector3(-2.0f, 0.8f, 0.0f),
                       Vector3(0.0f, 0.0f, HazardSpeed), HazardMotion::Vertical,
                       0.0f, 7.0f, true},
                Hazard{Vector3(0.0f, 0.7f, -2.0f), Vector3(0.0f, 0.7f, -2.0f),
                       Vector3(-HazardSpeed, 0.0f, 0.0f), HazardMotion::Horizontal,
                       0.0f, 7.0f, true},
                Hazard{Vector3(4.5f, 0.9f, 2.0f), Vector3(2.0f, 0.9f, 2.0f),
                       Vector3::Zero, HazardMotion::Orbit, 0.0f, 2.5f, true}};
            break;
        case 2:
            player_ = Player{Vector3(0.0f, 0.6f, 8.0f), 0.0f};
            extractionPosition_ = Vector3(-8.0f, 0.0f, -8.0f);
            collectibles_ = {
                Collectible{Vector3(-7.0f, 0.7f, -5.0f), false},
                Collectible{Vector3(0.0f, 0.7f, -6.0f), false},
                Collectible{Vector3(6.0f, 0.7f, 6.0f), false}};
            hazards_ = {
                Hazard{Vector3(0.0f, 0.8f, 4.0f), Vector3(0.0f, 0.8f, 4.0f),
                       Vector3(HazardSpeed + 0.5f, 0.0f, 0.0f),
                       HazardMotion::Horizontal, 0.0f, 8.0f, true},
                Hazard{Vector3(-4.0f, 0.7f, 0.0f), Vector3(-4.0f, 0.7f, 0.0f),
                       Vector3(0.0f, 0.0f, -HazardSpeed - 0.4f),
                       HazardMotion::Vertical, 0.0f, 8.0f, true},
                Hazard{Vector3(6.0f, 1.0f, -3.0f), Vector3(3.0f, 1.0f, -3.0f),
                       Vector3::Zero, HazardMotion::Orbit, 0.0f, 3.0f, true}};
            break;
        default:
            player_ = Player{Vector3(0.0f, 0.6f, 0.0f), 0.0f};
            extractionPosition_ = Vector3(0.0f, 0.0f, -9.0f);
            collectibles_ = {
                Collectible{Vector3(-6.0f, 0.7f, 0.0f), false},
                Collectible{Vector3(0.0f, 0.7f, -5.0f), false},
                Collectible{Vector3(6.0f, 0.7f, 0.0f), false}};
            hazards_ = {
                Hazard{Vector3(0.0f, 0.8f, 3.0f), Vector3(0.0f, 0.8f, 3.0f),
                       Vector3(HazardSpeed, 0.0f, 0.0f),
                       HazardMotion::Horizontal, 0.0f, 7.0f, true},
                Hazard{Vector3(0.0f, 0.6f, -3.5f), Vector3(0.0f, 0.6f, -3.5f),
                       Vector3(-HazardSpeed, 0.0f, 0.0f),
                       HazardMotion::Horizontal, 0.0f, 7.0f, true},
                inactive};
            break;
        }
        UpdateCamera();
    }

    void StarfieldGame::AdvanceGameplay(float seconds, float turn, float throttle,
                                        bool boost, bool restart)
    {
        if (restart)
        {
            PlaySound(restartSound_.get(), 0.42f, 0.08f);
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
        sectorBannerSeconds_ = std::max(0.0f, sectorBannerSeconds_ - dt);
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
            if (hazard.active)
            {
                MoveHazard(hazard, dt);
            }
        }

        bool collectedThisStep = false;
        for (Collectible& collectible : collectibles_)
        {
            if (!collectible.collected &&
                DistanceSquaredXZ(player_.position, collectible.position) <=
                    CellRadius * CellRadius)
            {
                collectible.collected = true;
                score_ += 100;
                collectedThisStep = true;
            }
        }
        const bool hitHazard = std::any_of(
            hazards_.begin(), hazards_.end(),
            [this](const Hazard& hazard)
            {
                return hazard.active &&
                       DistanceSquaredXZ(player_.position, hazard.position) <=
                       HazardCollisionRadius * HazardCollisionRadius;
            });
        if (hitHazard || elapsedSeconds_ >= TimeLimit)
        {
            runState_ = RunState::Lost;
            PlaySound(lossSound_.get(), 0.62f, hitHazard ? -0.18f : -0.35f);
            UpdateCamera();
            return;
        }

        if (collectedCount() == static_cast<int>(collectibles_.size()) &&
            DistanceSquaredXZ(player_.position, extractionPosition_) <=
                ExtractionRadius * ExtractionRadius)
        {
            if (sectorIndex_ + 1 < SectorCount)
            {
                score_ += SectorBonus;
                LoadSector(sectorIndex_ + 1);
                PlaySound(winSound_.get(), 0.48f, -0.08f);
                return;
            }

            runState_ = RunState::Won;
            score_ += FinalMissionBonus;
            PlaySound(winSound_.get(), 0.62f, 0.08f);
        }
        else if (collectedThisStep)
        {
            PlaySound(collectSound_.get(), 0.52f,
                      -0.08f + static_cast<float>(collectedCount()) * 0.08f);
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
        if (hazard.motion == HazardMotion::Orbit)
        {
            hazard.phase = std::remainder(hazard.phase + seconds * 1.35f, 2.0f * Pi);
            hazard.position.X = hazard.origin.X + std::cos(hazard.phase) * hazard.range;
            hazard.position.Z = hazard.origin.Z + std::sin(hazard.phase) * hazard.range;
            return;
        }

        float& coordinate = hazard.motion == HazardMotion::Horizontal
                                ? hazard.position.X : hazard.position.Z;
        float& speed = hazard.motion == HazardMotion::Horizontal
                           ? hazard.velocity.X : hazard.velocity.Z;
        const float center = hazard.motion == HazardMotion::Horizontal
                                 ? hazard.origin.X : hazard.origin.Z;
        coordinate += speed * seconds;
        if (coordinate >= center + hazard.range)
        {
            coordinate = center + hazard.range;
            speed = -std::abs(speed);
        }
        else if (coordinate <= center - hazard.range)
        {
            coordinate = center - hazard.range;
            speed = std::abs(speed);
        }
    }

    void StarfieldGame::DrawWorld()
    {
        auto& device = getGraphicsDeviceProperty();
        device.setDepthStencilStateProperty(xna::Graphics::DepthStencilState::Default);
        device.setBlendStateProperty(xna::Graphics::BlendState::Opaque);

        effect_->World = Matrix::getIdentityProperty();
        effect_->View = Matrix::CreateLookAt(cameraPosition_, cameraTarget_, Vector3::Up);
        effect_->Projection = Matrix::CreatePerspectiveFieldOfView(
            Pi / 4.0f,
            static_cast<float>(ReferenceWidth) / static_cast<float>(ReferenceHeight),
            0.1f, 100.0f);
        auto& pass = effect_->getTechniquesProperty()[0].getPassesProperty()[0];

        DrawStarfield(device, pass);
        DrawArena(device, pass);
        DrawPlayerCraft(device, pass);

        for (std::size_t index = 0; index < collectibles_.size(); ++index)
        {
            if (!collectibles_[index].collected)
            {
                DrawCollectible(device, pass, collectibles_[index], index);
            }
        }

        for (std::size_t index = 0; index < hazards_.size(); ++index)
        {
            if (hazards_[index].active)
            {
                DrawHazard(device, pass, hazards_[index], index);
            }
        }
        DrawExtractionGate(device, pass);
    }

    void StarfieldGame::DrawStarfield(xna::Graphics::GraphicsDevice& device,
                                      xna::Graphics::EffectPass& pass)
    {
        const SectorTheme theme = ThemeForSector(sectorIndex_);
        for (int index = 0; index < 52; ++index)
        {
            const float angle = static_cast<float>(index) * 2.3999632f;
            const float radius = 25.0f + static_cast<float>((index * 7) % 11);
            const float height = 3.0f + static_cast<float>((index * 13) % 17);
            const float size = 0.05f + static_cast<float>(index % 4) * 0.025f;
            const Color color = index % 7 == 0 ? theme.accent
                                                : Color(190, 210, 235);
            DrawCube(device, pass,
                     Vector3(std::cos(angle) * radius, height,
                             std::sin(angle) * radius),
                     Vector3(size, size, size), color);
        }
    }

    void StarfieldGame::DrawArena(xna::Graphics::GraphicsDevice& device,
                                  xna::Graphics::EffectPass& pass)
    {
        const SectorTheme theme = ThemeForSector(sectorIndex_);
        DrawCube(device, pass, Vector3(0.0f, -0.38f, 0.0f),
                 Vector3(21.0f, 0.6f, 21.0f), Shade(theme.floor, 0.38f));
        DrawCube(device, pass, Vector3(0.0f, -0.055f, 0.0f),
                 Vector3(20.0f, 0.08f, 20.0f), theme.floor);

        for (int offset = -10; offset <= 10; offset += 2)
        {
            DrawCube(device, pass, Vector3(static_cast<float>(offset), 0.005f, 0.0f),
                     Vector3(0.028f, 0.025f, 20.0f), theme.grid);
            DrawCube(device, pass, Vector3(0.0f, 0.005f, static_cast<float>(offset)),
                     Vector3(20.0f, 0.025f, 0.028f), theme.grid);
        }

        DrawCube(device, pass, Vector3(0.0f, 0.022f, 0.0f),
                 Vector3(0.06f, 0.035f, 20.0f), Shade(theme.accent, 0.72f));
        DrawCube(device, pass,
                 Vector3(extractionPosition_.X, 0.024f, extractionPosition_.Z),
                 Vector3(3.8f, 0.04f, 1.7f), Color(15, 68, 64));

        if (sectorIndex_ == 0)
        {
            for (const float laneX : {-1.35f, 1.35f})
            {
                DrawCube(device, pass, Vector3(laneX, 0.05f, -7.0f),
                         Vector3(0.06f, 0.05f, 5.4f), Color(35, 155, 110));
            }
        }
        else if (sectorIndex_ == 1)
        {
            for (const float diagonal : {-5.0f, 0.0f, 5.0f})
            {
                DrawCube(device, pass, Vector3(diagonal, 0.035f, diagonal),
                         Vector3(0.1f, 0.035f, 13.5f),
                         Shade(theme.accent, 0.7f), -Pi / 4.0f);
            }
            for (const float side : {-1.0f, 1.0f})
            {
                DrawCube(device, pass, Vector3(side * 11.2f, 2.0f, -6.0f),
                         Vector3(0.7f, 4.0f, 0.7f), theme.accent,
                         side * 0.25f);
                DrawCube(device, pass, Vector3(side * 11.4f, 1.2f, 5.5f),
                         Vector3(1.0f, 2.4f, 1.0f), theme.boundary,
                         -side * 0.35f);
            }
        }
        else
        {
            for (const float channelX : {-6.0f, -2.0f, 2.0f, 6.0f})
            {
                DrawCube(device, pass, Vector3(channelX, 0.035f, 0.0f),
                         Vector3(0.34f, 0.035f, 19.6f),
                         channelX == -2.0f || channelX == 2.0f
                             ? theme.accent : Shade(theme.boundary, 0.7f));
            }
            for (const float side : {-1.0f, 1.0f})
            {
                for (const float z : {-6.0f, 0.0f, 6.0f})
                {
                    DrawCube(device, pass, Vector3(side * 11.35f, 1.45f, z),
                             Vector3(1.2f, 2.9f, 1.2f), Color(75, 20, 18));
                    DrawCube(device, pass, Vector3(side * 11.35f, 3.0f, z),
                             Vector3(0.75f, 0.25f, 0.75f), theme.boundary);
                }
            }
        }

        DrawCube(device, pass, Vector3(-10.1f, 0.18f, 0.0f),
                 Vector3(0.22f, 0.36f, 20.4f), theme.boundary);
        DrawCube(device, pass, Vector3(10.1f, 0.18f, 0.0f),
                 Vector3(0.22f, 0.36f, 20.4f), theme.boundary);
        DrawCube(device, pass, Vector3(0.0f, 0.18f, -10.1f),
                 Vector3(20.4f, 0.36f, 0.22f), theme.boundary);
        DrawCube(device, pass, Vector3(0.0f, 0.18f, 10.1f),
                 Vector3(20.4f, 0.36f, 0.22f), theme.boundary);
        for (const float edge : {-10.0f, 10.0f})
        {
            for (const float offset : {-10.0f, -5.0f, 0.0f, 5.0f, 10.0f})
            {
                DrawCube(device, pass, Vector3(edge, 0.8f, offset),
                         Vector3(0.32f, 1.6f, 0.32f), Shade(theme.boundary, 0.38f));
                DrawCube(device, pass, Vector3(edge, 1.62f, offset),
                         Vector3(0.46f, 0.12f, 0.46f), theme.accent);
                DrawCube(device, pass, Vector3(offset, 0.8f, edge),
                         Vector3(0.32f, 1.6f, 0.32f), Shade(theme.boundary, 0.38f));
                DrawCube(device, pass, Vector3(offset, 1.62f, edge),
                         Vector3(0.46f, 0.12f, 0.46f), theme.accent);
            }
        }
    }

    void StarfieldGame::DrawPlayerCraft(xna::Graphics::GraphicsDevice& device,
                                        xna::Graphics::EffectPass& pass)
    {
        const float yaw = -player_.heading;
        DrawCube(device, pass, player_.position, Vector3(1.2f, 0.6f, 1.8f),
                 Color(0, 175, 225), yaw);
        DrawCube(device, pass, LocalOffset(player_.position, player_.heading,
                                           0.0f, 0.34f, 0.2f),
                 Vector3(0.62f, 0.28f, 0.8f), Color(25, 60, 105), yaw);
        DrawCube(device, pass, LocalOffset(player_.position, player_.heading,
                                           0.0f, 0.02f, 1.05f),
                 Vector3(0.52f, 0.32f, 0.65f), Color(100, 245, 255), yaw);
        for (const float side : {-1.0f, 1.0f})
        {
            DrawCube(device, pass, LocalOffset(player_.position, player_.heading,
                                               side * 0.86f, -0.08f, -0.05f),
                     Vector3(0.95f, 0.14f, 0.9f), Color(0, 115, 175), yaw);
            DrawCube(device, pass, LocalOffset(player_.position, player_.heading,
                                               side * 0.48f, -0.02f, -0.93f),
                     Vector3(0.28f, 0.32f, 0.48f), Color(25, 65, 110), yaw);
            DrawCube(device, pass, LocalOffset(player_.position, player_.heading,
                                               side * 0.48f, -0.02f, -1.2f),
                     Vector3(0.17f, 0.2f, 0.25f), Color(80, 220, 255), yaw);
        }
    }

    void StarfieldGame::DrawCollectible(xna::Graphics::GraphicsDevice& device,
                                        xna::Graphics::EffectPass& pass,
                                        const Collectible& collectible,
                                        std::size_t index)
    {
        const float phase = presentationSeconds_ * 1.8f + static_cast<float>(index) * 1.7f;
        const float pulse = 1.0f + std::sin(phase) * 0.08f;
        const float yaw = phase * 0.7f;
        const Vector3& center = collectible.position;
        DrawCube(device, pass, Vector3(center.X, 0.14f, center.Z),
                 Vector3(1.2f, 0.18f, 1.2f), Color(75, 55, 12), yaw);
        DrawCube(device, pass, center, Vector3(0.52f * pulse, 1.2f, 0.52f * pulse),
                 Color(255, 210, 0), yaw);
        DrawCube(device, pass, center, Vector3(1.05f, 0.09f, 0.16f),
                 Color(255, 242, 125), yaw);
        DrawCube(device, pass, center, Vector3(0.16f, 0.09f, 1.05f),
                 Color(255, 242, 125), yaw);
        DrawCube(device, pass, Vector3(center.X, center.Y + 0.7f, center.Z),
                 Vector3(0.7f, 0.1f, 0.7f), Color(255, 170, 0), -yaw);
        DrawCube(device, pass, Vector3(center.X, center.Y - 0.7f, center.Z),
                 Vector3(0.7f, 0.1f, 0.7f), Color(255, 170, 0), -yaw);
    }

    void StarfieldGame::DrawHazard(xna::Graphics::GraphicsDevice& device,
                                   xna::Graphics::EffectPass& pass,
                                   const Hazard& hazard, std::size_t index)
    {
        const float direction = index == 0 ? 1.0f : -1.0f;
        const float spin = presentationSeconds_ * 1.6f * direction;
        const float pulse = 1.0f + 0.1f * std::sin(presentationSeconds_ * 4.0f +
                                                  static_cast<float>(index));
        if (hazard.motion == HazardMotion::Horizontal)
        {
            const Color red = index == 0 ? Color(230, 40, 50) : Color(255, 55, 70);
            DrawCube(device, pass, hazard.position,
                     Vector3(1.15f * pulse, 1.05f, 1.15f * pulse), red, spin);
            DrawCube(device, pass, hazard.position, Vector3(2.2f, 0.2f, 0.34f),
                     Color(135, 20, 35), spin);
            DrawCube(device, pass, hazard.position, Vector3(0.34f, 0.2f, 2.2f),
                     Color(135, 20, 35), spin);
        }
        else if (hazard.motion == HazardMotion::Vertical)
        {
            DrawCube(device, pass, hazard.position,
                     Vector3(0.9f * pulse, 0.9f, 1.45f * pulse),
                     Color(205, 45, 235), -spin * 0.35f);
            DrawCube(device, pass, hazard.position, Vector3(0.3f, 0.18f, 3.0f),
                     Color(100, 20, 140), -spin * 0.35f);
            DrawCube(device, pass, hazard.position, Vector3(1.65f, 0.16f, 0.25f),
                     Color(255, 105, 225), -spin * 0.35f);
        }
        else
        {
            DrawCube(device, pass, hazard.position,
                     Vector3(1.2f * pulse, 1.2f, 1.2f * pulse),
                     Color(255, 105, 25), spin + Pi / 4.0f);
            DrawCube(device, pass, hazard.position, Vector3(2.55f, 0.18f, 0.28f),
                     Color(255, 185, 35), spin);
            DrawCube(device, pass, hazard.position, Vector3(0.28f, 0.18f, 2.55f),
                     Color(170, 45, 20), spin);
        }
        DrawCube(device, pass,
                 Vector3(hazard.position.X, hazard.position.Y + 0.62f,
                         hazard.position.Z),
                 Vector3(0.42f, 0.22f, 0.42f), Color(255, 150, 35), -spin);
        DrawCube(device, pass,
                 Vector3(hazard.position.X, hazard.position.Y - 0.62f,
                         hazard.position.Z),
                 Vector3(0.42f, 0.22f, 0.42f), Color(255, 90, 25), -spin);
    }

    void StarfieldGame::DrawExtractionGate(xna::Graphics::GraphicsDevice& device,
                                           xna::Graphics::EffectPass& pass)
    {
        const bool active = collectedCount() == static_cast<int>(collectibles_.size());
        const float pulse = 0.5f + 0.5f * std::sin(presentationSeconds_ * 4.0f);
        const Color gateColor = collectedCount() == static_cast<int>(collectibles_.size())
                                    ? Color(40, 255, 80)
                                    : Color(20, 100, 50);
        const Color trimColor = active
                                    ? Color(145, 255, static_cast<int>(150.0f + pulse * 90.0f))
                                    : Color(35, 135, 85);
        const float gateX = extractionPosition_.X;
        const float gateZ = extractionPosition_.Z;
        DrawCube(device, pass, Vector3(gateX, 0.05f, gateZ),
                 Vector3(3.7f, 0.1f, 1.3f), Color(12, 70, 58));
        for (const float side : {-1.45f, 1.45f})
        {
            DrawCube(device, pass, Vector3(gateX + side, 1.05f, gateZ),
                     Vector3(0.42f, 2.1f, 0.62f), Color(10, 58, 52));
            DrawCube(device, pass, Vector3(gateX + side, 1.15f, gateZ),
                     Vector3(0.2f, 1.65f, 0.7f), gateColor);
            DrawCube(device, pass, Vector3(gateX + side, 2.18f, gateZ),
                     Vector3(0.56f, 0.18f, 0.78f), trimColor);
        }
        DrawCube(device, pass, Vector3(gateX, 2.25f, gateZ),
                 Vector3(3.3f, 0.35f, 0.62f), Color(10, 58, 52));
        DrawCube(device, pass, Vector3(gateX, 2.23f, gateZ),
                 Vector3(2.72f, 0.16f, 0.72f), gateColor);
        DrawCube(device, pass, Vector3(gateX, 0.12f, gateZ),
                 Vector3(2.25f, 0.08f, 0.7f), trimColor);
    }

    void StarfieldGame::DrawHud()
    {
        auto& device = getGraphicsDeviceProperty();
        const SectorTheme theme = ThemeForSector(sectorIndex_);
        device.setDepthStencilStateProperty(xna::Graphics::DepthStencilState::None);
        effect_->View = Matrix::getIdentityProperty();
        effect_->Projection = Matrix::CreateOrthographicOffCenter(
            0.0f, static_cast<float>(ReferenceWidth),
            static_cast<float>(ReferenceHeight), 0.0f, 0.0f, 10.0f);
        auto& pass = effect_->getTechniquesProperty()[0].getPassesProperty()[0];
        device.setBlendStateProperty(xna::Graphics::BlendState::AlphaBlend);

        DrawHudRect(device, pass, 22.0f, 18.0f, 435.0f, 88.0f,
                    Color(5, 12, 30, 225));
        DrawHudRect(device, pass, 22.0f, 18.0f, 435.0f, 3.0f,
                    theme.accent);
        DrawText(device, pass, "CELLS", 36.0f, 29.0f, 2.0f, Color(130, 205, 255));
        for (std::size_t index = 0; index < collectibles_.size(); ++index)
        {
            const float x = 36.0f + static_cast<float>(index) * 38.0f;
            DrawHudRect(device, pass, x, 60.0f, 27.0f, 27.0f,
                        collectibles_[index].collected ? Color(255, 215, 0)
                                                       : Color(35, 52, 78));
            DrawHudRect(device, pass, x + 6.0f, 66.0f, 15.0f, 15.0f,
                        collectibles_[index].collected ? Color(255, 245, 145)
                                                       : Color(13, 25, 48));
        }

        const float remaining = std::max(0.0f, TimeLimit - elapsedSeconds_) / TimeLimit;
        DrawText(device, pass, "TIME", 184.0f, 29.0f, 2.0f, Color(130, 205, 255));
        DrawHudRect(device, pass, 184.0f, 66.0f, 244.0f, 14.0f,
                    Color(24, 38, 63));
        if (remaining > 0.0f)
        {
            DrawHudRect(device, pass, 187.0f, 69.0f, remaining * 238.0f, 8.0f,
                        remaining > 0.25f ? Color(60, 230, 130)
                                          : Color(255, 80, 50));
        }

        const Color stateColor = runState_ == RunState::Won ? Color(40, 255, 80)
                                : runState_ == RunState::Lost ? Color(255, 50, 50)
                                                             : Color(0, 200, 255);
        const std::string stateText = runState_ == RunState::Won ? "MISSION COMPLETE"
                                    : runState_ == RunState::Lost ? "MISSION FAILED"
                                    : "SECTOR " + std::to_string(sectorIndex_ + 1) +
                                          " " + SectorName(sectorIndex_);
        DrawHudRect(device, pass, 474.0f, 18.0f, 500.0f, 56.0f,
                    Color(5, 12, 30, 210));
        DrawHudRect(device, pass, 474.0f, 18.0f, 4.0f, 56.0f, stateColor);
        DrawText(device, pass, stateText, 506.0f, 36.0f, 2.0f, stateColor);

        DrawHudRect(device, pass, 996.0f, 18.0f, 262.0f, 88.0f,
                    Color(5, 12, 30, 225));
        DrawHudRect(device, pass, 996.0f, 18.0f, 262.0f, 3.0f,
                    theme.accent);
        DrawText(device, pass, "SCORE", 1011.0f, 29.0f, 2.0f,
                 Color(130, 205, 255));

        int displayScore = score_;
        for (int digitIndex = 3; digitIndex >= 0; --digitIndex)
        {
            DrawDigit(device, pass, displayScore % 10,
                      1230.0f - static_cast<float>(3 - digitIndex) * 34.0f,
                      68.0f, Color(100, 245, 255));
            displayScore /= 10;
        }

        DrawHudRect(device, pass, 22.0f, 682.0f, 610.0f, 25.0f,
                    Color(5, 12, 30, 205));
        DrawText(device, pass, "WASD MOVE  R RESTART  ESC QUIT",
                 34.0f, 688.0f, 1.5f, Color(115, 165, 205));
        const std::string sectorProgress = "SECTOR " +
                                           std::to_string(sectorIndex_ + 1) + "/" +
                                           std::to_string(SectorCount);
        DrawHudRect(device, pass, 1035.0f, 682.0f, 223.0f, 25.0f,
                    Color(5, 12, 30, 205));
        DrawText(device, pass, sectorProgress, 1062.0f, 688.0f, 1.5f,
                 theme.accent);

        if (sectorBannerSeconds_ > 0.0f &&
            runState_ != RunState::Won && runState_ != RunState::Lost)
        {
            const std::string sectorTitle = "SECTOR " +
                                            std::to_string(sectorIndex_ + 1);
            const std::string sectorName = SectorName(sectorIndex_);
            const auto centeredX = [](std::size_t length, float pixelSize)
            {
                return (static_cast<float>(ReferenceWidth) -
                        (static_cast<float>(length) * 6.0f - 1.0f) * pixelSize) * 0.5f;
            };
            DrawHudRect(device, pass, 350.0f, 122.0f, 580.0f, 122.0f,
                        Color(3, 8, 24, 145));
            DrawHudRect(device, pass, 350.0f, 122.0f, 580.0f, 4.0f,
                        theme.accent);
            DrawHudRect(device, pass, 350.0f, 240.0f, 580.0f, 4.0f,
                        theme.accent);
            DrawText(device, pass, sectorTitle,
                     centeredX(sectorTitle.size(), 3.0f), 145.0f, 3.0f,
                     Color(225, 240, 255));
            DrawText(device, pass, sectorName,
                     centeredX(sectorName.size(), 4.0f), 192.0f, 4.0f,
                     theme.accent);
        }

        if (runState_ == RunState::Won || runState_ == RunState::Lost)
        {
            const float pulse = 0.5f + 0.5f * std::sin(presentationSeconds_ * 4.0f);
            const Color terminalColor = runState_ == RunState::Won
                                            ? Color(40, static_cast<int>(205.0f + 50.0f * pulse), 90)
                                            : Color(255, static_cast<int>(45.0f + 55.0f * pulse), 55);
            DrawHudRect(device, pass, 145.0f, 205.0f, 990.0f, 305.0f,
                        Color(3, 8, 24, 238));
            DrawHudRect(device, pass, 145.0f, 205.0f, 990.0f, 6.0f, terminalColor);
            DrawHudRect(device, pass, 145.0f, 504.0f, 990.0f, 6.0f, terminalColor);
            DrawHudRect(device, pass, 145.0f, 205.0f, 6.0f, 305.0f, terminalColor);
            DrawHudRect(device, pass, 1129.0f, 205.0f, 6.0f, 305.0f, terminalColor);

            const auto centeredX = [](std::size_t length, float pixelSize)
            {
                return (static_cast<float>(ReferenceWidth) -
                        (static_cast<float>(length) * 6.0f - 1.0f) * pixelSize) * 0.5f;
            };
            const std::string headline = runState_ == RunState::Won
                                             ? "MISSION COMPLETE"
                                             : "MISSION FAILED";
            const std::string detail = runState_ == RunState::Won
                                           ? "ALL 3 SECTORS CLEARED"
                                           : "HAZARD IMPACT OR TIME LIMIT";
            DrawText(device, pass, headline, centeredX(headline.size(), 5.0f),
                     258.0f, 5.0f, terminalColor);
            DrawText(device, pass, detail, centeredX(detail.size(), 2.5f),
                     352.0f, 2.5f, Color(175, 205, 225));
            const std::string prompt = "PRESS R OR ENTER TO RESTART";
            DrawText(device, pass, prompt, centeredX(prompt.size(), 3.0f),
                     423.0f, 3.0f, Color(235, 245, 255));
        }

        device.setBlendStateProperty(xna::Graphics::BlendState::Opaque);
    }

    void StarfieldGame::DrawHudRect(xna::Graphics::GraphicsDevice& device,
                                    xna::Graphics::EffectPass& pass,
                                    float x, float y, float width, float height,
                                    const Color& color)
    {
        DrawCube(device, pass, Vector3(x + width * 0.5f, y + height * 0.5f, 0.0f),
                 Vector3(width, height, 0.1f), color);
    }

    void StarfieldGame::DrawText(xna::Graphics::GraphicsDevice& device,
                                 xna::Graphics::EffectPass& pass,
                                 const std::string& text, float x, float y,
                                 float pixelSize, const Color& color)
    {
        for (std::size_t characterIndex = 0; characterIndex < text.size(); ++characterIndex)
        {
            const auto rows = GlyphRows(text[characterIndex]);
            for (std::size_t row = 0; row < rows.size(); ++row)
            {
                for (int column = 0; column < 5; ++column)
                {
                    if ((rows[row] & (1U << (4 - column))) != 0)
                    {
                        DrawHudRect(device, pass,
                                    x + (static_cast<float>(characterIndex) * 6.0f +
                                         static_cast<float>(column)) * pixelSize,
                                    y + static_cast<float>(row) * pixelSize,
                                    pixelSize, pixelSize, color);
                    }
                }
            }
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

    std::unique_ptr<xna::Audio::SoundEffect>
    StarfieldGame::LoadSound(const std::string& fileName) const
    {
        const std::string path = options_.contentRoot + "/audio/" + fileName;
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
        {
            throw std::runtime_error("cannot open audio asset: " + path);
        }
        return std::unique_ptr<xna::Audio::SoundEffect>(
            xna::Audio::SoundEffect::FromStream(stream));
    }

    void StarfieldGame::PlaySound(xna::Audio::SoundEffect* sound,
                                  float volume, float pitch, float pan) noexcept
    {
        if (sound == nullptr)
        {
            return;
        }
        try
        {
            (void)sound->Play(volume, pitch, pan);
        }
        catch (...)
        {
            // A transient output-device failure must not break gameplay after
            // the content itself has already loaded successfully.
        }
    }

    void StarfieldGame::CaptureFrame()
    {
        const SectorTheme theme = ThemeForSector(sectorIndex_);
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
        std::size_t gatePixels = 0;
        std::size_t hudPixels = 0;
        const Color clear = theme.sky;
        for (std::size_t pixelIndex = 0; pixelIndex < pixels.size(); ++pixelIndex)
        {
            const Color& pixel = pixels[pixelIndex];
            const int red = pixel.getRProperty();
            const int green = pixel.getGProperty();
            const int blue = pixel.getBProperty();
            nonClearPixels += pixel != clear;
            floorPixels += pixel == theme.floor;
            gridPixels += pixel == theme.grid;
            boundaryPixels += pixel == theme.boundary;
            playerPixels += blue > 160 && green > 100 && red < 120;
            collectiblePixels += red > 210 && green > 135 && blue < 130;
            hazardPixels += red > 175 && green < 100 && blue < 110;
            gatePixels += green > 50 && green > red * 2 && green > blue;
            hudPixels += pixelIndex < static_cast<std::size_t>(width) * 115U &&
                         pixel != clear;
        }

        frameValid_ = width == ReferenceWidth && height == ReferenceHeight &&
                      nonClearPixels > pixels.size() / 100 &&
                      floorPixels > pixels.size() / 10 && gridPixels > 2500 &&
                      boundaryPixels > 3000 && playerPixels > 500 &&
                      collectiblePixels > 500 && hazardPixels > 500 &&
                      gatePixels > 300 && hudPixels > 5000;
        if (options_.validateFrame && !frameValid_)
        {
            throw std::runtime_error(
                "captured sector " + std::to_string(sectorIndex_ + 1) +
                " frame is incomplete: non-clear=" + std::to_string(nonClearPixels) +
                " floor=" + std::to_string(floorPixels) +
                " grid=" + std::to_string(gridPixels) +
                " boundary=" + std::to_string(boundaryPixels) +
                " player=" + std::to_string(playerPixels) +
                " collectible=" + std::to_string(collectiblePixels) +
                " hazard=" + std::to_string(hazardPixels) +
                " gate=" + std::to_string(gatePixels) +
                " hud=" + std::to_string(hudPixels));
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
