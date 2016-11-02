#!ruby

require "mkmf"

$INCFLAGS = %w(
  -I$(srcdir)/../contrib
  -I$(srcdir)/../contrib/zstd
  -I$(srcdir)/../contrib/zstd/common
  -I$(srcdir)/../contrib/zstd/dictBuilder
  -I$(srcdir)/../contrib/zstd/legacy
).join(" ") + " #$INCFLAGS"

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
