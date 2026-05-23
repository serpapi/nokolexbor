# frozen_string_literal: true

module Nokolexbor
  class Namespace
    attr_reader :prefix, :href, :document

    def initialize(document, prefix, href)
      @document = document
      @prefix = prefix
      @href = href
    end

    def inspect
      if prefix
        "#<#{self.class.name} prefix=#{prefix.inspect} href=#{href.inspect}>"
      else
        "#<#{self.class.name} href=#{href.inspect}>"
      end
    end
  end
end
