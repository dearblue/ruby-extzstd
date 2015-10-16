# encoding:utf-8 ;

# extzstd - ruby bindings for Zstd (Zstandard)

This is ruby bindings for compression library
[Zstd (https://github.com/Cyan4973/zstd)](https://github.com/Cyan4973/zstd).

  * package name: extzstd
  * author: dearblue (mailto:dearblue@users.osdn.me)
  * version: 0.0.2.CONCEPT
  * software quality: EXPERIMENTAL
  * license: 2-clause BSD License
  * report issue to: https://osdn.jp/projects/rutsubo/ticket/
  * dependency ruby: ruby-2.0+
  * dependency ruby gems: (none)
  * dependency library: (none)
  * bundled external libraries:
      * zstd-0.1.2 (https://github.com/Cyan4973/zstd/tree/zstd-0.1.2)


## ***WARNING***

Zstd data format compatibility is not guaranteed in future versions
(There is a possibility that it becomes impossible to future use).

Written in [zstd/README.md](https://github.com/Cyan4973/zstd/blob/zstd-0.1.2/README.md):

>   Zstd has not yet reached "stable" status. Specifically, it doesn't
>   guarantee yet that its current compressed format will remain stable
>   and supported in future versions.


## HOW TO USE

### basic usage (one pass encode/decode)

``` ruby:ruby
# First, load library
require "extzstd"

# Prepair data
source = "sample data..." * 50

# Directly compression
encdata = Zstd.encode(source)
puts "encdata.bytesize=#{encdata.bytesize}"

# Directly decompression
maxdestsize = source.bytesize
decdata = Zstd.decode(encdata, maxdestsize)
puts "decdata.bytesize=#{decdata.bytesize}"

# Comparison source and decoded data
p source == decdata # => true
```

----

[a stub]
