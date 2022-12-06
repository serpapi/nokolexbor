# frozen_string_literal: true

module Nokolexbor
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

      NodeSet.new do |combined|
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