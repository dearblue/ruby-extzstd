#define ZSTD_LEGACY_SUPPORT 1
#define MEM_MODULE 1
#define visibility(v) visibility("hidden")

#include "../contrib/zstd/lib/dictBuilder/zdict.c"
#include "../contrib/zstd/lib/dictBuilder/divsufsort.c"

#undef DISPLAY
#undef DISPLAYLEVEL
#undef DISPLAYUPDATE
#include "../contrib/zstd/lib/dictBuilder/cover.c"
