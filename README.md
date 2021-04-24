# extzstd - ruby bindings for Zstd (Zstandard)

This is unofficial ruby bindings for the data compression library
[Zstd (Zstandard)](https://github.com/facebook/zstd).

"extzstd" is supported decompression with the legacy formats (zstd-0.1 - 0.7).

  - [HISTORY (in Japanese)](HISTORY.ja.md)
  - [Quick reference](QUICKREF.md)


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


## Support `Ractor` (Ruby3 feature)

Ruby3 の `Ractor` に対応しています。

```ruby
require "extzstd"

using Zstd

p Ractor.new {
  Ractor.yield ("abcdefg" * 9).to_zstd, move: true
}.take
```

## Specification

  - package name: extzstd
  - project page: <https://github.com/dearblue/ruby-extzstd>
  - version: 0.3.1
  - product quality: TECHNICAL PREVIEW, UNSTABLE
  - license: [2 clause BSD License](LICENSE)
  - author: dearblue
  - support ruby: ruby-2.5+
  - dependency ruby gems: (none)
  - dependency library: (none)
  - bundled external C library (git submodules):
      - [zstd-1.4.9](https://github.com/facebook/zstd/blob/v1.4.9)
        under selectable dual licensed ([3 clause BSD License](https://github.com/facebook/zstd/blob/v1.4.9/LICENSE) and [GNU General Public License, version 2](https://github.com/facebook/zstd/blob/v1.4.9/COPYING))
        by [facebook](https://github.com/facebook)
