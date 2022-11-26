# frozen_string_literal: true

require 'lexbor/version'
VERSION = Lexbor::Version

Gem::Specification.new do |spec|
  spec.name          = 'lexbor'
  spec.version       = VERSION
  spec.summary       = "lexbor-ruby is a Ruby binding to the Lexbor library"
  spec.extensions    = ['ext/lexbor/extconf.rb']
  spec.require_paths = ['lib']
  spec.files         += Dir.glob("lib/**/*.rb")
  spec.files         += Dir.glob("vendor/lexbor/source/**/*")
  spec.files         += Dir.glob("vendor/lexbor/{CMakeLists.txt,config.cmake,feature.cmake}")
  spec.add_development_dependency "rake-compiler", ">= 0.9.0"
end
