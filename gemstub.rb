require_relative "lib/extzstd/version"

GEMSTUB = Gem::Specification.new do |s|
  s.name = "extzstd"
  s.version = Zstd::VERSION
  s.summary = "ruby bindings for Zstandard (zstd)"
  s.description = <<EOS
unoficial ruby bindings for Zstandard (zstd) <https://github.com/facebook/zstd>.
EOS
  s.homepage = "https://github.com/dearblue/ruby-extzstd/"
  s.license = "BSD-2-Clause"
  s.author = "dearblue"
  s.email = "dearblue@users.osdn.me"

  s.required_ruby_version = ">= 2.0"
  s.add_development_dependency "rake", ">= 12.0"
end

EXTRA.concat(FileList["contrib/**/*"])

filter = %r(contrib/zstd/(?:build|contrib|doc|examples|lib/dll|programs|tests|zlibWrapper))
DOC.reject! { |path| path =~ filter }
EXTRA.reject! { |path| path =~ filter }
