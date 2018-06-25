#!ruby

ver = RUBY_VERSION.slice(/\d+\.\d+/)
soname = File.basename(__FILE__, ".rb") << ".so"
require_relative File.join(ver, soname)

require_relative "extzstd/version"

require "stringio"

#
# This is ruby bindings for zstd <https://github.com/Cyan4973/zstd> the compression library.
#
module Zstd
  #
  # call-seq:
  #   encode(src_string, level = nil, opts = {}) -> zstd string
  #   encode(src_string, encode_params, opts = {}) -> zstd string
  #   encode(outport, level = nil, opts = {}) -> zstd encoder
  #   encode(outport, level = nil, opts = {}) { |encoder| ... } -> yield returned value
  #   encode(outport, encode_params, opts = {}) -> zstd encoder
  #   encode(outport, encode_params, opts = {}) { |encoder| ... } -> yield returned value
  #
  # [src_string (string)]
  # [outport (io liked object)]
  # [level = nil (integer or nil)]
  # [encode_params (instance of Zstd::Parameters)]
  # [opts dict: nil (string or nil)]
  def self.encode(src, params = nil, dict: nil, &block)
    if src.kind_of?(String)
      return ContextLess.encode(src, Aux::EMPTY_BUFFER.dup, nil, dict, params)
    end

    Encoder.open(src, params, dict, &block)
  end

  #
  # call-seq:
  #   decode(zstd_string, maxsize = nil, dict: nil) -> string
  #   decode(zstd_stream, dict: nil) -> zstd decoder
  #   decode(zstd_stream, dict: nil) { |decoder| ... } -> yield returned value
  #
  def self.decode(src, *args, dict: nil, &block)
    if src.kind_of?(String)
      case args.size
      when 0
        size = nil
      when 1
        size = args[0].to_i
      else
        raise ArgumentError, "wrong argument number (given #{args.size}, expect 1 or 2)"
      end

      Decoder.open(src, dict) { |d| return d.read(size) }
    end

    unless args.empty?
      raise ArgumentError, "wrong argument number (given #{args.size}, expect 1)"
    end

    Decoder.open(src, dict, &block)
  end

  StreamEncoder = Encoder

  class Encoder
    #
    # call-seq:
    #   open(outport, level = nil, dict = nil) -> zstd encoder
    #   open(outport, encode_params, dict = nil) { |encoder| ... } -> yield returned value
    #
    def self.open(outport, *args)
      e = new(outport, *args)

      return e unless block_given?

      begin
        yield e
      ensure
        e.close unless e.eof?
      end
    end

    def self.encode(src, params = nil, dest: nil, dict: nil)
      ContextLess.encode(src, dest || Aux::EMPTY_BUFFER.dup, nil, dict, params)
    end
  end

  class Encoder < Struct.new(:encoder, :outport, :destbuf, :status)
    #
    # call-seq:
    #   open(outport, level = nil, dict = nil) -> zstd encoder
    #   open(outport, encode_params, dict = nil) { |encoder| ... } -> yield returned value
    #
    def self.open(outport, *args)
      e = new(outport, *args)

      return e unless block_given?

      begin
        yield e
      ensure
        e.close rescue nil unless e.eof?
      end
    end

    #
    # call-seq:
    #   initialize(outport, level = nil, dict = nil)
    #   initialize(outport, encode_params, dict = nil)
    #
    # +outport+ need has +.<<+ method.
    #
    def initialize(outport, params = nil, dict = nil)
      encoder = StreamEncoder.new(params, dict)
      super encoder, outport, "".force_encoding(Encoding::BINARY), [true]
    end

    def eof
      !status[0]
    end

    alias eof? eof

    def close
      return nil if eof?
      encoder.end(destbuf, StreamEncoder::OUTSIZE)
      outport << destbuf
      status[0] = false
      nil
    end

    def write(buf)
      raise IOError, "closed stream" if eof?

      off = 0
      rest = buf.bytesize
      outsize = StreamEncoder::OUTSIZE
      while off && off < rest
        off = encoder.update(buf, off, destbuf, outsize)
        outport << destbuf
      end

      self
    end

    alias << write

    def flush
      raise IOError, "closed stream" if eof?

      off = 0
      encoder.flush(destbuf, StreamEncoder::OUTSIZE)
      outport << destbuf

      self
    end
  end if false

  StreamDecoder = Decoder

  class Decoder
    #
    # call-seq:
    #   open(inport, dict = nil) -> decoder
    #   open(inport, dict = nil) { |decoder| ... } -> yield returned value
    #
    # [inport]
    #   String instance or +read+ method haved Object.
    #
    def self.open(inport, dict = nil)
      inport = StringIO.new(inport) if inport.kind_of?(String)

      dec = new(inport, dict)

      return dec unless block_given?

      begin
        yield(dec)
      ensure
        dec.close rescue nil
      end
    end

    def self.decode(src, dest: nil, dict: nil)
      # NOTE: ContextLess.decode は伸長時のサイズが必要なため、常に利用できるわけではない
      # ContextLess.decode(src, dest || Aux::EMPTY_BUFFER.dup, nil, dict)

      new(StringIO.new(src), dict).read(nil, dest)
    end
  end

  class Decoder
    attr_reader :decoder, :inport, :readbuf, :destbuf, :status, :pos

    STATUS_CLOSED = nil
    STATUS_READY = 0
    STATUS_INPORT_EOF = 1

    #
    # call-seq:
    #   open(inport, dict = nil) -> decoder
    #   open(inport, dict = nil) { |decoder| ... } -> yield returned value
    #
    # [inport]
    #   String instance or +read+ method haved Object.
    #
    def self.open(inport, dict = nil)
      inport = StringIO.new(inport) if inport.kind_of?(String)

      dec = new(inport, dict)

      return dec unless block_given?

      begin
        yield(dec)
      ensure
        dec.close rescue nil
      end
    end

    def initialize(inport, dict = nil)
      raise Error, "require .read method - <%s:0x%08x>" % [inport.class, inport.object_id << 1] unless inport.respond_to?(:read)
      @decoder = StreamDecoder.new(dict)
      @inport = inport
      @readbuf = StringIO.new(Aux::EMPTY_BUFFER.dup)
      @destbuf = StringIO.new(Aux::EMPTY_BUFFER.dup)
      @status = STATUS_READY
      @pos = 0
    end

    def close
      inport.close rescue nil if inport.respond_to?(:close)
      readbuf.truncate 0
      destbuf.truncate 0
      @status = STATUS_CLOSED
      nil
    end

    def eof
      !status
    end

    alias eof? eof

    def read(size = nil, dest = Aux::EMPTY_BUFFER.dup)
      dest ||= Aux::EMPTY_BUFFER.dup
      size &&= size.to_i
      Aux.change_binary(dest) do
        #dest.clear
        dest[0 .. -1] = Aux::EMPTY_BUFFER unless dest.empty? # keep allocated heap
        return dest unless !size || size > 0

        d = Aux::EMPTY_BUFFER.dup
        until size && size <= 0
          if destbuf.eof?
            break unless fetch
          end

          destbuf.read(size, d)
          dest << d

          size -= d.bytesize if size
        end
      end

      if dest.empty?
        nil
      else
        @pos += dest.bytesize
        dest
      end
    end

    private
    def fetch
      return nil if eof?

      destbuf.rewind
      destbuf.truncate 0

      while true
        if readbuf.eof? && status != STATUS_INPORT_EOF
          readbuf.rewind
          unless inport.read StreamDecoder::INSIZE, readbuf.string
            @status = STATUS_INPORT_EOF
          end
        end

        begin
          off = decoder.update readbuf.string, readbuf.pos, destbuf.string, StreamDecoder::OUTSIZE
          readbuf.pos = off if off
          return self if destbuf.size > 0
          break if readbuf.eof? && status == STATUS_INPORT_EOF
        rescue Zstd::InitMissingError
          break if readbuf.eof? && status == STATUS_INPORT_EOF
          raise
        end
      end

      # when readbuf.eof? && status == STATUS_INPORT_EOF

      @status = nil
      nil
    end
  end if false

  class Parameters
    def inspect
      "#<#{self.class} windowlog=#{windowlog}, chainlog=#{chainlog}, " \
        "hashlog=#{hashlog}, searchlog=#{searchlog}, " \
        "searchlength=#{searchlength}, strategy=#{strategy}>"
    end

    def pretty_print(q)
      q.group(2, "#<#{self.class}") do
        q.breakable " "
        q.text "windowlog=#{windowlog},"
        q.breakable " "
        q.text "chainlog=#{chainlog},"
        q.breakable " "
        q.text "hashlog=#{hashlog},"
        q.breakable " "
        q.text "searchlog=#{searchlog},"
        q.breakable " "
        q.text "searchlength=#{searchlength},"
        q.breakable " "
        q.text "targetlength=#{targetlength},"
        q.breakable " "
        q.text "strategy=#{strategy}>"
      end
    end
  end

  module Aux
    EMPTY_BUFFER = "".force_encoding(Encoding::BINARY).freeze

    def self.io_read(io, size, buf)
      raise Error, "encounted EOF (read error)" unless io.read(size, buf)
      raise Error, "read size too small (read error)" unless buf.bytesize == size
      buf
    end

    def self.change_binary(str)
      e = str.encoding
      str.force_encoding(Encoding::BINARY)
      yield
    ensure
      str.force_encoding e rescue nil if e
    end
  end
end
