# extzstd - ruby bindings for Zstd (Zstandard)

This is unofficial ruby bindings for the data compression library
[Zstd (Zstandard)](https://github.com/facebook/zstd).

"extzstd" is supported decompression with the legacy formats (zstd-0.1 - 0.7).

  * [HISTORY (in Japanese)](HISTORY.ja.md)
  * [Quick reference](QUICKREF.md)


## HOW TO USE

### basic usage (simply encode/decode)

``` ruby
# First, load library
require "extzstd"

# Prepair data
source = "sample data..." * 50

# simply compression
encdata = Zstd.encode(source)
puts "encdata.bytesize=#{encdata.bytesize}"

# simply decompression
decdata = Zstd.decode(encdata)
puts "decdata.bytesize=#{decdata.bytesize}"

# Comparison source and decoded data
p source == decdata # => true
```

### Streaming compression (with block)

``` ruby
outport = StringIO.new("")
Zstd.encode(outport) do |encoder|
  encoder.write "abcdefg\n"
  encoder << "hijklmn\n"
  encoder.write "opqrstu\n"
  encoder << "vwxyz\n"
end
```

### Streaming compression (without block and write to file)

``` ruby
file = File.open("sample.zst", "wb")
encoder = Zstd.encode(file)

encoder.write "abcdefg\n"
encoder << "hijklmn\n"
encoder.write "opqrstu\n"
encoder << "vwxyz\n"

encoder.close
file.close
```

### Streaming decompression (with block and read from file)

``` ruby
File.open("sample.zst", "rb") do |file|
  Zstd.decode(file) do |decoder|
    p decoder.read(8)  # => "abcdefg\n"
    p decoder.read(1)  # => "h"
    p decoder.read(2)  # => "ij"
    p decoder.read     # => "klmn\nopqrstu\nvwxyz\n"
  end
end
```


## Specification

  * package name: extzstd
  * version: 0.3
  * product quality: TECHNICAL PREVIEW, UNSTABLE
  * license: [BSD-2-clause License](LICENSE)
  * author: dearblue <mailto:dearblue@users.noreply.github.com>
  * project page: <https://github.com/dearblue/ruby-extzstd>
  * support ruby: ruby-2.3+
  * dependency ruby gems: (none)
  * dependency library: (none)
  * bundled external C library (git submodules):
      * [zstd-1.3.4](https://github.com/facebook/zstd)
        under selectable dual licensed ([BSD-3-clause License](https://github.com/facebook/zstd/blob/v1.3.4/LICENSE) and [GNU General Public License, version 2](https://github.com/facebook/zstd/blob/v1.3.4/COPYING))
        by [facebook](https://github.com/facebook)
