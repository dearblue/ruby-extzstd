#vim: set fileencoding:utf-8

ver = RbConfig::CONFIG["ruby_version"].slice(/\d+\.\d+/)
soname = File.basename(__FILE__, ".rb") << ".so"
lib = File.join(File.dirname(__FILE__), ver, soname)
if File.file?(lib)
  require_relative File.join(ver, soname)
else
  require_relative soname
end

require_relative "extzstd/version"

require "stringio"

if false
def p(*args)
  sf = File.basename(caller(1, 1)[0])
  args.each do |mesg|
    $stderr.puts "#{sf}: #{mesg.inspect}\n"
  end
  return *args
end
end

module Zstd
  module Aux
    module_function
    def io_read(io, size, buf)
      raise Error, "encounted EOF (read error)" unless io.read(size, buf)
      raise Error, "read size too small (read error)" unless buf.bytesize == size
      buf
    end
  end

  class Decoder < Struct.new(:decoder, :import, :readbuf, :destbuf, :status)
    BLOCKSIZE = 1 << 18
    STATUS_TERMINATE = nil
    STATUS_BLOCK_APPROACH = 1
    STATUS_INBLOCK = 2

    #
    # call-seq:
    #   open(import) -> decoder
    #   open(import) { |decoder| ... } -> yield returned value
    #
    # [import]
    #   String instance or +read+ method haved Object.
    #
    def self.open(import)
      import = StringIO.new(import) if import.kind_of?(String)
      dec = new(import)

      return dec unless block_given?

      begin
        yield(dec)
      ensure
        dec.close rescue nil
      end
    end

    def initialize(import)
      raise Error, "require .read method - <%s:0x%08x>" % [import.class, import.object_id << 1] unless import.respond_to?(:read)
      super(LowLevelDecoder.new, import, "".b, "".b, STATUS_BLOCK_APPROACH)

      # read header
      Aux.io_read(import, decoder.next_srcsize, readbuf)
      decoder.decode(readbuf, "", 0)
    end

    def close
      decoder.reset
      import.close rescue nil if import.respond_to?(:close)
      readbuf.clear
      destbuf.clear
      self.status = STATUS_TERMINATE
      nil
    end

    def eof
      !status
    end

    alias eof? eof

    def read(size = nil, dest = "".b)
      dest.clear
      destenc = dest.encoding
      dest.force_encoding Encoding::BINARY

      until size && size <= 0
        if destbuf.empty?
          unless fetch
            return nil if dest.empty?
            break
          end
        end

        d = destbuf.slice!(0, size || destbuf.bytesize)
        dest << d

        size -= d.bytesize if size
      end

      dest
    ensure
      dest.force_encoding destenc rescue nil if destenc
    end

    private
    def fetch
      return nil if eof?

      while true
        if status == STATUS_INBLOCK
          s = decoder.next_srcsize
          if s > 0
            Aux.io_read(import, s, readbuf)
            return destbuf if decoder.decode(readbuf, destbuf, BLOCKSIZE)
            next
          end
          self.status = STATUS_BLOCK_APPROACH
        end

        # status == STATUS_BLOCK_APPROACH
        s = decoder.next_srcsize
        if s == 0
          self.status = STATUS_TERMINATE
          return nil
        end
        Aux.io_read(import, s, readbuf)
        decoder.decode(readbuf, "", 0)
        self.status = STATUS_INBLOCK
      end
    end
  end
end
