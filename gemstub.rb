require_relative "lib/extzstd/version"

GEMSTUB = Gem::Specification.new do |s|
  s.name = "extzstd"
  s.version = Zstd::VERSION
  s.summary = "ruby bindings for Zstandard (zstd)"
  s.description = <<EOS
ruby bindings for Zstandard (zstd) <https://github.com/Cyan4973/zstd>.
EOS
  s.homepage = "http://sourceforge.jp/projects/rutsubo/"
  s.license = "2-clause BSD License"
  s.author = "dearblue"
  s.email = "dearblue@users.sourceforge.jp"

  s.required_ruby_version = ">= 2.0"
  s.add_development_dependency "rake", "~> 10.0"
end

EXTRA.concat(FileList["contrib/**/*"])
