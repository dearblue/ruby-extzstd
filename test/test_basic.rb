#!ruby

require "test-unit"
require "extzstd"
require "digest"

class TestZstd < Test::Unit::TestCase
  def test_encode_decode
    src = "ABCDEFGabcdefg" * 50
    assert_equal(src, Zstd.decode(Zstd.encode(src), src.bytesize))
    #assert_raise(Zstd::Error) { Zstd.decode("", 1111) }
  end

  def test_huge
    src = "ABCDEFGabcdefg" * 10000000
    resrc = Zstd.decode(Zstd.encode(src))
    assert_equal(src.bytesize, resrc.bytesize)
    assert_equal(Digest::MD5.hexdigest(src), Digest::MD5.hexdigest(resrc))
  end

  def test_huge_stream
    src = "abcdefghijklmnopqrstuvwxyz" * 1000
    size_a = 0
    md5a = Digest::MD5.new
    d = StringIO.new("")
    Zstd.encode(d) { |z| 1000.times { z << src; size_a += src.bytesize; md5a.update src } }
    d.pos = 0
    size_b = 0
    md5b = Digest::MD5.new
    partial_list = [262144, 1, 262144, 262142, 524288, 524280, 99, 999, 9999, 99999, 999999, 9999999, nil]
    Zstd.decode(d) do |z|
      buf = ""
      while z.read(s = partial_list.shift, buf)
        assert_equal s, buf.bytesize if s
        size_b += buf.bytesize
        md5b.update buf
      end
    end
    assert_equal size_a, size_b
    assert_equal(md5a.hexdigest, md5b.hexdigest)
  end

  def test_dictionary
    dictsrc = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-" * 5
    dict = Zstd::Dictionary.train_from_buffer(dictsrc, 10000)
    src = "ABCDEFGabcdefg" * 50
    assert_equal(src, Zstd.decode(Zstd.encode(src, dict: dict), src.bytesize, dict: dict))
  end
end
