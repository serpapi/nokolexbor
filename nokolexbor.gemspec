# frozen_string_literal: true

require_relative 'lib/nokolexbor/version'

Gem::Specification.new do |spec|
  spec.name          = 'nokolexbor'
  spec.version       = Nokolexbor::VERSION
  spec.summary       = "Nokolexbor is a Ruby binding to the Lexbor library and is API-compatible with Nokogiri"
  spec.authors       = ['Yicheng Zhou']
  spec.extensions    = ['ext/nokolexbor/extconf.rb']
  spec.require_paths = ['lib']
  spec.files         += Dir.glob("lib/**/*.rb")
  spec.files         += Dir.glob("vendor/lexbor/source/**/*")
  spec.files         += Dir.glob("vendor/lexbor/{CMakeLists.txt,config.cmake,feature.cmake}")
  spec.add_development_dependency "rake-compiler", ">= 0.9.0"
end
