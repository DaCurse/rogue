#include <stdlib.h>

#include "utils.h"

int random_int(int min, int max) { return rand() % (max - min + 1) + min; }

bool chance(float percentage) {
    return (rand() / (float)RAND_MAX) < percentage;
}