# frozen_string_literal: true

module Nokolexbor
  class DocumentFragment < Nokolexbor::Node
    def self.parse(tags)
      new(Nokolexbor::Document.new, tags, nil)
    end

    def initialize(document, tags = nil, ctx = nil)
      return self unless tags

      ctx ||= document
      node_set = ctx.parse(tags)
      node_set.each { |child| child.parent = self } unless node_set.empty?
      nil
    end

    def name
      "#document-fragment"
    end

    alias_method :outer_html, :inner_html
    alias_method :to_html, :outer_html
    alias_method :to_s, :outer_html
    alias_method :serialize, :outer_html

    def fragment(data)
      document.fragment(data)
    end
  end
end
