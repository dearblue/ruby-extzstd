#!ruby
#vim: set fileencoding:utf-8

require "mkmf"

dir = File.dirname(__FILE__).gsub(/[\[\{\?\*]/, "[\\0]")
filepattern = "{.,../contrib/zstd}/*.c"
target = File.join(dir, filepattern)
files = Dir.glob(target).map { |n| File.basename n }
## reject fse.c, because it's included in zstd.c
files.reject! { |n| "/contrib/zstd/fse.c".include?(n) }
$srcs = files

$VPATH.push "$(srcdir)/../contrib/zstd"

find_header "zstd.h", "$(srcdir)/../contrib/zstd" or abort 1

if RbConfig::CONFIG["arch"] =~ /mingw/
  $LDFLAGS << " -static-libgcc"
end

create_makefile("extzstd")
