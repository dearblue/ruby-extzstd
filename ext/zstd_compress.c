#include "libzstd_conf.h"

#include "../contrib/zstd/lib/compress/fse_compress.c"
#include "../contrib/zstd/lib/compress/huf_compress.c"

#undef CHECK_F
#include "../contrib/zstd/lib/compress/zstd_compress.c"
#include "../contrib/zstd/lib/compress/zstd_compress_literals.c"
#include "../contrib/zstd/lib/compress/zstd_compress_sequences.c"
#include "../contrib/zstd/lib/compress/zstd_double_fast.c"
#include "../contrib/zstd/lib/compress/zstd_fast.c"
#include "../contrib/zstd/lib/compress/zstd_lazy.c"
#include "../contrib/zstd/lib/compress/zstd_ldm.c"
#include "../contrib/zstd/lib/compress/zstd_opt.c"
#include "../contrib/zstd/lib/compress/hist.c"
