# frozen_string_literal: true

$:.unshift File.expand_path("../lib", __FILE__)
begin
  require 'nokolexbor/version'
  version = Nokolexbor::VERSION
rescue LoadError => e
  if ENV['NOKOLEXBOR_TEST_GEM']
    version = '0.0.0'
  else
    raise e
  end
end

Gem::Specification.new do |spec|
  spec.name          = 'nokolexbor'
  spec.version       = version
  spec.platform      = Gem::Platform::RUBY
  spec.date          = Time.now.strftime('%Y-%m-%d')
  spec.summary       = "High-performance HTML5 parser, with support for both CSS selectors and XPath."
  spec.homepage      = "https://github.com/serpapi/nokolexbor"
  spec.authors       = ['Yicheng Zhou']
  spec.email         = "zyc9012@gmail.com"
  spec.license       = "MIT"
  spec.extensions    = ['ext/nokolexbor/extconf.rb']
  spec.require_paths = ['lib']
  spec.description   = "Nokolexbor is a high-performance HTML5 parser, with support for both CSS selectors and XPath. It's API is designed to be compatible with Nokogiri."
  spec.files         += Dir.glob("lib/**/*.rb")
  spec.files         += Dir.glob("ext/**/*.[ch]")
  spec.files         += Dir.glob("ext/**/*.in")
  spec.files         += Dir.glob("ext/**/CMakeLists.txt")
  spec.files         += Dir.glob("patches/**/*.patch")
  spec.files         += Dir.glob("vendor/lexbor/source/**/*")
  spec.files         += Dir.glob("vendor/lexbor/utils/lexbor/css/**/*")
  spec.files         += Dir.glob("vendor/lexbor/{CMakeLists.txt,config.cmake,feature.cmake,version}")
  spec.add_development_dependency "rake-compiler", "~> 1.0"
  spec.add_development_dependency "minitest", "~> 5.0"
  spec.metadata["msys2_mingw_dependencies"] = "cmake"
end
