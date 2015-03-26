# encoding:utf-8 ;

# extzstd - ruby binding for Zstandard (zstd)

This is ruby binding for compression library
[Zstd (https://github.com/Cyan4973/zstd)](https://github.com/Cyan4973/zstd).

*   PACKAGE NAME: extzstd
*   AUTHOR: dearblue <dearblue@users.sourceforge.jp>
*   VERSION: 0.0.1.CONCEPT
*   LICENSING: 2-clause BSD License
*   REPORT ISSUE TO: <http://sourceforge.jp/projects/rutsubo/ticket/>
*   DEPENDENCY RUBY: ruby-2.0+
*   DEPENDENCY RUBY GEMS: (none)
*   DEPENDENCY LIBRARY: (none)
*   BUNDLED EXTERNAL LIBRARIES:
    *   zstd <https://github.com/Cyan4973/zstd>
        (commit-e739b273f95902b7616e11338a4ef04bebc9d07b (Mon Feb 9 01:53:12 2015 +0100))


## HOW TO USE

### Simply process

``` ruby:ruby
# First, load library
require "extzstd"

# Prepair data
source = "sample data..." * 50

# Directly compression
encdata = Zstd.encode(source)
puts "encdata.bytesize=#{encdata.bytesize}"

# Directly decompression
maxdestsize = source.bytesize # MUST BE ORIGINAL SIZE OR MORE! If given a smaller size, crash ruby interpreter.
decdata = Zstd.decode(encdata, maxdestsize)
puts "decdata.bytesize=#{decdata.bytesize}"

# Comparison source and decoded data
p source == decdata # => true
```

----

[a stub]
