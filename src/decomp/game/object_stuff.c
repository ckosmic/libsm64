/**
 * This file just hacks a bunch of stuff together to get a valid Object and GraphNode for Mario
 */
#include <stdlib.h>
#include "mario.h"
#include "../shim.h"
#include "../engine/math_util.h"
#include "../include/object_fields.h"
#include "object_stuff.h"
#include "../../debug_print.h"
#include "../../obj_pool.h"

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

// The discriminant for different types of geo nodes
#define GRAPH_NODE_TYPE_ROOT                  0x001
#define GRAPH_NODE_TYPE_ORTHO_PROJECTION      0x002
#define GRAPH_NODE_TYPE_PERSPECTIVE          (0x003 | GRAPH_NODE_TYPE_FUNCTIONAL)
#define GRAPH_NODE_TYPE_MASTER_LIST           0x004
#define GRAPH_NODE_TYPE_START                 0x00A
#define GRAPH_NODE_TYPE_LEVEL_OF_DETAIL       0x00B
#define GRAPH_NODE_TYPE_SWITCH_CASE          (0x00C | GRAPH_NODE_TYPE_FUNCTIONAL)
#define GRAPH_NODE_TYPE_CAMERA               (0x014 | GRAPH_NODE_TYPE_FUNCTIONAL)
#define GRAPH_NODE_TYPE_TRANSLATION_ROTATION  0x015
#define GRAPH_NODE_TYPE_TRANSLATION           0x016
#define GRAPH_NODE_TYPE_ROTATION              0x017
#define GRAPH_NODE_TYPE_OBJECT                0x018
#define GRAPH_NODE_TYPE_ANIMATED_PART         0x019
#define GRAPH_NODE_TYPE_BILLBOARD             0x01A
#define GRAPH_NODE_TYPE_DISPLAY_LIST          0x01B
#define GRAPH_NODE_TYPE_SCALE                 0x01C
#define GRAPH_NODE_TYPE_SHADOW                0x028
#define GRAPH_NODE_TYPE_OBJECT_PARENT         0x029
#define GRAPH_NODE_TYPE_GENERATED_LIST       (0x02A | GRAPH_NODE_TYPE_FUNCTIONAL)
#define GRAPH_NODE_TYPE_BACKGROUND           (0x02C | GRAPH_NODE_TYPE_FUNCTIONAL)
#define GRAPH_NODE_TYPE_HELD_OBJ             (0x02E | GRAPH_NODE_TYPE_FUNCTIONAL)
#define GRAPH_NODE_TYPE_CULLING_RADIUS        0x02F

/* respawnInfoType */
#define RESPAWN_INFO_TYPE_NULL 0
#define RESPAWN_INFO_TYPE_32   1
#define RESPAWN_INFO_TYPE_16   2

#define GRAPH_RENDER_ACTIVE         (1 << 0)
#define GRAPH_RENDER_CHILDREN_FIRST (1 << 1)
#define GRAPH_RENDER_BILLBOARD      (1 << 2)
#define GRAPH_RENDER_Z_BUFFER       (1 << 3)
#define GRAPH_RENDER_INVISIBLE      (1 << 4)
#define GRAPH_RENDER_HAS_ANIMATION  (1 << 5)

struct ObjPool s_object_collider_pool = { 0, 0 };

static Vec3f gVec3fZero = { 0.0f, 0.0f, 0.0f };
static Vec3s gVec3sZero = { 0, 0, 0 };

static struct Object *try_allocate_object(void) {
    struct ObjectNode *nextObj;
    nextObj = (struct ObjectNode *) malloc(sizeof(struct Object));
    nextObj->prev = NULL;
    nextObj->next = NULL;
    return (struct Object *) nextObj;
}

static struct Object *allocate_object(void) {
    s32 i;
    struct Object *obj = try_allocate_object();

    // Initialize object fields

    obj->activeFlags = ACTIVE_FLAG_ACTIVE | ACTIVE_FLAG_UNK8;
    obj->parentObj = obj;
    obj->prevObj = NULL;
    obj->collidedObjInteractTypes = 0;
    obj->numCollidedObjs = 0;

    for (i = 0; i < 0x50; i++) {
        obj->rawData.asS32[i] = 0;
#if !IS_64_BIT
        obj->rawData.asVoidPtr[i] = NULL;
#else
        obj->ptrData.asVoidPtr[i] = NULL;
#endif
    }

    obj->unused1 = 0;
    obj->bhvStackIndex = 0;
    obj->bhvDelayTimer = 0;

    obj->hitboxRadius = 37.0f;    // Override directly for Mario
    obj->hitboxHeight = 160.0f;   // 
    obj->hurtboxRadius = 0.0f;
    obj->hurtboxHeight = 0.0f;
    obj->hitboxDownOffset = 0.0f;
    obj->unused2 = 0;

    obj->platform = NULL;
    obj->collisionData = NULL;
    obj->oIntangibleTimer = -1;
    obj->oDamageOrCoinValue = 0;
    obj->oHealth = 2048;

    obj->oCollisionDistance = 1000.0f;
    if (gCurrLevelNum == LEVEL_TTC) {
        obj->oDrawingDistance = 2000.0f;
    } else {
        obj->oDrawingDistance = 4000.0f;
    }

    mtxf_identity(obj->transform);

    obj->respawnInfoType = RESPAWN_INFO_TYPE_NULL;
    obj->respawnInfo = NULL;

    obj->oDistanceToMario = 19000.0f;
    obj->oRoom = -1;

    obj->header.gfx.node.flags &= ~GRAPH_RENDER_INVISIBLE;
    obj->header.gfx.pos[0] = -10000.0f;
    obj->header.gfx.pos[1] = -10000.0f;
    obj->header.gfx.pos[2] = -10000.0f;
    obj->header.gfx.throwMatrix = NULL;

    return obj;
}

static struct Object *create_object(void) {
    struct Object *obj;
    obj = allocate_object();
    obj->curBhvCommand = NULL;
    obj->behavior = NULL;
    return obj;
}

static void geo_obj_init(struct GraphNodeObject *graphNode, void *sharedChild, Vec3f pos, Vec3s angle) {
    vec3f_set(graphNode->scale, 1.0f, 1.0f, 1.0f);
    vec3f_copy(graphNode->pos, pos);
    vec3s_copy(graphNode->angle, angle);

    graphNode->sharedChild = sharedChild;
    graphNode->unk4C = 0;
    graphNode->throwMatrix = NULL;
    graphNode->animInfo.curAnim = NULL;

    graphNode->node.flags |= GRAPH_RENDER_ACTIVE;
    graphNode->node.flags &= ~GRAPH_RENDER_INVISIBLE;
    graphNode->node.flags |= GRAPH_RENDER_HAS_ANIMATION;
    graphNode->node.flags &= ~GRAPH_RENDER_BILLBOARD;
}

static struct Object *spawn_object_at_origin(void) {
    struct Object *obj;
    obj = create_object();

    obj->parentObj = NULL;
    obj->header.gfx.areaIndex = 0;
    obj->header.gfx.activeAreaIndex = 0;
    geo_obj_init((struct GraphNodeObject *) &obj->header.gfx, NULL, gVec3fZero, gVec3sZero);

    return obj;
}

/**
 * Copy position, velocity, and angle variables from MarioState to the Mario
 * object.
 */
static void copy_mario_state_to_object(void) {
    s32 i = 0;
    // L is real
    if (gCurrentObject != gMarioObject) {
        i += 1;
    }

    gCurrentObject->oVelX = gMarioState->vel[0];
    gCurrentObject->oVelY = gMarioState->vel[1];
    gCurrentObject->oVelZ = gMarioState->vel[2];

    gCurrentObject->oPosX = gMarioState->pos[0];
    gCurrentObject->oPosY = gMarioState->pos[1];
    gCurrentObject->oPosZ = gMarioState->pos[2];

    gCurrentObject->oMoveAnglePitch = gCurrentObject->header.gfx.angle[0];
    gCurrentObject->oMoveAngleYaw = gCurrentObject->header.gfx.angle[1];
    gCurrentObject->oMoveAngleRoll = gCurrentObject->header.gfx.angle[2];

    gCurrentObject->oFaceAnglePitch = gCurrentObject->header.gfx.angle[0];
    gCurrentObject->oFaceAngleYaw = gCurrentObject->header.gfx.angle[1];
    gCurrentObject->oFaceAngleRoll = gCurrentObject->header.gfx.angle[2];

    gCurrentObject->oAngleVelPitch = gMarioState->angleVel[0];
    gCurrentObject->oAngleVelYaw = gMarioState->angleVel[1];
    gCurrentObject->oAngleVelRoll = gMarioState->angleVel[2];
}

struct Object *hack_allocate_mario(void)
{
    return spawn_object_at_origin();
}

/**
 * Mario's primary behavior update function.
 */
void bhv_mario_update(void) {
    u32 particleFlags = 0;
//  s32 i;

	gCurrentObject = gMarioObject;
    particleFlags = execute_mario_action(gCurrentObject);
    gCurrentObject->oMarioParticleFlags = particleFlags;

    // Mario code updates MarioState's versions of position etc, so we need
    // to sync it with the Mario object
    copy_mario_state_to_object();

//  i = 0;
//  while (sParticleTypes[i].particleFlag != 0) {
//      if (particleFlags & sParticleTypes[i].particleFlag) {
//          spawn_particle(sParticleTypes[i].activeParticleFlag, sParticleTypes[i].model,
//                         sParticleTypes[i].behavior);
//      }

//      i++;
//  }
}

void create_transformation_from_matrices(Mat4 a0, Mat4 a1, Mat4 a2) {
    f32 spC, sp8, sp4;

    spC = a2[3][0] * a2[0][0] + a2[3][1] * a2[0][1] + a2[3][2] * a2[0][2];
    sp8 = a2[3][0] * a2[1][0] + a2[3][1] * a2[1][1] + a2[3][2] * a2[1][2];
    sp4 = a2[3][0] * a2[2][0] + a2[3][1] * a2[2][1] + a2[3][2] * a2[2][2];

    a0[0][0] = a1[0][0] * a2[0][0] + a1[0][1] * a2[0][1] + a1[0][2] * a2[0][2];
    a0[0][1] = a1[0][0] * a2[1][0] + a1[0][1] * a2[1][1] + a1[0][2] * a2[1][2];
    a0[0][2] = a1[0][0] * a2[2][0] + a1[0][1] * a2[2][1] + a1[0][2] * a2[2][2];

    a0[1][0] = a1[1][0] * a2[0][0] + a1[1][1] * a2[0][1] + a1[1][2] * a2[0][2];
    a0[1][1] = a1[1][0] * a2[1][0] + a1[1][1] * a2[1][1] + a1[1][2] * a2[1][2];
    a0[1][2] = a1[1][0] * a2[2][0] + a1[1][1] * a2[2][1] + a1[1][2] * a2[2][2];

    a0[2][0] = a1[2][0] * a2[0][0] + a1[2][1] * a2[0][1] + a1[2][2] * a2[0][2];
    a0[2][1] = a1[2][0] * a2[1][0] + a1[2][1] * a2[1][1] + a1[2][2] * a2[1][2];
    a0[2][2] = a1[2][0] * a2[2][0] + a1[2][1] * a2[2][1] + a1[2][2] * a2[2][2];

    a0[3][0] = a1[3][0] * a2[0][0] + a1[3][1] * a2[0][1] + a1[3][2] * a2[0][2] - spC;
    a0[3][1] = a1[3][0] * a2[1][0] + a1[3][1] * a2[1][1] + a1[3][2] * a2[1][2] - sp8;
    a0[3][2] = a1[3][0] * a2[2][0] + a1[3][1] * a2[2][1] + a1[3][2] * a2[2][2] - sp4;

    a0[0][3] = 0;
    a0[1][3] = 0;
    a0[2][3] = 0;
    a0[3][3] = 1.0f;
}
void obj_update_pos_from_parent_transformation(Mat4 a0, struct Object *a1) {
    f32 spC = a1->oParentRelativePosX;
    f32 sp8 = a1->oParentRelativePosY;
    f32 sp4 = a1->oParentRelativePosZ;

    a1->oPosX = spC * a0[0][0] + sp8 * a0[1][0] + sp4 * a0[2][0] + a0[3][0];
    a1->oPosY = spC * a0[0][1] + sp8 * a0[1][1] + sp4 * a0[2][1] + a0[3][1];
    a1->oPosZ = spC * a0[0][2] + sp8 * a0[1][2] + sp4 * a0[2][2] + a0[3][2];
}
void obj_set_gfx_pos_from_pos(struct Object *obj) {
    obj->header.gfx.pos[0] = obj->oPosX;
    obj->header.gfx.pos[1] = obj->oPosY;
    obj->header.gfx.pos[2] = obj->oPosZ;
}

int detect_object_hitbox_overlap(struct Object *a, struct SM64ObjectCollider *collider) {
    f32 sp3C = a->oPosY - a->hitboxDownOffset;  // Object hitbox bottom
    f32 sp38 = collider->position[1] - 0;       // Collider bottom
    f32 dx = a->oPosX - collider->position[0];
    f32 dz = a->oPosZ - collider->position[2];
    f32 collisionRadius = a->hitboxRadius + collider->radius;
    f32 distance = sqrtf(dx * dx + dz * dz);

    if (distance < collisionRadius) {
        f32 sp20 = a->hitboxHeight + sp3C;      // Object hitbox height
        f32 sp1C = collider->height + sp38;     // Collider height

        if (sp3C > sp1C) {
            return 0;
        }
        if (sp20 < sp38) {
            return 0;
        }
        
        dx = collider->position[0] - a->oPosX;
        dz = collider->position[2] - a->oPosZ;
        s16 angle = atan2s(dx, dz);

        f32 radius = a->hitboxRadius;
        f32 otherRadius = collider->radius;
        f32 relativeRadius = radius / (radius + otherRadius);

        f32 newCenterX = a->oPosX + dx * relativeRadius;
        f32 newCenterZ = a->oPosZ + dz * relativeRadius;

        a->oPosX = newCenterX - radius * coss(angle);
        a->oPosZ = newCenterZ - radius * sins(angle);

        //collider->position[0] = newCenterX + otherRadius * coss(angle);
        //collider->position[2] = newCenterZ + otherRadius * sins(angle);

        return 1;
    }

    return 0;
}
void resolve_object_collisions() {
    if(gMarioState->flags & 0x00000002) return;
    for(int i = 0; i < s_object_collider_pool.size; i++) {
        if(s_object_collider_pool.objects[i] != NULL) {
            detect_object_hitbox_overlap(gMarioObject, s_object_collider_pool.objects[i]);
            gMarioState->pos[0] = gMarioObject->oPosX;
            gMarioState->pos[2] = gMarioObject->oPosZ;
            vec3f_copy(gMarioState->marioObj->header.gfx.pos, gMarioState->pos);
        }
    }
}
uint32_t add_object_collider(struct SM64ObjectCollider *collider) {
    int32_t objId = obj_pool_alloc_index(&s_object_collider_pool, sizeof(struct SM64ObjectCollider));
    struct SM64ObjectCollider *colInstance = s_object_collider_pool.objects[objId];
    vec3f_copy(colInstance->position, collider->position);
    colInstance->height = collider->height;
    colInstance->radius = collider->radius;
    return objId;
}
void move_object_collider(uint32_t objId, float x, float y, float z) {
    if(objId >= s_object_collider_pool.size || s_object_collider_pool.objects[objId] == NULL) {
        DEBUG_PRINT("Tried to move non-existent object collider with ID: %d!", objId);
        return;
    }
    struct SM64ObjectCollider *collider = s_object_collider_pool.objects[objId];
    collider->position[0] = x;
    collider->position[1] = y;
    collider->position[2] = z;
}
void delete_object_collider(uint32_t objId) {
    if(objId >= s_object_collider_pool.size || s_object_collider_pool.objects[objId] == NULL) {
        DEBUG_PRINT("Tried to delete non-existent object collider with ID: %d!", objId);
        return;
    }
    obj_pool_free_index(&s_object_collider_pool, objId);
}
void free_obj_pool() {
    obj_pool_free_all(&s_object_collider_pool);
}