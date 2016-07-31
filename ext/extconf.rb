#!ruby

require "mkmf"

find_header "zstd.h", "$(srcdir)/../contrib/zstd/common" or abort "can't find ``zstd.h''"
find_header "zdict.h", "$(srcdir)/../contrib/zstd/dictBuilder" or abort "can't find ``zdict.h''"
find_header "zstd_legacy.h", "$(srcdir)/../contrib/zstd/legacy" or abort "can't find ``zstd_legacy.h''"

#dir = File.dirname(__FILE__).gsub(/[\[\{\?\*]/, "[\\0]")
#filepattern = "{.,../contrib/zstd}/**/*.c"
#target = File.join(dir, filepattern)
#files = Dir.glob(target).sort.map { |n| File.basename n }
#$srcs = files
#$VPATH.push "$(srcdir)/../contrib/zstd", "$(srcdir)/../contrib/zstd/legacy"

if RbConfig::CONFIG["arch"] =~ /mingw/
  $LDFLAGS << " -static-libgcc"
end

create_makefile File.join(RUBY_VERSION.slice(/\d+\.\d+/), "extzstd")
