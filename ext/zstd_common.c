#define ZSTD_LEGACY_SUPPORT 1
#define MEM_MODULE 1
#define visibility(v) visibility("hidden")

#include "../contrib/zstd/lib/common/entropy_common.c"
#include "../contrib/zstd/lib/common/error_private.c"
#include "../contrib/zstd/lib/common/fse_decompress.c"

#undef CHECK_F
#include "../contrib/zstd/lib/common/pool.c"
#include "../contrib/zstd/lib/common/threading.c"
#include "../contrib/zstd/lib/common/xxhash.c"
#include "../contrib/zstd/lib/common/zstd_common.c"
