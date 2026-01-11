#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stddef.h>

int random_int(int min, int max);
bool chance(float percentage);
void shuffle_array(void *array, int n, size_t size);
void filter_non_alpha(char *s);

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#endif // UTILS_H
