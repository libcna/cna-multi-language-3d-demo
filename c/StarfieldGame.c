#include <CNA/C/cna.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REFERENCE_WIDTH 1280
#define REFERENCE_HEIGHT 720
#define SECTOR_COUNT 3
#define PI 3.14159265358979323846F
#define ARRAY_COUNT(value) (sizeof(value) / sizeof((value)[0]))

typedef enum RunState { RUN_TITLE, RUN_PLAYING, RUN_WON, RUN_LOST } RunState;
typedef enum HazardMotion { HAZARD_HORIZONTAL, HAZARD_VERTICAL, HAZARD_ORBIT } HazardMotion;

typedef struct Player {
    CNA_Vector3 position;
    float heading;
} Player;

typedef struct Hazard {
    CNA_Vector3 position;
    CNA_Vector3 origin;
    CNA_Vector3 velocity;
    HazardMotion motion;
    float phase;
    float range;
    int active;
} Hazard;

typedef struct Collectible {
    CNA_Vector3 position;
    int collected;
} Collectible;

typedef struct Theme {
    CNA_Color sky;
    CNA_Color floor;
    CNA_Color grid;
    CNA_Color boundary;
    CNA_Color accent;
} Theme;

typedef struct RuntimeOptions {
    int smoke_frames;
    int start_sector;
    int validate_frame;
    const char* screenshot_path;
    char content_root[1024];
} RuntimeOptions;

typedef struct StarfieldGame {
    RuntimeOptions options;
    CNA_Handle game;
    CNA_GraphicsDeviceManagerHandle manager;
    CNA_EffectHandle effect;
    CNA_Handle sounds[4];
    CNA_SongHandle music;
    RunState run_state;
    Player player;
    Hazard hazards[3];
    Collectible collectibles[3];
    CNA_Vector3 camera_position;
    CNA_Vector3 camera_target;
    CNA_Vector3 extraction_position;
    float elapsed_seconds;
    float presentation_seconds;
    float sector_banner_seconds;
    int score;
    int sector_index;
    int drawn_frames;
    int restart_held;
    int captured;
    int frame_valid;
} StarfieldGame;

static CNA_Vector3 vector3(float x, float y, float z)
{
    CNA_Vector3 value = {x, y, z};
    return value;
}

static CNA_Color color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    CNA_Color value = {r, g, b, a};
    return value;
}

static int clamp_int(int value, int low, int high)
{
    return value < low ? low : value > high ? high : value;
}

static float clamp_float(float value, float low, float high)
{
    return value < low ? low : value > high ? high : value;
}

static CNA_Color shade(CNA_Color value, float amount)
{
    return color(
        (uint8_t)clamp_int((int)lroundf((float)value.r * amount), 0, 255),
        (uint8_t)clamp_int((int)lroundf((float)value.g * amount), 0, 255),
        (uint8_t)clamp_int((int)lroundf((float)value.b * amount), 0, 255), value.a);
}

static Theme theme_for_sector(int sector)
{
    if (sector == 1) {
        const Theme value = {
            {13, 5, 27, 255}, {55, 30, 85, 255}, {130, 65, 150, 255},
            {30, 220, 220, 255}, {205, 80, 255, 255}};
        return value;
    }
    if (sector == 2) {
        const Theme value = {
            {25, 5, 8, 255}, {72, 27, 25, 255}, {150, 58, 35, 255},
            {255, 185, 35, 255}, {255, 75, 35, 255}};
        return value;
    }
    {
        const Theme value = {
            {4, 10, 30, 255}, {25, 45, 90, 255}, {55, 90, 135, 255},
            {255, 120, 20, 255}, {0, 175, 225, 255}};
        return value;
    }
}

static float distance_squared_xz(CNA_Vector3 left, CNA_Vector3 right)
{
    const float x = left.x - right.x;
    const float z = left.z - right.z;
    return x * x + z * z;
}

static int collected_count(const StarfieldGame* state)
{
    int count = 0;
    int index;
    for (index = 0; index < 3; ++index) {
        count += state->collectibles[index].collected != 0;
    }
    return count;
}

static void update_camera(StarfieldGame* state)
{
    const float forward_x = sinf(state->player.heading);
    const float forward_z = -cosf(state->player.heading);
    state->camera_position = vector3(
        state->player.position.x - forward_x * 10.0F,
        7.0F,
        state->player.position.z - forward_z * 10.0F);
    state->camera_target = vector3(
        state->player.position.x, 0.5F, state->player.position.z);
}

static void set_cell(StarfieldGame* state, int index, float x, float z)
{
    state->collectibles[index].position = vector3(x, 0.7F, z);
    state->collectibles[index].collected = 0;
}

static void set_hazard(
    StarfieldGame* state, int index, float x, float y, float z,
    float velocity_x, float velocity_z, HazardMotion motion, float range)
{
    Hazard* const hazard = &state->hazards[index];
    hazard->position = vector3(x, y, z);
    hazard->origin = hazard->position;
    hazard->velocity = vector3(velocity_x, 0.0F, velocity_z);
    hazard->motion = motion;
    hazard->phase = 0.0F;
    hazard->range = range;
    hazard->active = 1;
}

static void load_sector(StarfieldGame* state, int sector)
{
    int index;
    state->sector_index = clamp_int(sector, 0, SECTOR_COUNT - 1);
    state->elapsed_seconds = 0.0F;
    state->sector_banner_seconds = 2.4F;
    memset(state->hazards, 0, sizeof(state->hazards));
    memset(state->collectibles, 0, sizeof(state->collectibles));
    state->player.heading = 0.0F;

    switch (state->sector_index) {
    case 1:
        state->player.position = vector3(0.0F, 0.6F, 8.0F);
        state->extraction_position = vector3(8.0F, 0.0F, -8.0F);
        set_cell(state, 0, -7.0F, 6.0F);
        set_cell(state, 1, 7.0F, -1.0F);
        set_cell(state, 2, -5.0F, -6.0F);
        set_hazard(state, 0, -2.0F, 0.8F, 0.0F, 0.0F, 2.0F, HAZARD_VERTICAL, 7.0F);
        set_hazard(state, 1, 0.0F, 0.7F, -2.0F, -2.0F, 0.0F, HAZARD_HORIZONTAL, 7.0F);
        set_hazard(state, 2, 4.5F, 0.9F, 2.0F, 0.0F, 0.0F, HAZARD_ORBIT, 2.5F);
        state->hazards[2].origin = vector3(2.0F, 0.9F, 2.0F);
        break;
    case 2:
        state->player.position = vector3(0.0F, 0.6F, 8.0F);
        state->extraction_position = vector3(-8.0F, 0.0F, -8.0F);
        set_cell(state, 0, -7.0F, -5.0F);
        set_cell(state, 1, 0.0F, -6.0F);
        set_cell(state, 2, 6.0F, 6.0F);
        set_hazard(state, 0, 0.0F, 0.8F, 4.0F, 2.5F, 0.0F, HAZARD_HORIZONTAL, 8.0F);
        set_hazard(state, 1, -4.0F, 0.7F, 0.0F, 0.0F, -2.4F, HAZARD_VERTICAL, 8.0F);
        set_hazard(state, 2, 6.0F, 1.0F, -3.0F, 0.0F, 0.0F, HAZARD_ORBIT, 3.0F);
        state->hazards[2].origin = vector3(3.0F, 1.0F, -3.0F);
        break;
    default:
        state->player.position = vector3(0.0F, 0.6F, 0.0F);
        state->extraction_position = vector3(0.0F, 0.0F, -9.0F);
        set_cell(state, 0, -6.0F, 0.0F);
        set_cell(state, 1, 0.0F, -5.0F);
        set_cell(state, 2, 6.0F, 0.0F);
        set_hazard(state, 0, 0.0F, 0.8F, 3.0F, 2.0F, 0.0F, HAZARD_HORIZONTAL, 7.0F);
        set_hazard(state, 1, 0.0F, 0.6F, -3.5F, -2.0F, 0.0F, HAZARD_HORIZONTAL, 7.0F);
        for (index = 2; index < 3; ++index) state->hazards[index].active = 0;
        break;
    }
    update_camera(state);
}

static void reset_game(StarfieldGame* state)
{
    state->run_state = RUN_TITLE;
    state->score = 0;
    load_sector(state, 0);
}

static void move_hazard(Hazard* hazard, float seconds)
{
    float* coordinate;
    float* speed;
    float center;
    if (hazard->motion == HAZARD_ORBIT) {
        hazard->phase = remainderf(hazard->phase + seconds * 1.35F, 2.0F * PI);
        hazard->position.x = hazard->origin.x + cosf(hazard->phase) * hazard->range;
        hazard->position.z = hazard->origin.z + sinf(hazard->phase) * hazard->range;
        return;
    }
    coordinate = hazard->motion == HAZARD_HORIZONTAL ? &hazard->position.x : &hazard->position.z;
    speed = hazard->motion == HAZARD_HORIZONTAL ? &hazard->velocity.x : &hazard->velocity.z;
    center = hazard->motion == HAZARD_HORIZONTAL ? hazard->origin.x : hazard->origin.z;
    *coordinate += *speed * seconds;
    if (*coordinate >= center + hazard->range) {
        *coordinate = center + hazard->range;
        *speed = -fabsf(*speed);
    } else if (*coordinate <= center - hazard->range) {
        *coordinate = center - hazard->range;
        *speed = fabsf(*speed);
    }
}

static void play_sound(CNA_Handle sound, float volume, float pitch)
{
    CNA_Bool played = CNA_FALSE;
    if (sound != CNA_INVALID_HANDLE) {
        (void)cna_sound_effect_play_with_settings(sound, volume, pitch, 0.0F, &played);
    }
}

static void advance_game(
    StarfieldGame* state, float seconds, float turn, float throttle, int boost, int restart)
{
    int index;
    int collected_this_step = 0;
    float dt;
    float movement;
    if (restart) {
        play_sound(state->sounds[2], 0.42F, 0.08F);
        reset_game(state);
        return;
    }
    if (state->run_state == RUN_WON || state->run_state == RUN_LOST) return;
    if (state->run_state == RUN_TITLE) state->run_state = RUN_PLAYING;

    dt = clamp_float(seconds, 0.0F, 0.25F);
    state->sector_banner_seconds = fmaxf(0.0F, state->sector_banner_seconds - dt);
    state->player.heading = remainderf(
        state->player.heading + clamp_float(turn, -1.0F, 1.0F) * PI * dt, 2.0F * PI);
    movement = clamp_float(throttle, -1.0F, 1.0F) * (boost ? 7.0F : 4.0F) * dt;
    state->player.position.x = clamp_float(
        state->player.position.x + sinf(state->player.heading) * movement, -10.0F, 10.0F);
    state->player.position.z = clamp_float(
        state->player.position.z - cosf(state->player.heading) * movement, -10.0F, 10.0F);
    state->elapsed_seconds += dt;

    for (index = 0; index < 3; ++index) {
        if (state->hazards[index].active) move_hazard(&state->hazards[index], dt);
    }
    for (index = 0; index < 3; ++index) {
        Collectible* const cell = &state->collectibles[index];
        if (!cell->collected && distance_squared_xz(state->player.position, cell->position) <= 0.81F) {
            cell->collected = 1;
            state->score += 100;
            collected_this_step = 1;
        }
    }
    for (index = 0; index < 3; ++index) {
        if (state->hazards[index].active &&
            distance_squared_xz(state->player.position, state->hazards[index].position) <= 3.0625F) {
            state->run_state = RUN_LOST;
            play_sound(state->sounds[1], 0.62F, -0.18F);
            update_camera(state);
            return;
        }
    }
    if (state->elapsed_seconds >= 60.0F) {
        state->run_state = RUN_LOST;
        play_sound(state->sounds[1], 0.62F, -0.35F);
        update_camera(state);
        return;
    }
    if (collected_count(state) == 3 &&
        distance_squared_xz(state->player.position, state->extraction_position) <= 1.96F) {
        if (state->sector_index + 1 < SECTOR_COUNT) {
            state->score += 500;
            load_sector(state, state->sector_index + 1);
            play_sound(state->sounds[3], 0.48F, -0.08F);
            return;
        }
        state->run_state = RUN_WON;
        state->score += 1000;
        play_sound(state->sounds[3], 0.62F, 0.08F);
    } else if (collected_this_step) {
        play_sound(state->sounds[0], 0.52F, -0.08F + (float)collected_count(state) * 0.08F);
    }
    update_camera(state);
}

static void make_cube(CNA_VertexPositionColor* vertices, CNA_Color value)
{
    static const CNA_Vector3 points[8] = {
        {-0.5F,-0.5F,-0.5F}, {0.5F,-0.5F,-0.5F}, {0.5F,0.5F,-0.5F}, {-0.5F,0.5F,-0.5F},
        {-0.5F,-0.5F,0.5F}, {0.5F,-0.5F,0.5F}, {0.5F,0.5F,0.5F}, {-0.5F,0.5F,0.5F}};
    static const int indices[36] = {
        0,1,2,0,2,3, 4,6,5,4,7,6, 0,4,5,0,5,1, 3,2,6,3,6,7,
        1,5,6,1,6,2, 0,3,7,0,7,4};
    static const float brightness[6] = {0.82F,0.58F,0.34F,1.0F,0.72F,0.48F};
    int index;
    for (index = 0; index < 36; ++index) {
        vertices[index].position = points[indices[index]];
        vertices[index].color = shade(value, brightness[index / 6]);
    }
}

static CNA_Result draw_cube(
    StarfieldGame* state, CNA_Handle device, CNA_Vector3 position,
    CNA_Vector3 scale, CNA_Color value, float yaw)
{
    CNA_VertexPositionColor vertices[36];
    CNA_UserPrimitives primitives;
    CNA_Matrix scale_matrix, rotation_matrix, translation_matrix, intermediate, world;
    make_cube(vertices, value);
    if (cna_matrix_create_scale_vector3(scale, &scale_matrix) != CNA_RESULT_SUCCESS ||
        cna_matrix_create_rotation_y(yaw, &rotation_matrix) != CNA_RESULT_SUCCESS ||
        cna_matrix_create_translation_vector3(position, &translation_matrix) != CNA_RESULT_SUCCESS ||
        cna_matrix_multiply(scale_matrix, rotation_matrix, &intermediate) != CNA_RESULT_SUCCESS ||
        cna_matrix_multiply(intermediate, translation_matrix, &world) != CNA_RESULT_SUCCESS ||
        cna_effect_matrices_set_world(state->effect, world) != CNA_RESULT_SUCCESS ||
        cna_effect_apply(state->effect) != CNA_RESULT_SUCCESS) {
        return CNA_RESULT_INVALID_STATE;
    }
    memset(&primitives, 0, sizeof(primitives));
    primitives.struct_size = (uint32_t)sizeof(primitives);
    primitives.struct_version = UINT32_C(1);
    primitives.primitive_type = CNA_PRIMITIVE_TRIANGLE_LIST;
    primitives.vertex_source = CNA_USER_VERTEX_SOURCE_POSITION_COLOR;
    primitives.vertex_data = vertices;
    primitives.vertex_declaration = CNA_INVALID_HANDLE;
    primitives.num_vertices = 36;
    primitives.primitive_count = 12;
    return cna_graphics_device_draw_user_primitives(device, &primitives);
}

#define DRAW(position, scale, value, yaw) do { \
    const CNA_Result draw_result__ = draw_cube(state, device, position, scale, value, yaw); \
    if (draw_result__ != CNA_RESULT_SUCCESS) return draw_result__; \
} while (0)

static CNA_Vector3 local_offset(CNA_Vector3 origin, float heading, float right, float up, float forward)
{
    return vector3(
        origin.x + cosf(heading) * right + sinf(heading) * forward,
        origin.y + up,
        origin.z + sinf(heading) * right - cosf(heading) * forward);
}

static CNA_Result draw_world(StarfieldGame* state, CNA_Handle device)
{
    CNA_DepthStencilState depth;
    CNA_BlendState blend;
    CNA_RasterizerState rasterizer;
    CNA_Matrix identity = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    CNA_Matrix view, projection;
    const Theme theme = theme_for_sector(state->sector_index);
    int offset, index;
    if (cna_depth_stencil_state_init(CNA_DEPTH_STENCIL_STATE_PRESET_DEFAULT, &depth) != CNA_RESULT_SUCCESS ||
        cna_blend_state_init(CNA_BLEND_STATE_PRESET_OPAQUE, &blend) != CNA_RESULT_SUCCESS ||
        cna_rasterizer_state_init(CNA_RASTERIZER_STATE_PRESET_CULL_NONE, &rasterizer) != CNA_RESULT_SUCCESS ||
        cna_graphics_device_set_depth_stencil_state(device, &depth) != CNA_RESULT_SUCCESS ||
        cna_graphics_device_set_blend_state(device, &blend) != CNA_RESULT_SUCCESS ||
        cna_graphics_device_set_rasterizer_state(device, &rasterizer) != CNA_RESULT_SUCCESS ||
        cna_matrix_create_look_at(state->camera_position, state->camera_target, vector3(0,1,0), &view) != CNA_RESULT_SUCCESS ||
        cna_matrix_create_perspective_field_of_view(PI / 4.0F, 16.0F / 9.0F, 0.1F, 100.0F, &projection) != CNA_RESULT_SUCCESS ||
        cna_effect_matrices_set_world(state->effect, identity) != CNA_RESULT_SUCCESS ||
        cna_effect_matrices_set_view(state->effect, view) != CNA_RESULT_SUCCESS ||
        cna_effect_matrices_set_projection(state->effect, projection) != CNA_RESULT_SUCCESS) {
        return CNA_RESULT_INVALID_STATE;
    }

    DRAW(vector3(0,-0.38F,0), vector3(21,0.6F,21), shade(theme.floor,0.38F), 0);
    DRAW(vector3(0,-0.055F,0), vector3(20,0.08F,20), theme.floor, 0);
    for (offset = -10; offset <= 10; offset += 2) {
        DRAW(vector3((float)offset,0.005F,0), vector3(0.028F,0.025F,20), theme.grid, 0);
        DRAW(vector3(0,0.005F,(float)offset), vector3(20,0.025F,0.028F), theme.grid, 0);
    }
    DRAW(vector3(0,0.022F,0), vector3(0.06F,0.035F,20), shade(theme.accent,0.72F), 0);
    DRAW(vector3(-10.1F,0.18F,0), vector3(0.22F,0.36F,20.4F), theme.boundary, 0);
    DRAW(vector3(10.1F,0.18F,0), vector3(0.22F,0.36F,20.4F), theme.boundary, 0);
    DRAW(vector3(0,0.18F,-10.1F), vector3(20.4F,0.36F,0.22F), theme.boundary, 0);
    DRAW(vector3(0,0.18F,10.1F), vector3(20.4F,0.36F,0.22F), theme.boundary, 0);
    for (offset = -10; offset <= 10; offset += 5) {
        DRAW(vector3(-10,0.8F,(float)offset), vector3(0.32F,1.6F,0.32F), shade(theme.boundary,0.38F), 0);
        DRAW(vector3(10,0.8F,(float)offset), vector3(0.32F,1.6F,0.32F), shade(theme.boundary,0.38F), 0);
        DRAW(vector3((float)offset,0.8F,-10), vector3(0.32F,1.6F,0.32F), shade(theme.boundary,0.38F), 0);
        DRAW(vector3((float)offset,0.8F,10), vector3(0.32F,1.6F,0.32F), shade(theme.boundary,0.38F), 0);
    }
    if (state->sector_index == 1) {
        DRAW(vector3(-11.2F,2,-6), vector3(0.7F,4,0.7F), theme.accent, -0.25F);
        DRAW(vector3(11.2F,2,-6), vector3(0.7F,4,0.7F), theme.accent, 0.25F);
    } else if (state->sector_index == 2) {
        for (offset = -6; offset <= 6; offset += 4)
            DRAW(vector3((float)offset,0.035F,0), vector3(0.34F,0.035F,19.6F), theme.accent, 0);
    } else {
        DRAW(vector3(-1.35F,0.05F,-7), vector3(0.06F,0.05F,5.4F), color(35,155,110,255), 0);
        DRAW(vector3(1.35F,0.05F,-7), vector3(0.06F,0.05F,5.4F), color(35,155,110,255), 0);
    }

    {
        const float heading = state->player.heading;
        const float yaw = -heading;
        DRAW(state->player.position, vector3(1.2F,0.6F,1.8F), color(0,175,225,255), yaw);
        DRAW(local_offset(state->player.position,heading,0,0.34F,0.2F), vector3(0.62F,0.28F,0.8F), color(25,60,105,255), yaw);
        DRAW(local_offset(state->player.position,heading,0,0.02F,1.05F), vector3(0.52F,0.32F,0.65F), color(100,245,255,255), yaw);
    }
    for (index = 0; index < 3; ++index) {
        if (!state->collectibles[index].collected) {
            const float phase = state->presentation_seconds * 1.8F + (float)index * 1.7F;
            const float pulse = 1.0F + sinf(phase) * 0.08F;
            DRAW(state->collectibles[index].position, vector3(0.52F*pulse,1.2F,0.52F*pulse), color(255,210,0,255), phase*0.7F);
            DRAW(state->collectibles[index].position, vector3(1.05F,0.09F,0.16F), color(255,242,125,255), phase*0.7F);
        }
        if (state->hazards[index].active) {
            const CNA_Color hazard_color = state->hazards[index].motion == HAZARD_VERTICAL
                ? color(205,45,235,255) : state->hazards[index].motion == HAZARD_ORBIT
                ? color(255,105,25,255) : color(230,40,50,255);
            const float spin = state->presentation_seconds * 1.6F * (index == 0 ? 1.0F : -1.0F);
            DRAW(state->hazards[index].position, vector3(1.15F,1.05F,1.15F), hazard_color, spin);
            DRAW(state->hazards[index].position, vector3(2.2F,0.2F,0.34F), shade(hazard_color,0.58F), spin);
        }
    }
    {
        const int active = collected_count(state) == 3;
        const CNA_Color gate = active ? color(40,255,80,255) : color(20,100,50,255);
        const float x = state->extraction_position.x;
        const float z = state->extraction_position.z;
        DRAW(vector3(x-1.45F,1.15F,z), vector3(0.42F,2.3F,0.7F), gate, 0);
        DRAW(vector3(x+1.45F,1.15F,z), vector3(0.42F,2.3F,0.7F), gate, 0);
        DRAW(vector3(x,2.23F,z), vector3(3.3F,0.35F,0.7F), gate, 0);
    }
    return CNA_RESULT_SUCCESS;
}

static CNA_Result draw_digit(
    StarfieldGame* state, CNA_Handle device, int digit, float x, float y, CNA_Color value)
{
    static const uint8_t segments[10] = {
        0x7E,0x30,0x6D,0x79,0x33,0x5B,0x5F,0x70,0x7F,0x7B};
    const uint8_t bits = segments[digit];
    if (bits & 0x40) DRAW(vector3(x,y-18,0),vector3(16,4,0.1F),value,0);
    if (bits & 0x20) DRAW(vector3(x+8,y-9,0),vector3(4,16,0.1F),value,0);
    if (bits & 0x10) DRAW(vector3(x+8,y+9,0),vector3(4,16,0.1F),value,0);
    if (bits & 0x08) DRAW(vector3(x,y+18,0),vector3(16,4,0.1F),value,0);
    if (bits & 0x04) DRAW(vector3(x-8,y+9,0),vector3(4,16,0.1F),value,0);
    if (bits & 0x02) DRAW(vector3(x-8,y-9,0),vector3(4,16,0.1F),value,0);
    if (bits & 0x01) DRAW(vector3(x,y,0),vector3(16,4,0.1F),value,0);
    return CNA_RESULT_SUCCESS;
}

static CNA_Result draw_hud(StarfieldGame* state, CNA_Handle device)
{
    CNA_DepthStencilState depth;
    CNA_BlendState blend;
    CNA_Matrix identity = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    CNA_Matrix projection;
    const Theme theme = theme_for_sector(state->sector_index);
    int index;
    int display_score = state->score;
    const float remaining = fmaxf(0.0F, 60.0F - state->elapsed_seconds) / 60.0F;
    if (cna_depth_stencil_state_init(CNA_DEPTH_STENCIL_STATE_PRESET_NONE, &depth) != CNA_RESULT_SUCCESS ||
        cna_blend_state_init(CNA_BLEND_STATE_PRESET_ALPHA_BLEND, &blend) != CNA_RESULT_SUCCESS ||
        cna_graphics_device_set_depth_stencil_state(device, &depth) != CNA_RESULT_SUCCESS ||
        cna_graphics_device_set_blend_state(device, &blend) != CNA_RESULT_SUCCESS ||
        cna_matrix_create_orthographic_off_center(0,REFERENCE_WIDTH,REFERENCE_HEIGHT,0,0,10,&projection) != CNA_RESULT_SUCCESS ||
        cna_effect_matrices_set_view(state->effect, identity) != CNA_RESULT_SUCCESS ||
        cna_effect_matrices_set_projection(state->effect, projection) != CNA_RESULT_SUCCESS) return CNA_RESULT_INVALID_STATE;

    DRAW(vector3(239.5F,62,0), vector3(435,88,0.1F), color(5,12,30,225), 0);
    for (index = 0; index < 3; ++index)
        DRAW(vector3(49.5F+38.0F*(float)index,73.5F,0), vector3(27,27,0.1F),
            state->collectibles[index].collected ? color(255,215,0,255) : color(35,52,78,255),0);
    DRAW(vector3(306,73,0), vector3(244,14,0.1F), color(24,38,63,255),0);
    DRAW(vector3(187+remaining*119,73,0), vector3(remaining*238,8,0.1F),
        remaining > 0.25F ? color(60,230,130,255) : color(255,80,50,255),0);
    DRAW(vector3(724,46,0), vector3(500,56,0.1F), color(5,12,30,210),0);
    DRAW(vector3(480+state->sector_index*38,46,0), vector3(28,28,0.1F), theme.accent,0);
    DRAW(vector3(1127,62,0), vector3(262,88,0.1F), color(5,12,30,225),0);
    for (index = 3; index >= 0; --index) {
        const CNA_Result result = draw_digit(state,device,display_score%10,
            1230.0F-(float)(3-index)*34.0F,68.0F,color(100,245,255,255));
        if (result != CNA_RESULT_SUCCESS) return result;
        display_score /= 10;
    }
    DRAW(vector3(327,694.5F,0), vector3(610,25,0.1F), color(5,12,30,205),0);
    if (state->run_state == RUN_WON || state->run_state == RUN_LOST) {
        const CNA_Color terminal = state->run_state == RUN_WON ? color(40,255,90,255) : color(255,70,55,255);
        DRAW(vector3(640,357.5F,0), vector3(990,305,0.1F), color(3,8,24,238),0);
        DRAW(vector3(640,208,0), vector3(990,6,0.1F), terminal,0);
        DRAW(vector3(640,507,0), vector3(990,6,0.1F), terminal,0);
    }
    return CNA_RESULT_SUCCESS;
}

static int colors_equal(CNA_Color left, CNA_Color right)
{
    return left.r == right.r && left.g == right.g && left.b == right.b && left.a == right.a;
}

static CNA_Result capture_frame(StarfieldGame* state, CNA_Handle device)
{
    CNA_BackBufferInfo info;
    CNA_Color* pixels;
    uint64_t count = 0;
    uint64_t index;
    uint64_t non_clear=0, floor=0, grid=0, boundary=0, player=0, cell=0, hazard=0, gate=0, hud=0;
    const Theme theme = theme_for_sector(state->sector_index);
    FILE* output;
    memset(&info,0,sizeof(info));
    info.struct_size = (uint32_t)sizeof(info);
    info.struct_version = UINT32_C(1);
    if (cna_graphics_device_get_backbuffer_info(device,&info) != CNA_RESULT_SUCCESS) return CNA_RESULT_INVALID_STATE;
    count = (uint64_t)info.width * (uint64_t)info.height;
    pixels = (CNA_Color*)malloc((size_t)count * sizeof(CNA_Color));
    if (pixels == NULL) return CNA_RESULT_OUT_OF_MEMORY;
    if (cna_graphics_device_get_backbuffer_data_rgba8(device,pixels,count,&count) != CNA_RESULT_SUCCESS) {
        free(pixels); return CNA_RESULT_INVALID_STATE;
    }
    for (index=0; index<count; ++index) {
        const CNA_Color pixel = pixels[index];
        non_clear += !colors_equal(pixel,theme.sky);
        floor += colors_equal(pixel,theme.floor);
        grid += colors_equal(pixel,theme.grid);
        boundary += colors_equal(pixel,theme.boundary);
        player += pixel.b>160 && pixel.g>100 && pixel.r<120;
        cell += pixel.r>210 && pixel.g>135 && pixel.b<130;
        hazard += pixel.r>175 && pixel.g<100 && pixel.b<140;
        gate += pixel.g>50 && pixel.g>pixel.r*2 && pixel.g>pixel.b;
        hud += index<(uint64_t)info.width*115U && !colors_equal(pixel,theme.sky);
    }
    state->frame_valid = info.width==REFERENCE_WIDTH && info.height==REFERENCE_HEIGHT &&
        non_clear>count/100U && floor>count/10U && grid>1500U && boundary>2000U &&
        player>300U && cell>250U && hazard>150U && gate>200U && hud>3000U;
    if (!state->frame_valid) fprintf(stderr,
        "incomplete C frame: non-clear=%llu floor=%llu grid=%llu boundary=%llu player=%llu cell=%llu hazard=%llu gate=%llu hud=%llu\n",
        (unsigned long long)non_clear,(unsigned long long)floor,(unsigned long long)grid,
        (unsigned long long)boundary,(unsigned long long)player,(unsigned long long)cell,
        (unsigned long long)hazard,(unsigned long long)gate,(unsigned long long)hud);
    if (state->options.screenshot_path != NULL) {
        output = fopen(state->options.screenshot_path,"wb");
        if (output == NULL) { free(pixels); return CNA_RESULT_IO; }
        fprintf(output,"P6\n%u %u\n255\n",info.width,info.height);
        for (index=0; index<count; ++index) {
            fputc(pixels[index].r,output); fputc(pixels[index].g,output); fputc(pixels[index].b,output);
        }
        if (fclose(output)!=0) { free(pixels); return CNA_RESULT_IO; }
    }
    free(pixels);
    return state->options.validate_frame && !state->frame_valid ? CNA_RESULT_INVALID_STATE : CNA_RESULT_SUCCESS;
}

#ifndef STARFIELD_GAMEPLAY_TEST
static CNA_Result on_initialize(CNA_Handle game, const CNA_GameTime* time, void* context, CNA_CallbackError* error)
{
    StarfieldGame* const state = (StarfieldGame*)context;
    (void)game; (void)time; (void)error;
    reset_game(state);
    if (state->options.start_sector > 0) {
        load_sector(state,state->options.start_sector);
        state->run_state=RUN_TITLE; state->score=0;
    }
    return CNA_RESULT_SUCCESS;
}

static CNA_Result on_load(CNA_Handle game, const CNA_GameTime* time, void* context, CNA_CallbackError* error)
{
    StarfieldGame* const state = (StarfieldGame*)context;
    CNA_Handle device = CNA_INVALID_HANDLE;
    const char* names[4] = {"collect.wav","loss.wav","restart.wav","win.wav"};
    int index;
    char path[1200];
    (void)time; (void)error;
    if (cna_game_get_graphics_device(game,&device)!=CNA_RESULT_SUCCESS ||
        cna_basic_effect_create(device,&state->effect)!=CNA_RESULT_SUCCESS ||
        cna_basic_effect_set_vertex_color_enabled(state->effect,CNA_TRUE)!=CNA_RESULT_SUCCESS)
        return CNA_RESULT_INVALID_STATE;
    for (index=0; index<4; ++index) {
        CNA_Result result;
        snprintf(path,sizeof(path),"%s/audio/%s",state->options.content_root,names[index]);
        {
            const CNA_StringView view = {path,(uint64_t)strlen(path)};
            result = cna_sound_effect_create_from_asset_ext(game,view,&state->sounds[index]);
        }
        if (result==CNA_RESULT_NOT_SUPPORTED) state->sounds[index]=CNA_INVALID_HANDLE;
        else if (result!=CNA_RESULT_SUCCESS) return result;
    }
    snprintf(path,sizeof(path),"%s/audio/outer_space.mp3",state->options.content_root);
    {
        const CNA_StringView name = {"Outer Space Loop",16U};
        const CNA_StringView uri = {path,(uint64_t)strlen(path)};
        const CNA_Result result = cna_song_create_from_uri(game,name,uri,&state->music);
        if (result==CNA_RESULT_SUCCESS) {
            (void)cna_media_player_set_volume(game,0.14F);
            (void)cna_media_player_set_is_repeating(game,CNA_TRUE);
            (void)cna_media_player_play_song(game,state->music);
        } else if (result!=CNA_RESULT_NOT_SUPPORTED) return result;
    }
    return CNA_RESULT_SUCCESS;
}

static CNA_Result on_update(CNA_Handle game, const CNA_GameTime* time, void* context, CNA_CallbackError* error)
{
    StarfieldGame* const state = (StarfieldGame*)context;
    CNA_KeyboardState keys;
    CNA_Bool escape,left,right,forward,back,boost,restart;
    CNA_Bool arrow_left,arrow_right,arrow_up,arrow_down,enter;
    (void)error;
    memset(&keys,0,sizeof(keys)); keys.struct_size=sizeof(keys); keys.struct_version=1;
    if (cna_keyboard_get_state(game,&keys)!=CNA_RESULT_SUCCESS) return CNA_RESULT_INVALID_STATE;
#define KEY(name,key) if(cna_keyboard_state_is_key_down(&keys,key,&name)!=CNA_RESULT_SUCCESS)return CNA_RESULT_INVALID_STATE
    KEY(escape,CNA_KEY_ESCAPE); KEY(left,CNA_KEY_A); KEY(right,CNA_KEY_D);
    KEY(forward,CNA_KEY_W); KEY(back,CNA_KEY_S); KEY(boost,CNA_KEY_SPACE); KEY(restart,CNA_KEY_R);
    KEY(arrow_left,CNA_KEY_LEFT); KEY(arrow_right,CNA_KEY_RIGHT);
    KEY(arrow_up,CNA_KEY_UP); KEY(arrow_down,CNA_KEY_DOWN); KEY(enter,CNA_KEY_ENTER);
#undef KEY
    if (escape==CNA_TRUE) return cna_game_request_exit(game);
    state->presentation_seconds=(float)((double)time->total_game_time_ticks/10000000.0);
    advance_game(state,(float)((double)time->elapsed_game_time_ticks/10000000.0),
        (right==CNA_TRUE || arrow_right==CNA_TRUE?1.0F:0.0F)-
            (left==CNA_TRUE || arrow_left==CNA_TRUE?1.0F:0.0F),
        (forward==CNA_TRUE || arrow_up==CNA_TRUE?1.0F:0.0F)-
            (back==CNA_TRUE || arrow_down==CNA_TRUE?1.0F:0.0F),
        boost==CNA_TRUE,
        (restart==CNA_TRUE || enter==CNA_TRUE) && !state->restart_held);
    state->restart_held=restart==CNA_TRUE || enter==CNA_TRUE;
    return CNA_RESULT_SUCCESS;
}

static CNA_Result on_draw(CNA_Handle game, const CNA_GameTime* time, void* context, CNA_CallbackError* error)
{
    StarfieldGame* const state = (StarfieldGame*)context;
    CNA_Handle device=CNA_INVALID_HANDLE;
    CNA_Result result;
    const Theme theme=theme_for_sector(state->sector_index);
    (void)time; (void)error;
    if(cna_game_get_graphics_device(game,&device)!=CNA_RESULT_SUCCESS)return CNA_RESULT_INVALID_STATE;
    result=cna_graphics_device_clear_options(device,CNA_CLEAR_OPTION_TARGET|CNA_CLEAR_OPTION_DEPTH_BUFFER,theme.sky,1.0F,0);
    if(result==CNA_RESULT_SUCCESS)result=draw_world(state,device);
    if(result==CNA_RESULT_SUCCESS)result=draw_hud(state,device);
    if(result==CNA_RESULT_SUCCESS && !state->captured &&
       (state->options.validate_frame || state->options.screenshot_path!=NULL)) {
        result=capture_frame(state,device); state->captured=1;
    }
    ++state->drawn_frames;
    if(result==CNA_RESULT_SUCCESS && state->options.smoke_frames>0 && state->drawn_frames>=state->options.smoke_frames)
        result=cna_game_request_exit(game);
    return result;
}

static CNA_Result on_unload(CNA_Handle game, const CNA_GameTime* time, void* context, CNA_CallbackError* error)
{
    StarfieldGame* const state=(StarfieldGame*)context;
    int index;
    (void)time; (void)error;
    (void)cna_media_player_stop(game);
    if(state->music!=CNA_INVALID_HANDLE){(void)cna_song_destroy(state->music);state->music=CNA_INVALID_HANDLE;}
    for(index=3;index>=0;--index)if(state->sounds[index]!=CNA_INVALID_HANDLE){(void)cna_sound_effect_destroy(state->sounds[index]);state->sounds[index]=CNA_INVALID_HANDLE;}
    if(state->effect!=CNA_INVALID_HANDLE){(void)cna_effect_destroy(state->effect);state->effect=CNA_INVALID_HANDLE;}
    return CNA_RESULT_SUCCESS;
}

static const char* state_name(RunState state)
{
    return state==RUN_TITLE?"title":state==RUN_PLAYING?"playing":state==RUN_WON?"won":"lost";
}

#else
static int near_value(float actual,float expected,float tolerance){return fabsf(actual-expected)<=tolerance;}
static int expect(int condition,const char* message){if(!condition)fprintf(stderr,"FAILED: %s\n",message);return condition;}
int main(void)
{
    StarfieldGame state;
    int passed=1,index;
    memset(&state,0,sizeof(state)); for(index=0;index<4;++index)state.sounds[index]=CNA_INVALID_HANDLE;
    reset_game(&state);
    passed&=expect(state.run_state==RUN_TITLE&&state.score==0&&state.sector_index==0,"initial reset");
    advance_game(&state,0.25F,1,0,0,0);
    passed&=expect(state.run_state==RUN_PLAYING&&near_value(state.player.heading,PI/4,0.001F),"turning");
    advance_game(&state,0.25F,0,1,0,0);
    passed&=expect(near_value(state.player.position.x,0.707107F,0.001F)&&near_value(state.player.position.z,-0.707107F,0.001F),"movement");
    reset_game(&state); load_sector(&state,1); state.run_state=RUN_PLAYING;
    for(index=0;index<3;++index){state.player.position=state.collectibles[index].position;advance_game(&state,0,0,0,0,0);}
    state.player.position=state.extraction_position;advance_game(&state,0,0,0,0,0);
    passed&=expect(state.sector_index==2&&state.score==800,"sector transition");
    for(index=0;index<3;++index){state.player.position=state.collectibles[index].position;advance_game(&state,0,0,0,0,0);}
    state.player.position=state.extraction_position;advance_game(&state,0,0,0,0,0);
    passed&=expect(state.run_state==RUN_WON&&state.score==2100,"final win");
    reset_game(&state);for(index=0;index<3700&&state.run_state!=RUN_LOST;++index)advance_game(&state,1.0F/60.0F,0,0,0,0);
    passed&=expect(state.run_state==RUN_LOST&&near_value(state.elapsed_seconds,60,0.02F),"timeout");
    advance_game(&state,0.25F,1,1,1,1);
    passed&=expect(state.run_state==RUN_TITLE&&state.score==0&&state.sector_index==0,"restart");
    return passed?0:1;
}
#endif
#ifndef STARFIELD_GAMEPLAY_TEST
static int parse_positive(const char* text,int* value)
{
    char* end; long parsed;
    if(text==NULL||*text=='\0')return 0;
    parsed=strtol(text,&end,10);if(*end!='\0'||parsed<=0||parsed>2147483647L)return 0;
    *value=(int)parsed;return 1;
}

int main(int argc,char** argv)
{
    StarfieldGame state;
    CNA_GameCallbacks callbacks;
    CNA_GameFrameHooks hooks;
    CNA_GameCreateInfo info;
    CNA_Result result;
    const char* slash;
    int index;
    memset(&state,0,sizeof(state));
    state.options.smoke_frames=-1;state.frame_valid=1;
    state.game=state.manager=state.effect=state.music=CNA_INVALID_HANDLE;
    for(index=0;index<4;++index)state.sounds[index]=CNA_INVALID_HANDLE;
    for(index=1;index<argc;++index){
        if(strcmp(argv[index],"--smoke-frames")==0&&index+1<argc&&parse_positive(argv[++index],&state.options.smoke_frames))continue;
        if(strcmp(argv[index],"--start-sector")==0&&index+1<argc&&parse_positive(argv[++index],&state.options.start_sector)&&state.options.start_sector<=3){--state.options.start_sector;continue;}
        if(strcmp(argv[index],"--validate-frame")==0){state.options.validate_frame=1;continue;}
        if(strcmp(argv[index],"--screenshot")==0&&index+1<argc){state.options.screenshot_path=argv[++index];continue;}
        fprintf(stderr,"usage: starfield_c [--smoke-frames COUNT] [--validate-frame] [--screenshot FILE.ppm] [--start-sector 1|2|3]\n");return 2;
    }
    if(state.options.smoke_frames<0&&(state.options.validate_frame||state.options.screenshot_path!=NULL))state.options.smoke_frames=1;
    slash=strrchr(argv[0],'/');
    if(slash==NULL)strcpy(state.options.content_root,"Content");
    else {const size_t length=(size_t)(slash-argv[0]);if(length+9>=sizeof(state.options.content_root))return 2;memcpy(state.options.content_root,argv[0],length);state.options.content_root[length]='\0';strcat(state.options.content_root,"/Content");}
    reset_game(&state);
    memset(&callbacks,0,sizeof(callbacks));callbacks.struct_size=sizeof(callbacks);callbacks.struct_version=1;
    callbacks.load_content=on_load;callbacks.update=on_update;callbacks.draw=on_draw;callbacks.unload_content=on_unload;callbacks.context=&state;
    memset(&info,0,sizeof(info));info.struct_size=sizeof(info);info.struct_version=1;info.is_fixed_time_step=CNA_TRUE;info.target_elapsed_time_ticks=166667;
    info.window_title.data="CNA Starfield Courier - C";info.window_title.byte_length=25;info.callbacks=&callbacks;
    result=cna_game_create(&info,&state.game);
    if(result!=CNA_RESULT_SUCCESS)return 3;
    memset(&hooks,0,sizeof(hooks));hooks.struct_size=sizeof(hooks);hooks.struct_version=1;hooks.initialize=on_initialize;hooks.context=&state;
    if(cna_game_set_frame_hooks_ext(state.game,&hooks)!=CNA_RESULT_SUCCESS||
       cna_graphics_device_manager_create(state.game,&state.manager)!=CNA_RESULT_SUCCESS||
       cna_graphics_device_manager_set_preferred_back_buffer_width(state.manager,REFERENCE_WIDTH)!=CNA_RESULT_SUCCESS||
       cna_graphics_device_manager_set_preferred_back_buffer_height(state.manager,REFERENCE_HEIGHT)!=CNA_RESULT_SUCCESS||
       cna_graphics_device_manager_set_preferred_depth_stencil_format(state.manager,CNA_DEPTH_FORMAT_DEPTH24)!=CNA_RESULT_SUCCESS){(void)cna_game_destroy(state.game);return 4;}
    result=cna_game_run(state.game);
    printf("starfield-c state=%s collected=%d score=%d sector=%d frame_valid=%s player_x=%g player_z=%g heading=%g\n",
        state_name(state.run_state),collected_count(&state),state.score,state.sector_index+1,state.frame_valid?"true":"false",
        state.player.position.x,state.player.position.z,state.player.heading);
    if(cna_graphics_device_manager_destroy(state.manager)!=CNA_RESULT_SUCCESS)return 5;
    if(cna_game_destroy(state.game)!=CNA_RESULT_SUCCESS)return 6;
    return result==CNA_RESULT_SUCCESS&&state.frame_valid?0:7;
}
#endif
