#include "load_tex_data.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "libsm64.h"

#include "decomp/tools/libmio0.h"
#include "decomp/tools/n64graphics.h"

struct TextureAtlasInfo mario_atlas_info = {
    .offset             = 0x114750,
    .numUsedTextures    = 11,
    .atlasWidth         = 11*64,
    .atlasHeight        = 64,
    .texInfos = {
        { .offset = 144, .width = 64, .height = 32, .format = FORMAT_RGBA },
        { .offset = 4240, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 6288, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 8336, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 10384, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 12432, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 14480, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 16528, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 30864, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 32912, .width = 32, .height = 64, .format = FORMAT_RGBA },
        { .offset = 37008, .width = 32, .height = 64, .format = FORMAT_RGBA },
    }
};

struct TextureAtlasInfo coin_atlas_info = {
    .offset             = 0x201410,
    .numUsedTextures    = 4,
    .atlasWidth         = 4*32,
    .atlasHeight        = 32,
    .texInfos = {
        { .offset = 0x5780, .width = 32, .height = 32, .format = FORMAT_IA },
        { .offset = 0x5F80, .width = 32, .height = 32, .format = FORMAT_IA },
        { .offset = 0x6780, .width = 32, .height = 32, .format = FORMAT_IA },
        { .offset = 0x6F80, .width = 32, .height = 32, .format = FORMAT_IA },
    }
};

struct TextureAtlasInfo ui_atlas_info = {
    .offset             = 0x108A40,
    .numUsedTextures    = 14,
    .atlasWidth         = 14*16,
    .atlasHeight        = 16,
    .texInfos = {
        { .offset = 0x0000, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x0200, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x0400, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x0600, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x0800, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x0A00, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x0C00, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x0E00, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x1000, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x1200, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x4200, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x4400, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x4600, .width = 16, .height = 16, .format = FORMAT_RGBA },
        { .offset = 0x4800, .width = 16, .height = 16, .format = FORMAT_RGBA },
    }
};

struct TextureAtlasInfo health_atlas_info = {
    .offset             = 0x201410,
    .numUsedTextures    = 10,
    .atlasWidth         = 10*64,
    .atlasHeight        = 64,
    .texInfos = {
        { .offset = 0x233E0, .width = 32, .height = 64, .format = FORMAT_RGBA },
        { .offset = 0x243E0, .width = 32, .height = 64, .format = FORMAT_RGBA },
        { .offset = 0x253E0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 0x25BE0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 0x263E0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 0x26BE0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 0x273E0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 0x27BE0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 0x283E0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        { .offset = 0x28BE0, .width = 32, .height = 32, .format = FORMAT_RGBA },
    }
};


static void blt_rgba_to_atlas( rgba *img, int i, struct TextureAtlasInfo* atlasInfo, uint8_t *outTexture )
{
    for( int iy = 0; iy < atlasInfo->texInfos[i].height; ++iy )
    for( int ix = 0; ix < atlasInfo->texInfos[i].width; ++ix )
    {
        int o = (ix-1 + atlasInfo->atlasHeight * i) + iy * atlasInfo->atlasWidth;
        int q = ix + iy * atlasInfo->texInfos[i].width;
        if(o < 0) continue;
        outTexture[4*o + 0] = img[q].red;
        outTexture[4*o + 1] = img[q].green;
        outTexture[4*o + 2] = img[q].blue;
        outTexture[4*o + 3] = img[q].alpha;
    }
}

static void blt_ia_to_atlas( ia *img, int i, struct TextureAtlasInfo* atlasInfo, uint8_t *outTexture )
{
    for( int iy = 0; iy < atlasInfo->texInfos[i].height; ++iy )
    for( int ix = 0; ix < atlasInfo->texInfos[i].width; ++ix )
    {
        int o = (ix-1 + atlasInfo->atlasHeight * i) + iy * atlasInfo->atlasWidth;
        int q = ix + iy * atlasInfo->texInfos[i].width;
        if(o < 0) continue;
        outTexture[4*o + 0] = img[q].intensity;
        outTexture[4*o + 1] = img[q].intensity;
        outTexture[4*o + 2] = img[q].intensity;
        outTexture[4*o + 3] = img[q].alpha;
    }
}

void load_textures_from_rom( uint8_t *rom, struct TextureAtlasInfo* atlasInfo, uint8_t *outTexture )
{
    memset( outTexture, 0, 4 * atlasInfo->atlasWidth * atlasInfo->atlasHeight );

    mio0_header_t head;
    uint8_t *in_buf = rom + atlasInfo->offset;

    mio0_decode_header( in_buf, &head );
    uint8_t *out_buf = malloc( head.dest_size );
    mio0_decode( in_buf, out_buf, NULL );

    for( int i = 0; i < atlasInfo->numUsedTextures; i++ )
    {
        uint8_t *raw = out_buf + atlasInfo->texInfos[i].offset;
        if(atlasInfo->texInfos[i].format == FORMAT_RGBA) 
        {
            rgba *img = raw2rgba( raw, atlasInfo->texInfos[i].width, atlasInfo->texInfos[i].height, 16 );
            blt_rgba_to_atlas( img, i, atlasInfo, outTexture );
            free( img );
        } else if(atlasInfo->texInfos[i].format == FORMAT_IA)  {
            ia *img = raw2ia( raw, atlasInfo->texInfos[i].width, atlasInfo->texInfos[i].height, 16 );
            blt_ia_to_atlas( img, i, atlasInfo, outTexture );
            free( img );
        }
    }

    free( out_buf );
}

void load_mario_textures_from_rom( uint8_t *rom, uint8_t *outTexture )
{
    load_textures_from_rom(rom, &mario_atlas_info, outTexture);
}

void load_coin_textures_from_rom( uint8_t *rom, uint8_t *outTexture )
{
    load_textures_from_rom(rom, &coin_atlas_info, outTexture);
}

void load_ui_textures_from_rom( uint8_t *rom, uint8_t *outTexture )
{
    load_textures_from_rom(rom, &ui_atlas_info, outTexture);
}

void load_health_textures_from_rom( uint8_t *rom, uint8_t *outTexture )
{
    load_textures_from_rom(rom, &health_atlas_info, outTexture);
}