#ifndef OBJECT_STUFF_H
#define OBJECT_STUFF_H

#include "../include/types.h"
#include "../../libsm64.h"

/* activeFlags */
#define ACTIVE_FLAG_DEACTIVATED            0         // 0x0000
#define ACTIVE_FLAG_ACTIVE                 (1 <<  0) // 0x0001
#define ACTIVE_FLAG_FAR_AWAY               (1 <<  1) // 0x0002
#define ACTIVE_FLAG_UNK2                   (1 <<  2) // 0x0004
#define ACTIVE_FLAG_IN_DIFFERENT_ROOM      (1 <<  3) // 0x0008
#define ACTIVE_FLAG_UNIMPORTANT            (1 <<  4) // 0x0010
#define ACTIVE_FLAG_INITIATED_TIME_STOP    (1 <<  5) // 0x0020
#define ACTIVE_FLAG_MOVE_THROUGH_GRATE     (1 <<  6) // 0x0040
#define ACTIVE_FLAG_DITHERED_ALPHA         (1 <<  7) // 0x0080
#define ACTIVE_FLAG_UNK8                   (1 <<  8) // 0x0100
#define ACTIVE_FLAG_UNK9                   (1 <<  9) // 0x0200
#define ACTIVE_FLAG_UNK10                  (1 << 10) // 0x0400

struct Object *hack_allocate_mario(void);
void bhv_mario_update(void);
void create_transformation_from_matrices(Mat4 a0, Mat4 a1, Mat4 a2);
void obj_update_pos_from_parent_transformation(Mat4 a0, struct Object *a1);
void obj_set_gfx_pos_from_pos(struct Object *obj);

void resolve_object_collisions();
uint32_t add_object_collider(struct SM64ObjectCollider* collider);
void move_object_collider(uint32_t objId, float x, float y, float z);
void delete_object_collider(uint32_t objId);
void free_obj_pool();

#endif