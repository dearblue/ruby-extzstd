#include "libzstd_conf.h"

#include "../contrib/zstd/lib/dictBuilder/zdict.c"
#include "../contrib/zstd/lib/dictBuilder/divsufsort.c"

#undef DISPLAY
#undef DISPLAYLEVEL
#undef DISPLAYUPDATE
#define prime4bytes cover_prime4bytes
#include "../contrib/zstd/lib/dictBuilder/cover.c"
