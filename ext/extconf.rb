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
else
  if try_compile(<<-"VISIBILITY")
__attribute__ ((visibility("hidden"))) int conftest(void) { return 0; }
  VISIBILITY
    if try_cflags("-fvisibility=hidden")
      $CFLAGS << " -fvisibility=hidden"
      $defs << %('-DRBEXT_API=__attribute__ ((visibility("default")))')
    end
  end
end

create_makefile File.join(RUBY_VERSION.slice(/\d+\.\d+/), "extzstd")
