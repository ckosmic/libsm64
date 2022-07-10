#pragma once

#include <stdint.h>

enum MarioTextures
{
    mario_texture_metal = 0,
    mario_texture_yellow_button,
    mario_texture_m_logo,
    mario_texture_hair_sideburn,
    mario_texture_mustache,
    mario_texture_eyes_front,
    mario_texture_eyes_half_closed,
    mario_texture_eyes_closed,
    mario_texture_eyes_dead,
    mario_texture_wings_half_1,
    mario_texture_wings_half_2,
    mario_texture_metal_wings_half_1 = -99,
    mario_texture_metal_wings_half_2,
    mario_texture_eyes_closed_unused1,
    mario_texture_eyes_closed_unused2,
    mario_texture_eyes_right,
    mario_texture_eyes_left,
    mario_texture_eyes_up,
    mario_texture_eyes_down
};

#define FORMAT_RGBA 0
#define FORMAT_IA 1

struct Texture {
    const int offset;
    const int width;
    const int height;
    const int format;
};

struct TextureAtlasInfo
{
    const uintptr_t offset;
    const int numUsedTextures;
    const int atlasWidth;
    const int atlasHeight;
    const struct Texture texInfos[];
};

extern struct TextureAtlasInfo* mario_atlas_info;

extern void load_textures_from_rom( uint8_t *rom, struct TextureAtlasInfo* atlasInfo, uint8_t *outTexture );