# extzstd - ruby bindings for Zstd (Zstandard)

This is unofficial ruby bindings for compression library
[Zstd (https://github.com/Cyan4973/zstd)](https://github.com/Cyan4973/zstd).

  * package name: extzstd
  * version: 0.0.3.CONCEPT
  * software quality: EXPERIMENTAL
  * license: 2-clause BSD License
  * author: dearblue <mailto:dearblue@users.osdn.me>
  * report issue to: <https://osdn.jp/projects/rutsubo/ticket/>
  * dependency ruby: ruby-2.0+
  * dependency ruby gems: (none)
  * dependency library: (none)
  * bundled external C library:
      * zstd-0.7.4 <https://github.com/Cyan4973/zstd/tree/v0.7.4>

"extzstd" is supported the legacy formats (zstd-0.1 - 0.6).


## Quick API Reference

  * Encode
      * ``Zstd.encode(buf, params = nil, dict = nil) -> encoded string``
      * ``Zstd.encode(outport, params = nil, dict = nil) -> an instance of Zstd::Encoder``
      * ``Zstd.encode(outport, params = nil, dict = nil) { |encoder| ... } -> block returned value``
      * ``Zstd::Encoder#write(buf) -> this instance``
      * ``Zstd::Encoder#close -> nil``

  * Decode
      * ``Zstd.decode(zstd_buf, dict = nil) -> decoded string``
      * ``Zstd.decode(inport, dict = nil) -> an intance of Zstd::Decoder``
      * ``Zstd.decode(inport, dict = nil) { |decoder| ... } -> block returned value``
      * ``Zstd::Decoder#read(size = nil, buf = nil) -> buf``
      * ``Zstd::Decoder#close -> nil``

  * buffered encoder
      * ``Zstd::BufferedEncoder.new(params, dict)`` (``ZBUFF_createCCtx``, ``ZBUFF_compressInit_advanced``)
      * ``Zstd::BufferedEncoder#continue(data, dataoff, dest, maxdest) -> integer as new data offset`` (``ZBUFF_compressContinue``)
      * ``Zstd::BufferedEncoder#flush(dest, maxdest) -> dest`` (``ZBUFF_compressFlush``)
      * ``Zstd::BufferedEncoder#end(dest, maxdest) -> dest`` (``ZBUFF_compressEnd``)

  * Train dictionary
      * ``Zstd.dict_train_from_buffer(buf, dict_capacity) -> dictionary'd string`` (``ZDICT_trainFromBuffer``)
      * ``Zstd.dict_add_entropy_tables_from_buffer(dict, dict_capacity, sample) -> dict`` (``ZDICT_addEntropyTablesFromBuffer``)


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
