#include "../include/sm64.h"
#include "../include/types.h"
#include "../include/geo_commands.h"
#include "../game/rendering_graph_node.h"
#include "../shim.h"
#include "../game/object_stuff.h"
#include "../game/behavior_actions.h"
#include "../tools/geolayout_parser.h"
#include "../tools/convUtils.h"

#define SHADOW_CIRCLE_4_VERTS 1

GeoLayout* star_geo_ptr = 0x0;

void initStarGeo(unsigned char* rom) {
   if (star_geo_ptr != 0x0) {
      return;
   }
   uintptr_t ptr = convertPtr_follow(rom, 0x16000EA0); // 16000EA0 -> star_geo
   uintptr_t data[] = {
      GEO_SHADOW(SHADOW_CIRCLE_4_VERTS, 0x9B, 100),
      GEO_OPEN_NODE(),
          GEO_SCALE(0x00, 16384),
          GEO_OPEN_NODE(),
              //GEO_DISPLAY_LIST(LAYER_OPAQUE, star_seg3_dl_0302B870),
              //GEO_DISPLAY_LIST(LAYER_ALPHA, star_seg3_dl_0302BA18),
              GEO_BRANCH(0, ptr), 
          GEO_CLOSE_NODE(),
      GEO_CLOSE_NODE(),
      GEO_END(),
   };
   // transfer memory allocated in stack to actual memory
   star_geo_ptr = malloc(sizeof(GeoLayout) * 20);
   memcpy(star_geo_ptr, data, sizeof(GeoLayout) * 20);
} 