#define ZSTD_LEGACY_SUPPORT 1
#define MEM_MODULE 1
#define visibility(v) visibility("hidden")

#include "../contrib/zstd/lib/decompress/zstd_decompress.c"
#include "../contrib/zstd/lib/decompress/zstd_decompress_block.c"

#undef CHECK_F
#include "../contrib/zstd/lib/decompress/huf_decompress.c"
#include "../contrib/zstd/lib/decompress/zstd_ddict.c"
