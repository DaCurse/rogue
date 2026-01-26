#include "consumable.h"

#include "utils.h"
#include "world.h"

static void use_healing_potion(World *w, Entity user, Entity item) {
    if (!w->has[user].combat_stats)
        return;

    Consumable *consumable = &w->consumables[item];
    CombatStats *cs = &w->combat_stats[user];
    int16_t heal_amount = consumable->effect.healing_potion.heal_amount;
    cs->hp = MIN(cs->max_hp, cs->hp + heal_amount);
    world_logf(w, "%s heals for %d HP.", w->names[user].name, heal_amount);
}

static void use_invisibility_potion(World *w, Entity user, Entity item) {
    UNUSED(w);
    UNUSED(user);
    UNUSED(item);
    TODO();
}

void world_use_consumable(World *w, Entity user, Entity item) {
    if (!w->has[item].consumable)
        return;
    if (!w->has[user].equipment ||
        w->equipment[user].equipped[SLOT_OFF_HAND] != item)
        return;

    Consumable *consumable = &w->consumables[item];
    switch (consumable->type) {
    case CONSUMABLE_HEALING_POTION:
        use_healing_potion(w, user, item);
        break;
    case CONSUMABLE_INVISIBILITY_POTION:
        use_invisibility_potion(w, user, item);
        break;
    default:
        return;
    }

    // Remove the consumable from the world
    memset(&w->has[item], 0, sizeof(ComponentFlags));
}
