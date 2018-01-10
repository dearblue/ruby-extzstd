#define ZSTD_LEGACY_SUPPORT 1
#define MEM_MODULE 1
#define visibility(v) visibility("hidden")

#include "../contrib/zstd/lib/compress/fse_compress.c"
#include "../contrib/zstd/lib/compress/huf_compress.c"

#undef CHECK_F
#include "../contrib/zstd/lib/compress/zstd_compress.c"
#include "../contrib/zstd/lib/compress/zstd_double_fast.c"
#include "../contrib/zstd/lib/compress/zstd_fast.c"
#include "../contrib/zstd/lib/compress/zstd_lazy.c"
#include "../contrib/zstd/lib/compress/zstd_ldm.c"
#include "../contrib/zstd/lib/compress/zstd_opt.c"
