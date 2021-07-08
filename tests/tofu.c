#include <malloc.h>
#include "tofu.h"

extern tofu_t *tofu_new()
{
    tofu_t *t = (tofu_t *)calloc(1, sizeof(tofu_t));
    return t;
}

void tofu_delete(tofu_t *t)
{
    free(t);
}

int tofu_munch(tofu_t *t)
{
    t->shoreditch += 42;
    return t->shoreditch;
}

