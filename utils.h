#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

int random_int(int min, int max);
bool chance(float percentage);

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#endif // UTILS_H