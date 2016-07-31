
file "lib/extzstd/version.rb" => %w(README.md) do
  reg = /^\s*\*\s+version:\s*(\d+(?:\.\w+)+)\s*$/i
  unless File.read("README.md", mode: "rt") =~ reg
    raise "``version'' is not defined or bad syntax in ``README.md''"
  end

  GEMSTUB.version = ver = String($1)

  File.write "lib/extzstd/version.rb", <<-"EOS", mode: "wb"
module Zstd
  VERSION = #{ver.inspect}
end
  EOS
end

require_relative "lib/extzstd/version"

GEMSTUB = Gem::Specification.new do |s|
  s.name = "extzstd"
  s.version = Zstd::VERSION
  s.summary = "ruby bindings for Zstandard (zstd)"
  s.description = <<EOS
unoficial ruby bindings for Zstandard (zstd) <https://github.com/Cyan4973/zstd>.
EOS
  s.homepage = "https://osdn.jp/projects/rutsubo/"
  s.license = "2-clause BSD License"
  s.author = "dearblue"
  s.email = "dearblue@users.osdn.me"

  s.required_ruby_version = ">= 2.0"
  s.add_development_dependency "rake"
end

EXTRA.concat(FileList["contrib/**/*"])
