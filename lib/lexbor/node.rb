# frozen_string_literal: true

module Lexbor
  class Node
    def ancestors
      return NodeSet.new unless respond_to?(:parent)
      return NodeSet.new unless parent

      parents = [parent]

      while parents.last.respond_to?(:parent)
        break unless (ctx_parent = parents.last.parent)

        parents << ctx_parent
      end

      NodeSet.new(parents)
    end

    def matches?(selector)
      ancestors.last.css(selector).any? { |node| node == self }
    end

    def attribute(name)
      return nil unless key?(name)
      Attribute.new(name, attr(name))
    end

    def attributes
      attrs.map { |k, v| [k, Attribute.new(k, v)] }.to_h
    end

    def replace(node)
      add_sibling(:previous, node)
      remove
    end
  end
end