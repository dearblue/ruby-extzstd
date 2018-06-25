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
  end

  refine String do
    def zstd(*args)
      Zstd::Encoder.encode self, *args
    end

    def unzstd(*args)
      Zstd::Decoder.decode self, *args
    end
  end

  refine Object do
    def zstd(*args, &block)
      Zstd::Encoder.open self, *args, &block
    end

    def unzstd(*args, &block)
      Zstd::Decoder.open self, *args, &block
    end
  end
end
