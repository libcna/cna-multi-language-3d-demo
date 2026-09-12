#include "StarfieldGame.hpp"

#include <cerrno>
#include <cstdlib>
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
                     "[--validate-frame] [--screenshot FILE.ppm]\n";
        return 2;
    }

    starfield::StarfieldGame game(std::move(options));
    game.Run();

    std::cout << "starfield-cpp state=" << StateName(game.runState())
              << " collected=" << game.collectedCount()
              << " score=" << game.score()
              << " frame_valid=" << (game.frameValid() ? "true" : "false")
              << " player_x=" << game.player().position.X
              << " player_z=" << game.player().position.Z
              << " heading=" << game.player().heading
              << " hazard0_x=" << game.hazards()[0].position.X
              << " hazard1_x=" << game.hazards()[1].position.X << '\n';
    return game.frameValid() ? 0 : 1;
}
