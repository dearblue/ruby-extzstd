#!ruby

require "test-unit"
require "extzstd"
require "digest"

class TestZstd < Test::Unit::TestCase
  def test_encode_decode
    src = "ABCDEFGabcdefg" * 50
    assert_equal(src, Zstd.decode(Zstd.encode(src), src.bytesize))
    #assert_raise(Zstd::Error) { Zstd.decode("", 1111) }
    d1 = Zstd.encode(src)
    assert_same(src.tainted?, Zstd.encode(src).tainted?)
    src1 = src.dup
    src1.taint
    assert_same(src1.tainted?, Zstd.encode(src1).tainted?)
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
    Zstd.decode(d) { |z| buf = ""; while z.read(654321, buf); size_b += buf.bytesize; md5b.update buf; end }
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
