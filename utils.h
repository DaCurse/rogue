#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stddef.h>

inline int random_int(int min, int max);
inline bool chance(float percentage);
void shuffle_array(void *array, int n, size_t size);
void filter_non_alpha(char *s);

#if __STDC_VERSION__ < 199901L
#if __GNUC__ >= 2
#define __func__ __FUNCTION__
#else
#define __func__ "<unknown>"
#endif
#endif

#ifndef TODO
#define TODO()                                                                 \
    do {                                                                       \
        fprintf(stderr, "TODO: %s:%d: %s\n", __FILE__, __LINE__, __func__);    \
        abort();                                                               \
    } while (0)
#endif

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#endif // UTILS_H
