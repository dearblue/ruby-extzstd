#!ruby
#vim: set fileencoding:utf-8

require "mkmf"

dir = File.dirname(__FILE__).gsub(/[\[\{\?\*]/, "[\\0]")
filepattern = "{.,../contrib/zstd}/*.c"
target = File.join(dir, filepattern)
files = Dir.glob(target).map { |n| File.basename n }
files.reject! { |n| "/contrib/zstd/fse.c".include?(n) }
$srcs = files

$VPATH.push "$(srcdir)/../contrib/zstd"

find_header "zstd.h", "$(srcdir)/../contrib/zstd"

create_makefile("extzstd")
