#define ZSTD_LEGACY_SUPPORT 1
#define MEM_MODULE 1

#include "../contrib/zstd/compress/zbuff_compress.c"
#include "../contrib/zstd/compress/fse_compress.c"
#include "../contrib/zstd/compress/zstd_compress.c"
#include "../contrib/zstd/compress/huf_compress.c"
#include "../contrib/zstd/common/xxhash.c"
#include "../contrib/zstd/common/zstd_common.c"
#define FSE_abs FSE_abs__redef 
#include "../contrib/zstd/common/entropy_common.c"
#include "../contrib/zstd/common/fse_decompress.c"
#include "../contrib/zstd/decompress/zstd_decompress.c"
#include "../contrib/zstd/decompress/huf_decompress.c"
#define ZBUFF_limitCopy ZBUFF_limitCopy__redef
#include "../contrib/zstd/decompress/zbuff_decompress.c"
#include "../contrib/zstd/dictBuilder/zdict.c"
#include "../contrib/zstd/dictBuilder/divsufsort.c"
