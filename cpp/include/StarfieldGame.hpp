#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace Microsoft::Xna::Framework
{
    class GraphicsDeviceManager;

    namespace Graphics
    {
        class BasicEffect;
        class EffectPass;
        class GraphicsDevice;
        class RenderTarget2D;
    }
}

namespace starfield
{
    inline constexpr int ReferenceWidth = 1280;
    inline constexpr int ReferenceHeight = 720;

    enum class RunState : std::uint8_t
    {
        Title,
        Playing,
        Won,
        Lost
    };

    struct Player
    {
        Microsoft::Xna::Framework::Vector3 position;
        float heading = 0.0f;
    };

    struct Hazard
    {
        Microsoft::Xna::Framework::Vector3 position;
        float velocityX = 0.0f;
    };

    struct Collectible
    {
        Microsoft::Xna::Framework::Vector3 position;
        bool collected = false;
    };

    struct RuntimeOptions
    {
        int smokeFrames = -1;
        std::string screenshotPath;
        bool validateFrame = false;
    };

    class StarfieldGameTestAccess;

    class StarfieldGame final : public Microsoft::Xna::Framework::Game
    {
    public:
        explicit StarfieldGame(RuntimeOptions options = {});
        ~StarfieldGame() override;

        [[nodiscard]] RunState runState() const noexcept;
        [[nodiscard]] const Player& player() const noexcept;
        [[nodiscard]] const std::array<Hazard, 2>& hazards() const noexcept;
        [[nodiscard]] const std::array<Collectible, 3>& collectibles() const noexcept;
        [[nodiscard]] const Microsoft::Xna::Framework::Vector3& cameraPosition() const noexcept;
        [[nodiscard]] const Microsoft::Xna::Framework::Vector3& cameraTarget() const noexcept;
        [[nodiscard]] float elapsedSeconds() const noexcept;
        [[nodiscard]] int score() const noexcept;
        [[nodiscard]] int collectedCount() const noexcept;
        [[nodiscard]] bool frameValid() const noexcept;

    protected:
        void Initialize() override;
        void LoadContent() override;
        void UnloadContent() override;
        void Update(Microsoft::Xna::Framework::GameTime& gameTime) override;
        void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override;

    private:
        friend class StarfieldGameTestAccess;

        void ResetGameplay();
        void AdvanceGameplay(float seconds, float turn, float throttle, bool boost, bool restart);
        void UpdateCamera();
        void MoveHazard(Hazard& hazard, float seconds);
        void DrawWorld();
        void DrawHud();
        void DrawCube(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                      Microsoft::Xna::Framework::Graphics::EffectPass& pass,
                      const Microsoft::Xna::Framework::Vector3& position,
                      const Microsoft::Xna::Framework::Vector3& scale,
                      const Microsoft::Xna::Framework::Color& color,
                      float yaw = 0.0f);
        void DrawDigit(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       Microsoft::Xna::Framework::Graphics::EffectPass& pass,
                       int digit, float x, float y,
                       const Microsoft::Xna::Framework::Color& color);
        void CaptureFrame();

        RuntimeOptions options_;
        RunState runState_ = RunState::Title;
        Player player_{};
        std::array<Hazard, 2> hazards_{};
        std::array<Collectible, 3> collectibles_{};
        Microsoft::Xna::Framework::Vector3 cameraPosition_;
        Microsoft::Xna::Framework::Vector3 cameraTarget_;
        float elapsedSeconds_ = 0.0f;
        int score_ = 0;
        int drawnFrames_ = 0;
        bool captured_ = false;
        bool frameValid_ = true;
        std::unique_ptr<Microsoft::Xna::Framework::GraphicsDeviceManager> graphics_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::RenderTarget2D> captureTarget_;
    };
}
