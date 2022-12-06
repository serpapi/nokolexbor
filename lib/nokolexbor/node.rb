# frozen_string_literal: true

module Nokolexbor
  class Node
    ELEMENT_NODE = 1
    ATTRIBUTE_NODE = 2
    TEXT_NODE = 3
    CDATA_SECTION_NODE = 4
    ENTITY_REF_NODE = 5
    ENTITY_NODE = 6
    PI_NODE = 7
    COMMENT_NODE = 8
    DOCUMENT_NODE = 9
    DOCUMENT_TYPE_NODE = 10
    DOCUMENT_FRAG_NODE = 11
    NOTATION_NODE = 12

    attr_reader :document

    def comment?
      type == COMMENT_NODE
    end

    def cdata?
      type == CDATA_SECTION_NODE
    end

    def processing_instruction?
      type == PI_NODE
    end

    def text?
      type == TEXT_NODE
    end

    def fragment?
      type == DOCUMENT_FRAG_NODE
    end

    def element?
      type == ELEMENT_NODE
    end

    def ancestors
      return NodeSet.new(@document) unless respond_to?(:parent)
      return NodeSet.new(@document) unless parent

      parents = [parent]

      while parents.last.respond_to?(:parent)
        break unless (ctx_parent = parents.last.parent)

        parents << ctx_parent
      end

      NodeSet.new(@document, parents)
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
      if node.is_a?(NodeSet)
        node.each { |n| add_sibling(:previous, n) }
      else
        add_sibling(:previous, node)
      end
      remove
    end

    def xpath(*args)
      paths, handler, ns, binds = extract_params(args)

      xpath_internal(self, paths, handler, ns, binds)
    end

    def at_xpath(*args)
      xpath(*args).first
    end

    private

    def xpath_internal(node, paths, handler, ns, binds)
      # document = node.document
      # return NodeSet.new(document) unless document

      if paths.length == 1
        return xpath_impl(node, paths.first, handler, ns, binds)
      end

      NodeSet.new(@document) do |combined|
        paths.each do |path|
          xpath_impl(node, path, handler, ns, binds).each { |set| combined << set }
        end
      end
    end

    def xpath_impl(node, path, handler, ns, binds)
      ctx = XPathContext.new(node)
      ctx.register_namespaces(ns)
      # path = path.gsub(/xmlns:/, " :") unless Nokogiri.uses_libxml?

      binds&.each do |key, value|
        ctx.register_variable(key.to_s, value)
      end

      ctx.evaluate(path, handler)
    end

    def extract_params(params)
      handler = params.find do |param|
        ![Hash, String, Symbol].include?(param.class)
      end
      params -= [handler] if handler

      hashes = []
      while Hash === params.last || params.last.nil?
        hashes << params.pop
        break if params.empty?
      end
      ns, binds = hashes.reverse

      # ns ||= (document.root&.namespaces || {})
      ns ||= {}

      [params, handler, ns, binds]
    end
  end
end