#include <CNA/C/cna.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum starfield_state {
    STARFIELD_TITLE = 0,
    STARFIELD_PLAYING = 1,
    STARFIELD_WON = 2,
    STARFIELD_LOST = 3
};

struct starfield_game {
    enum starfield_state state;
    unsigned collected_mask;
    float x;
    float z;
    float elapsed;
    float score;
    float hazard_x;
    float hazard_direction;
    unsigned updates;
};

static CNA_Result callback_error(CNA_CallbackError* out_error, const char* message) {
    if (out_error != NULL) {
        out_error->message.data = message;
        out_error->message.byte_length = (uint64_t)strlen(message);
    }
    return CNA_RESULT_CALLBACK;
}

static CNA_Result load_content(CNA_Handle game, const CNA_GameTime* time, void* context,
                               CNA_CallbackError* out_error) {
    (void)game;
    (void)time;
    (void)out_error;
    struct starfield_game* state = (struct starfield_game*)context;
    state->state = STARFIELD_TITLE;
    state->collected_mask = 0;
    state->x = 0.0f;
    state->z = 0.0f;
    state->elapsed = 0.0f;
    state->score = 0.0f;
    state->hazard_x = 0.0f;
    state->hazard_direction = 1.0f;
    state->updates = 0;
    return CNA_RESULT_SUCCESS;
}

static CNA_Result update(CNA_Handle game, const CNA_GameTime* time, void* context,
                         CNA_CallbackError* out_error) {
    struct starfield_game* state = (struct starfield_game*)context;
    if (time == NULL) {
        return callback_error(out_error, "CNA supplied no GameTime to update");
    }

    const float seconds = (float)time->elapsed_game_time_ticks / 10000000.0f;
    if (state->state == STARFIELD_TITLE) {
        state->state = STARFIELD_PLAYING;
    }
    if (state->state == STARFIELD_PLAYING) {
        state->elapsed += seconds < 0.25f ? seconds : 0.25f;
        state->updates += 1;
        state->hazard_x += state->hazard_direction * 2.0f * seconds;
        if (state->hazard_x >= 7.0f) {
            state->hazard_x = 7.0f;
            state->hazard_direction = -1.0f;
        } else if (state->hazard_x <= -7.0f) {
            state->hazard_x = -7.0f;
            state->hazard_direction = 1.0f;
        }
        if ((state->x - state->hazard_x) * (state->x - state->hazard_x) +
                (state->z - 3.0f) * (state->z - 3.0f) <= 1.75f * 1.75f ||
            state->elapsed >= 60.0f) {
            state->state = STARFIELD_LOST;
        }
    }
    return CNA_RESULT_SUCCESS;
}

static CNA_Result draw(CNA_Handle game, const CNA_GameTime* time, void* context,
                       CNA_CallbackError* out_error) {
    (void)game;
    (void)time;
    (void)out_error;
    const struct starfield_game* state = (const struct starfield_game*)context;
    const CNA_Color color = state->state == STARFIELD_LOST
                                ? (CNA_Color){120u, 16u, 24u, 255u}
                                : state->state == STARFIELD_WON
                                      ? (CNA_Color){16u, 120u, 48u, 255u}
                                      : (CNA_Color){8u, 16u, 48u, 255u};
    return cna_game_clear(game, color);
}

static int run_frames(int frame_count) {
    struct starfield_game state = {STARFIELD_TITLE, 0u, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0u};
    const CNA_GameCallbacks callbacks = {
        sizeof(CNA_GameCallbacks), 1u, load_content, update, draw, NULL, NULL, &state};
    const CNA_GameCreateInfo create_info = {
        sizeof(CNA_GameCreateInfo), 1u, CNA_TRUE, {0u}, 166667,
        {(const char*)"CNA Starfield Courier", sizeof("CNA Starfield Courier") - 1u}, &callbacks};
    CNA_Handle game = CNA_INVALID_HANDLE;
    CNA_Result result = cna_game_create(&create_info, &game);
    if (result != CNA_RESULT_SUCCESS) {
        fprintf(stderr, "cna_game_create failed: %u\n", result);
        return 1;
    }

    for (int frame = 0; frame < frame_count && result == CNA_RESULT_SUCCESS; ++frame) {
        result = cna_game_run_one_frame(game);
    }
    const CNA_Result destroy_result = cna_game_destroy(game);
    if (result != CNA_RESULT_SUCCESS || destroy_result != CNA_RESULT_SUCCESS) {
        fprintf(stderr, "CNA game failed: run=%u destroy=%u\n", result, destroy_result);
        return 1;
    }
    printf("cna-c state=%u collected=%u score=%.0f updates=%u\n", (unsigned)state.state,
           state.collected_mask, (double)state.score, state.updates);
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 3 || strcmp(argv[1], "--frames") != 0) {
        fprintf(stderr, "usage: cna_starfield_c --frames COUNT\n");
        return 2;
    }
    return run_frames(atoi(argv[2]));
}