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
  #   encode(src_string, level = nil, dict = nil) -> zstd string
  #   encode(src_string, encoder_params, dict = nil) -> zstd string
  #   encode(outport, level = nil, dict = nil) -> zstd encoder
  #   encode(outport, level = nil, dict = nil) { |encoder| ... } -> yield returned value
  #   encode(outport, encoder_params, dict = nil) -> zstd encoder
  #   encode(outport, encoder_params, dict = nil) { |encoder| ... } -> yield returned value
  #
  def self.encode(src, *args, &block)
    if src.kind_of?(String)
      dest = Aux::EMPTY_BUFFER.dup
      return Encoder.open(dest, *args) { |e| e.write src; dest }
    end

    Encoder.open(src, *args, &block)
  end

  #
  # call-seq:
  #   decode(zstd_string, maxsize = nil, dict = nil) -> string
  #   decode(zstd_stream, maxsize = nil, dict = nil) -> zstd decoder
  #   decode(zstd_stream, maxsize = nil, dict = nil) { |decoder| ... } -> yield returned value
  #
  def self.decode(src, maxsize = nil, dict = nil, &block)
    if src.kind_of?(String)
      maxsize &&= maxsize.to_i
      return Decoder.open(src, dict) { |d| return d.read(maxsize) }
    end

    Decoder.open(src, dict, &block)
  end

  class Encoder < Struct.new(:encoder, :outport, :destbuf, :status)
    #
    # call-seq:
    #   open(outport, level = nil, dict = nil) -> zstd encoder
    #   open(outport, encoder_params, dict = nil) { |encoder| ... } -> yield returned value
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
    #   initialize(outport, encoder_params, dict = nil)
    #
    # +outport+ need has +.<<+ method.
    #
    def initialize(outport, params = nil, dict = nil)
      encoder = BufferedEncoder.new(params, dict)
      super encoder, outport, "".force_encoding(Encoding::BINARY), [true]
    end

    def eof
      !status[0]
    end

    alias eof? eof

    def close
      return nil if eof?
      encoder.end(destbuf, BufferedEncoder.recommended_outsize)
      outport << destbuf
      status[0] = false
      nil
    end

    def write(buf)
      raise IOError, "closed stream" if eof?

      off = 0
      rest = buf.bytesize
      outsize = BufferedEncoder.recommended_outsize
      while off && off < rest
        off = encoder.continue(buf, off, destbuf, outsize)
        outport << destbuf
      end

      self
    end

    alias << write

    def flush
      raise IOError, "closed stream" if eof?

      off = 0
      encoder.flush(destbuf, BufferedEncoder.recommended_outsize)
      outport << destbuf

      self
    end
  end

  class Decoder < Struct.new(:decoder, :inport, :readbuf, :destbuf, :status)
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
      super(BufferedDecoder.new(dict), inport, StringIO.new(Aux::EMPTY_BUFFER.dup), StringIO.new(Aux::EMPTY_BUFFER.dup), STATUS_READY)
    end

    def close
      decoder.reset
      inport.close rescue nil if inport.respond_to?(:close)
      readbuf.clear
      #destbuf.clear
      self.status = STATUS_CLOSED
      nil
    end

    def eof
      !status
    end

    alias eof? eof

    def read(size = nil, dest = Aux::EMPTY_BUFFER.dup)
      dest ||= Aux::EMPTY_BUFFER.dup
      Aux.change_binary(dest) do
        #dest.clear
        dest[0 .. -1] = Aux::EMPTY_BUFFER # keep allocated heap

        return dest if size == 0

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
        dest
      end
    end

    private
    def fetch
      return nil if eof? || status == STATUS_INPORT_EOF

      while true
        if readbuf.eof?
          readbuf.string[0 .. -1] = Aux::EMPTY_BUFFER
          readbuf.rewind
          unless inport.read(BufferedDecoder.recommended_insize, readbuf.string)
            self.status = STATUS_INPORT_EOF
            return nil
          end
        end

        off = decoder.continue(readbuf.string, readbuf.pos, destbuf.string, BufferedDecoder.recommended_outsize)
        readbuf.pos = off if off
        destbuf.rewind
        return self if destbuf.size > 0
      end
    end
  end

  class EncodeParameters
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
