#ifndef ITEM_H
#define ITEM_H

#include "ecs_base.h"

#define MAX_INVENTORY (10)

typedef struct {
    Entity items[MAX_INVENTORY];
    int16_t count;
} Inventory;

typedef enum {
    SLOT_HAND,
    SLOT_OFF_HAND,
    SLOT_BODY,

    SLOT_COUNT,
} EquipmentSlot;

static const char *equipment_slot_names[SLOT_COUNT] = {
    [SLOT_HAND] = "Hand",
    [SLOT_OFF_HAND] = "Off-Hand",
    [SLOT_BODY] = "Body",
};

static const char *equipment_slot_prepositions[SLOT_COUNT] = {
    [SLOT_HAND] = "in",
    [SLOT_OFF_HAND] = "in",
    [SLOT_BODY] = "on",
};

typedef struct {
    Entity equipped[SLOT_COUNT];
} Equipment;

typedef enum {
    EQUIPMENT_CONSUMABLE,
    EQUIPMENT_WEAPON,
    EQUIPMENT_ARMOR,
} EquipmentType;

typedef union {
    struct {
        int16_t attack_bonus;
    } weapon;

    struct {
        int16_t defense_bonus;
    } armor;
} EquipmentData;

typedef struct {
    EquipmentType type;
    EquipmentSlot slot;
    EquipmentData data;
} Equippable;

typedef enum {
    CONSUMABLE_HEALING_POTION,
    CONSUMABLE_INVISIBILITY_POTION,
} ConsumableType;

typedef union {
    struct {
        int16_t heal_amount;
    } healing_potion;

    struct {
        int16_t duration_turns;
    } invisibility_potion;
} ConsumableData;

typedef struct {
    ConsumableType type;
    ConsumableData effect;
} Consumable;

void reset_equipment(Equipment *eq);
void world_add_equipment(World *w, Entity e, Equipment eq);
void world_add_consumable(World *w, Entity e, Consumable c);
void world_add_equippable(World *w, Entity e, Equippable eqp);
void world_equip_item(World *w, Entity wearer, Entity item);
void world_unequip_item(World *w, Entity wearer, EquipmentSlot slot);
bool world_add_item_to_inventory(World *w, Entity item);

#endif // ITEM_H
