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

module Nokolexbor
  class << self
    def HTML(*args)
      Document.parse(*args)
    end
  end
end