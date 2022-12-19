# frozen_string_literal: true

require 'nokolexbor/nokolexbor'
require 'nokolexbor/version'
require 'nokolexbor/node'
require 'nokolexbor/document'
require 'nokolexbor/node_set'
require 'nokolexbor/attribute'
require 'nokolexbor/xpath'
require 'nokolexbor/xpath_context'

module Nokolexbor
  class << self
    def HTML(*args)
      Document.parse(*args)
    end
  end
end