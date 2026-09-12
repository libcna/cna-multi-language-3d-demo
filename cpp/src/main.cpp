#include "StarfieldGame.hpp"

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string_view>
#include <utility>

namespace
{
    bool ParsePositive(const char* text, int& value)
    {
        if (text == nullptr || *text == '\0')
        {
            return false;
        }

        char* end = nullptr;
        errno = 0;
        const long parsed = std::strtol(text, &end, 10);
        if (errno != 0 || *end != '\0' || parsed <= 0 ||
            parsed > std::numeric_limits<int>::max())
        {
            return false;
        }

        value = static_cast<int>(parsed);
        return true;
    }

    bool ParseOptions(int argc, char** argv, starfield::RuntimeOptions& options)
    {
        for (int index = 1; index < argc; ++index)
        {
            const std::string_view argument(argv[index]);
            if (argument == "--smoke-frames" && index + 1 < argc)
            {
                if (!ParsePositive(argv[++index], options.smokeFrames))
                {
                    return false;
                }
            }
            else if (argument == "--screenshot" && index + 1 < argc)
            {
                options.screenshotPath = argv[++index];
            }
            else if (argument == "--validate-frame")
            {
                options.validateFrame = true;
            }
            else if (argument == "--start-sector" && index + 1 < argc)
            {
                int sector = 0;
                if (!ParsePositive(argv[++index], sector) ||
                    sector > starfield::SectorCount)
                {
                    return false;
                }
                options.startSector = sector - 1;
            }
            else
            {
                return false;
            }
        }

        if (options.smokeFrames < 0 &&
            (!options.screenshotPath.empty() || options.validateFrame))
        {
            options.smokeFrames = 1;
        }
        return true;
    }

    const char* StateName(starfield::RunState state)
    {
        switch (state)
        {
        case starfield::RunState::Title:
            return "title";
        case starfield::RunState::Playing:
            return "playing";
        case starfield::RunState::Won:
            return "won";
        case starfield::RunState::Lost:
            return "lost";
        }
        return "unknown";
    }
}

int main(int argc, char** argv)
{
    starfield::RuntimeOptions options;
    if (!ParseOptions(argc, argv, options))
    {
        std::cerr << "usage: starfield_cpp [--smoke-frames COUNT] "
                     "[--validate-frame] [--screenshot FILE.ppm] "
                     "[--start-sector 1|2|3]\n";
        return 2;
    }
    options.contentRoot =
        (std::filesystem::absolute(argv[0]).parent_path() / "Content").string();

    starfield::StarfieldGame game(std::move(options));
    game.Run();

    std::cout << "starfield-cpp state=" << StateName(game.runState())
              << " collected=" << game.collectedCount()
              << " score=" << game.score()
              << " sector=" << game.sectorIndex() + 1
              << " frame_valid=" << (game.frameValid() ? "true" : "false")
              << " player_x=" << game.player().position.X
              << " player_z=" << game.player().position.Z
              << " heading=" << game.player().heading
              << " hazard0_x=" << game.hazards()[0].position.X
              << " hazard0_z=" << game.hazards()[0].position.Z
              << " hazard1_x=" << game.hazards()[1].position.X
              << " hazard1_z=" << game.hazards()[1].position.Z
              << " hazard2_active=" << (game.hazards()[2].active ? "true" : "false")
              << " hazard2_x=" << game.hazards()[2].position.X
              << " hazard2_z=" << game.hazards()[2].position.Z << '\n';
    return game.frameValid() ? 0 : 1;
}
