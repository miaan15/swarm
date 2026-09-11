#pragma once

#include "action.h"
#include "define.h"
#include "effect.h"

// FUNCTIONS
// =============================================================================
extern void (*action_handle_fn)(action);
extern void (*effect_handle_fn)(effect *);

// ENTITY DATA
// =============================================================================
// NPC
struct npc_entity_data {
    // stats
    u32 races;
    bool female;

    // runtime
};

// INTERACTABLE
struct interactable_entity_data {

};

// =============================================================================
typedef struct {
    u32 type;

    union {
        struct npc_entity_data npc;
        struct interactable_entity_data interactable;
    };
} entity_data;
