verreg = /^\s*\*\s+version:\s*(\d+(?:\.\w+)+)\s*$/i
unless File.read("README.md", mode: "rt") =~ verreg
  raise "``version'' is not defined or bad syntax in ``README.md''"
end

version = String($1)

GEMSTUB = Gem::Specification.new do |s|
  s.name = "extzstd"
  s.version = version
  s.summary = "ruby bindings for Zstandard (zstd)"
  s.description = <<EOS
unoficial ruby bindings for Zstandard (zstd) <https://github.com/facebook/zstd>.
EOS
  s.homepage = "https://github.com/dearblue/ruby-extzstd/"
  s.license = "BSD-2-Clause"
  s.author = "dearblue"
  s.email = "dearblue@users.noreply.github.com"

  s.required_ruby_version = ">= 2.0"
  s.add_development_dependency "rake", ">= 12.0"
end

LIB << "lib/extzstd/version.rb"

file "lib/extzstd/version.rb" => %w(README.md) do
  GEMSTUB.version = version

  mkpath "lib/extzstd"
  File.write "lib/extzstd/version.rb", <<-"EOS", mode: "wb"
module Zstd
  VERSION = #{version.inspect}
end
  EOS
end

EXTRA.concat(FileList["contrib/**/*"])

filter = %r(contrib/zstd/(?:build|contrib|doc|examples|lib/dll|programs|tests|zlibWrapper))
DOC.reject! { |path| path =~ filter }
EXTRA.reject! { |path| path =~ filter }
