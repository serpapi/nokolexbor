# frozen_string_literal: true

module Nokolexbor
  class DocumentFragment < Nokolexbor::Node
    # Create a {DocumentFragment} from +tags+.
    #
    # @return [DocumentFragment]
    def self.parse(tags)
      new(Nokolexbor::Document.new, tags, nil)
    end

    #  Create a new {DocumentFragment} from +tags+.
    #
    #  If +ctx+ is present, it is used as a context node for the
    #  subtree created.
    def initialize(document, tags = nil, ctx = nil)
      return self unless tags

      ctx ||= document
      node_set = ctx.parse(tags)
      node_set.each { |child| child.parent = self } unless node_set.empty?
      nil
    end

    # @return [String] The name of {DocumentFragment}
    def name
      "#document-fragment"
    end

    alias_method :outer_html, :inner_html
    alias_method :to_html, :outer_html
    alias_method :to_s, :outer_html
    alias_method :serialize, :outer_html

    # Create a {DocumentFragment} from +data+.
    #
    # @return [DocumentFragment]
    def fragment(data)
      document.fragment(data)
    end
  end
end
