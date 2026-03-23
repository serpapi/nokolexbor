# frozen_string_literal: true

begin
  # pre-compiled extension by rake-compiler is located inside lib/nokolexbor/<ruby_version>/
  RUBY_VERSION =~ /(\d+\.\d+)/
  require "nokolexbor/#{Regexp.last_match(1)}/nokolexbor"
rescue LoadError => e
  if /GLIBC/.match?(e.message)
    warn(<<~EOM)
      ERROR: It looks like you're trying to use Nokolexbor as a precompiled native gem on a system
             with an unsupported version of glibc.
        #{e.message}
        If that's the case, then please install Nokolexbor via the `ruby` platform gem:
            gem install nokolexbor --platform=ruby
        or:
            bundle config set force_ruby_platform true
    EOM
    raise e
  end

  require 'nokolexbor/nokolexbor'
end

require 'nokolexbor/version'
require 'nokolexbor/node'
require 'nokolexbor/document'
require 'nokolexbor/node_set'
require 'nokolexbor/document_fragment'
require 'nokolexbor/xpath'
require 'nokolexbor/xpath_context'
require 'nokolexbor/builder'

module Nokolexbor
  class << self
    def parse(*args)
      Document.parse(*args)
    end

    alias_method :HTML, :parse
  end
end

# Parse an HTML document, or build one with the DSL when a block is given.
def Nokolexbor(string_or_io = nil, &block)
  if block
    Nokolexbor::Builder.new(&block).parent
  else
    Nokolexbor.parse(string_or_io)
  end
end