#pragma once

#include <array>
#include <cstddef>
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

    namespace Audio
    {
        class SoundEffect;
    }

    namespace Media
    {
        class Song;
    }
}

namespace starfield
{
    inline constexpr int ReferenceWidth = 1280;
    inline constexpr int ReferenceHeight = 720;
    inline constexpr int SectorCount = 3;

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

    enum class HazardMotion : std::uint8_t
    {
        Horizontal,
        Vertical,
        Orbit
    };

    struct Hazard
    {
        Microsoft::Xna::Framework::Vector3 position;
        Microsoft::Xna::Framework::Vector3 origin;
        Microsoft::Xna::Framework::Vector3 velocity;
        HazardMotion motion = HazardMotion::Horizontal;
        float phase = 0.0f;
        float range = 0.0f;
        bool active = false;
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
        std::string contentRoot;
        int startSector = 0;
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
        [[nodiscard]] const std::array<Hazard, 3>& hazards() const noexcept;
        [[nodiscard]] const std::array<Collectible, 3>& collectibles() const noexcept;
        [[nodiscard]] const Microsoft::Xna::Framework::Vector3& cameraPosition() const noexcept;
        [[nodiscard]] const Microsoft::Xna::Framework::Vector3& cameraTarget() const noexcept;
        [[nodiscard]] float elapsedSeconds() const noexcept;
        [[nodiscard]] int score() const noexcept;
        [[nodiscard]] int collectedCount() const noexcept;
        [[nodiscard]] int sectorIndex() const noexcept;
        [[nodiscard]] const Microsoft::Xna::Framework::Vector3&
            extractionPosition() const noexcept;
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
        void LoadSector(int sectorIndex);
        void AdvanceGameplay(float seconds, float turn, float throttle, bool boost, bool restart);
        void UpdateCamera();
        void MoveHazard(Hazard& hazard, float seconds);
        void DrawWorld();
        void DrawStarfield(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                           Microsoft::Xna::Framework::Graphics::EffectPass& pass);
        void DrawArena(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       Microsoft::Xna::Framework::Graphics::EffectPass& pass);
        void DrawPlayerCraft(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                             Microsoft::Xna::Framework::Graphics::EffectPass& pass);
        void DrawCollectible(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                             Microsoft::Xna::Framework::Graphics::EffectPass& pass,
                             const Collectible& collectible, std::size_t index);
        void DrawHazard(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                        Microsoft::Xna::Framework::Graphics::EffectPass& pass,
                        const Hazard& hazard, std::size_t index);
        void DrawExtractionGate(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                Microsoft::Xna::Framework::Graphics::EffectPass& pass);
        void DrawHud();
        void DrawHudRect(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                         Microsoft::Xna::Framework::Graphics::EffectPass& pass,
                         float x, float y, float width, float height,
                         const Microsoft::Xna::Framework::Color& color);
        void DrawText(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                      Microsoft::Xna::Framework::Graphics::EffectPass& pass,
                      const std::string& text, float x, float y, float pixelSize,
                      const Microsoft::Xna::Framework::Color& color);
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
        [[nodiscard]] std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect>
            LoadSound(const std::string& fileName) const;
        void PlaySound(Microsoft::Xna::Framework::Audio::SoundEffect* sound,
                       float volume, float pitch = 0.0f, float pan = 0.0f) noexcept;
        void CaptureFrame();

        RuntimeOptions options_;
        RunState runState_ = RunState::Title;
        Player player_{};
        std::array<Hazard, 3> hazards_{};
        std::array<Collectible, 3> collectibles_{};
        Microsoft::Xna::Framework::Vector3 cameraPosition_;
        Microsoft::Xna::Framework::Vector3 cameraTarget_;
        Microsoft::Xna::Framework::Vector3 extractionPosition_;
        float elapsedSeconds_ = 0.0f;
        int score_ = 0;
        int sectorIndex_ = 0;
        int drawnFrames_ = 0;
        float presentationSeconds_ = 0.0f;
        float sectorBannerSeconds_ = 0.0f;
        bool restartHeld_ = false;
        bool captured_ = false;
        bool frameValid_ = true;
        std::unique_ptr<Microsoft::Xna::Framework::GraphicsDeviceManager> graphics_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::RenderTarget2D> captureTarget_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> collectSound_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> lossSound_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> restartSound_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> winSound_;
        std::unique_ptr<Microsoft::Xna::Framework::Media::Song> backgroundMusic_;
    };
}
