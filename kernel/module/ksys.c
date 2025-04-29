#include "core/export.h"
#include "std/stddef.h"
#include "lib/string.h"


struct ksym *alloc_ksym(struct kernel_symbol *sym)
{
    struct ksym *ks = kmalloc(sizeof(struct ksym), 0);
    ks->ksp = sym;
    INIT_HASH_NODE(&ks->node);
    return ks;
}

uint32 ksym_hash(struct ksym *ks)
{
    return strhash(ks->ksp->name);
}
