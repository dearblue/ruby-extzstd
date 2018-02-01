# Quick API Reference for extzstd

  * stream encoder (compression)
      * ``Zstd.encode(buf, params = nil, dict: nil) -> encoded string``
      * ``Zstd.encode(outport, params = nil, dict: nil) -> an instance of Zstd::Encoder``
      * ``Zstd.encode(outport, params = nil, dict: nil) { |encoder| ... } -> block returned value``
      * ``Zstd::Encoder#write(buf) -> this instance``
      * ``Zstd::Encoder#close -> nil``

  * stream decoder (decompression)
      * ``Zstd.decode(zstd_buf, dict: nil) -> decoded string``
      * ``Zstd.decode(inport, dict: nil) -> an intance of Zstd::Decoder``
      * ``Zstd.decode(inport, dict: nil) { |decoder| ... } -> block returned value``
      * ``Zstd::Decoder#read(size = nil, buf = nil) -> buf``
      * ``Zstd::Decoder#close -> nil``

  * context less encoder/decoder (***DEPRECATED***)
      * ``Zstd::ContextLess.encode(src, dest, maxdest, predict, params) -> dest`` (``ZSTD_compress_usingDict``, ``ZSTD_compress_advanced``)
      * ``Zstd::ContextLess.decode(src, dest, maxdest, predict) -> dest`` (``ZSTD_decompress_usingDict``)

  * dictionary (*EXPEREMENTAL*)
      * ``Zstd::Dictionary.train_from_buffer(buf, dict_capacity) -> dictionary'ed string`` (``ZDICT_trainFromBuffer``)
      * ``Zstd::Dictionary.add_entropy_tables_from_buffer(dict, dict_capacity, sample) -> dict`` (``ZDICT_addEntropyTablesFromBuffer``)
      * ``Zstd::Dictionary.getid(dict) -> dict id as integer`` (``ZDICT_getDictID``)
