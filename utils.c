#include "utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int random_int(int min, int max) {
    if (max < min)
        return min;
    return rand() % (max - min + 1) + min;
}

bool chance(float percentage) {
    return ((float)rand() / (float)RAND_MAX) < percentage;
}

void shuffle_array(void *array, int n, size_t size) {
    char *arr = (char *)array; // Use char* for pointer arithmetic

    if (n > 1) {
        for (int i = n - 1; i > 0; i--) {
            int j = random_int(0, i);

            // Swap elements byte-by-byte
            char *p1 = arr + (size_t)i * size;
            char *p2 = arr + (size_t)j * size;
            for (size_t k = 0; k < size; k++) {
                char t = p1[k];
                p1[k] = p2[k];
                p2[k] = t;
            }
        }
    }
}

void filter_non_alpha(char *s) {
    char *dst = s;
    for (; *s; s++) {
        if (isalpha((unsigned char)*s)) {
            *dst++ = *s;
        }
    }
    *dst = '\0';
}

