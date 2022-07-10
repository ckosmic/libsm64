#include "load_tex_data.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "libsm64.h"

#include "decomp/tools/libmio0.h"
#include "decomp/tools/n64graphics.h"

struct TextureAtlasInfo* mario_atlas_info;

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
    // Terrible way of storing Mario's texture atlas info
    if(mario_atlas_info == NULL && atlasInfo->offset == 0x114750 && atlasInfo->texInfos[0].offset == 144) {
        mario_atlas_info = atlasInfo;
    }

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