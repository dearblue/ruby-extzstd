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
    assert_equal(Digest::MD5.hexdigest(src), Digest::MD5.hexdigest(Zstd.decode(Zstd.encode(src))))
  end

  def test_huge_stream
    src = "abcdefghijklmnopqrstuvwxyz" * 1000
    md5a = Digest::MD5.new
    d = StringIO.new("")
    Zstd.encode(d) { |z| 1000.times { z << src; md5a.update src } }
    d.pos = 0
    md5b = Digest::MD5.new
    Zstd.decode(d) { |z| buf = ""; while z.read(654321, buf); md5b.update buf; end }
    assert_equal(md5a.hexdigest, md5b.hexdigest)
  end

  def test_dictionary
    dictsrc = "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz" * 10
    dict = Zstd.dict_train_from_buffer(dictsrc, 10000)
    src = "ABCDEFGabcdefg" * 50
    assert_equal(src, Zstd.decode(Zstd.encode(src, nil, dict), src.bytesize, dict))
  end
end
