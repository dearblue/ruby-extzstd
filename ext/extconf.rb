#!ruby

require "mkmf"

$INCFLAGS = %w(
  -I$(srcdir)/../contrib
  -I$(srcdir)/../contrib/zstd/lib
  -I$(srcdir)/../contrib/zstd/lib/common
  -I$(srcdir)/../contrib/zstd/lib/dictBuilder
  -I$(srcdir)/../contrib/zstd/lib/legacy
).join(" ") + " #$INCFLAGS"

#dir = File.dirname(__FILE__).gsub(/[\[\{\?\*]/, "[\\0]")
#filepattern = "{.,../contrib/zstd}/**/*.c"
#target = File.join(dir, filepattern)
#files = Dir.glob(target).sort.map { |n| File.basename n }
#$srcs = files
#$VPATH.push "$(srcdir)/../contrib/zstd", "$(srcdir)/../contrib/zstd/legacy"

if RbConfig::CONFIG["arch"] =~ /mingw/i
  $LDFLAGS << " -static-libgcc" if try_ldflags("-static-libgcc")
end

create_makefile File.join(RUBY_VERSION.slice(/\d+\.\d+/), "extzstd")
