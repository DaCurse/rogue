
#include "item.h"

#include <stdbool.h>

#include "world.h"

void reset_equipment(Equipment *eq) {
    for (size_t i = 0; i < SLOT_COUNT; i++) {
        eq->equipped[i] = INVALID_ENTITY;
    }
}

void world_add_equipment(World *w, Entity e, Equipment eq) {
    w->equipment[e] = eq;
    w->has[e].equipment = true;
}

void world_add_consumable(World *w, Entity e, Consumable c) {
    w->consumables[e] = c;
    w->has[e].consumable = true;
}

void world_add_equippable(World *w, Entity e, Equippable eqp) {
    w->equippables[e] = eqp;
    w->has[e].equippable = true;
}

void world_equip_item(World *w, Entity wearer, Entity item) {
    // TODO: Error handling?
    if (!w->has[wearer].equipment)
        return;
    if (!w->has[item].equippable)
        return;

    Equippable *eqp = &w->equippables[item];
    Equipment *eq = &w->equipment[wearer];

    if (eq->equipped[eqp->slot] != INVALID_ENTITY) {
        return;
    }

    world_logf(w, "You equip the %s %s your %s.", w->names[item].name,
               equipment_slot_prepositions[eqp->slot],
               equipment_slot_names[eqp->slot]);

    eq->equipped[eqp->slot] = item;
}

void world_unequip_item(World *w, Entity wearer, EquipmentSlot slot) {
    if (!w->has[wearer].equipment)
        return;

    Equipment *eq = &w->equipment[wearer];
    eq->equipped[slot] = INVALID_ENTITY;
}

bool world_add_item_to_inventory(World *w, Entity item) {
    Inventory *inv = &w->player_inventory;
    if (inv->count >= MAX_INVENTORY)
        return false;

    inv->items[inv->count++] = item;
    return true;
}
