# extzstd - ruby bindings for Zstd (Zstandard)

This is unofficial ruby bindings for compression library
[Zstd (Zstandard)](https://github.com/facebook/zstd).

  * package name: extzstd
  * version: 0.1.1
  * software quality: EXPERIMENTAL
  * license: BSD-2-clause License
  * author: dearblue <mailto:dearblue@users.noreply.github.com>
  * report issue to: <https://github.com/dearblue/ruby-extzstd/issues>
  * dependency ruby: ruby-2.1+
  * dependency ruby gems: (none)
  * dependency library: (none)
  * bundled external C library:
      * zstd-1.1.4 <https://github.com/facebook/zstd/tree/v1.1.4>

        under BSD-3-clause License <https://github.com/facebook/zstd/blob/v1.1.4/LICENSE>

        by facebook <https://github.com/facebook>

"extzstd" is supported decompression with the legacy formats (zstd-0.1 - 0.7).


## Quick API Reference

  * encoder (compression)
      * ``Zstd.encode(buf, params = nil, dict: nil) -> encoded string``
      * ``Zstd.encode(outport, params = nil, dict: nil) -> an instance of Zstd::Encoder``
      * ``Zstd.encode(outport, params = nil, dict: nil) { |encoder| ... } -> block returned value``
      * ``Zstd::Encoder#write(buf) -> this instance``
      * ``Zstd::Encoder#close -> nil``

  * decoder (decompression)
      * ``Zstd.decode(zstd_buf, dict: nil) -> decoded string``
      * ``Zstd.decode(inport, dict: nil) -> an intance of Zstd::Decoder``
      * ``Zstd.decode(inport, dict: nil) { |decoder| ... } -> block returned value``
      * ``Zstd::Decoder#read(size = nil, buf = nil) -> buf``
      * ``Zstd::Decoder#close -> nil``

  * stream encoder
      * ``Zstd::StreamEncoder.new(params, dict)`` (``ZSTD_createCStream``, ``ZSTD_initCStream_advanced``)
      * ``Zstd::StreamEncoder#update(src, srcoff, dest, maxdest) -> integer as new src offset`` (``ZSTD_compressStream``)
      * ``Zstd::StreamEncoder#flush(dest, maxdest) -> dest`` (``ZSTD_flushStream``)
      * ``Zstd::StreamEncoder#end(dest, maxdest) -> dest`` (``ZSTD_endStream``)

  * stream decoder
      * ``Zstd::StreamDecoder.new(dict)`` (``ZSTD_createDStream``, ``ZSTD_initDStream_usingDict``)
      * ``Zstd::StreamDecoder#update(src, srcoff, dest, maxdest) -> integer as new src offset`` (``ZSTD_decompressStream``)

  * instant (context less) encoder/decoder
      * ``Zstd::ContextLess.encode(src, dest, maxdest, predict, params) -> dest`` (``ZSTD_compress_usingDict``, ``ZSTD_compress_advanced``)
      * ``Zstd::ContextLess.decode(src, dest, maxdest, predict) -> dest`` (``ZSTD_decompress_usingDict``)

  * dictionary (*EXPEREMENTAL*)
      * ``Zstd::Dictionary.train_from_buffer(buf, dict_capacity) -> dictionary'ed string`` (``ZDICT_trainFromBuffer``)
      * ``Zstd::Dictionary.add_entropy_tables_from_buffer(dict, dict_capacity, sample) -> dict`` (``ZDICT_addEntropyTablesFromBuffer``)
      * ``Zstd::Dictionary.getid(dict) -> dict id as integer`` (``ZDICT_getDictID``)


## HOW TO USE

### basic usage (simply encode/decode)

``` ruby:ruby
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

``` ruby:ruby
outport = StringIO.new("")
Zstd.encode(outport) do |encoder|
  encoder.write "abcdefg\n"
  encoder << "hijklmn\n"
  encoder.write "opqrstu\n"
  encoder << "vwxyz\n"
end
```

### Streaming compression (without block and write to file)

``` ruby:ruby
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

``` ruby:ruby
File.open("sample.zst", "rb") do |file|
  Zstd.decode(file) do |decoder|
    p decoder.read(8)  # => "abcdefg\n"
    p decoder.read(1)  # => "h"
    p decoder.read(2)  # => "ij"
    p decoder.read     # => "klmn\nopqrstu\nvwxyz\n"
  end
end
```
