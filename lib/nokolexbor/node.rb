# frozen_string_literal: true

module Nokolexbor
  class Node
    include Enumerable

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

    LOOKS_LIKE_XPATH = %r{^(\./|/|\.\.|\.$)}

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

    def children=(node)
      children.remove
      if node.is_a?(NodeSet)
        node.each { |n| add_child(n) }
      else
        add_child(node)
      end
    end

    def each
      attributes.each do |node|
        yield [node.name, node.value]
      end
    end

    alias_method :inner_html=, :children=

    def css(*args)
      css_impl(args.join(', '))
    end

    def at_css(*args)
      at_css_impl(args.join(', '))
    end

    def xpath(*args)
      paths, handler, ns, binds = extract_params(args)

      xpath_internal(self, paths, handler, ns, binds)
    end

    def at_xpath(*args)
      xpath(*args).first
    end

    def search(*args)
      paths, handler, ns, binds = extract_params(args)

      if paths.size == 1 && !LOOKS_LIKE_XPATH.match?(paths.first)
        return css(paths.first)
      end

      xpath(*(paths + [ns, handler, binds].compact))
    end

    alias_method :/, :search

    def at(*args)
      paths, handler, ns, binds = extract_params(args)

      if paths.size == 1 && !LOOKS_LIKE_XPATH.match?(paths.first)
        return at_css(paths.first)
      end

      at_xpath(*(paths + [ns, handler, binds].compact))
    end

    alias_method :%, :at

    def classes
      kwattr_values("class")
    end

    def add_class(names)
      kwattr_add("class", names)
    end

    def append_class(names)
      kwattr_append("class", names)
    end

    def remove_class(names = nil)
      kwattr_remove("class", names)
    end

    def kwattr_values(attribute_name)
      keywordify(attr(attribute_name) || [])
    end

    def kwattr_add(attribute_name, keywords)
      keywords = keywordify(keywords)
      current_kws = kwattr_values(attribute_name)
      new_kws = (current_kws + (keywords - current_kws)).join(" ")
      set_attr(attribute_name, new_kws)
      self
    end

    def kwattr_append(attribute_name, keywords)
      keywords = keywordify(keywords)
      current_kws = kwattr_values(attribute_name)
      new_kws = (current_kws + keywords).join(" ")
      set_attr(attribute_name, new_kws)
      self
    end

    def kwattr_remove(attribute_name, keywords)
      if keywords.nil?
        remove_attr(attribute_name)
        return self
      end

      keywords = keywordify(keywords)
      current_kws = kwattr_values(attribute_name)
      new_kws = current_kws - keywords
      if new_kws.empty?
        remove_attr(attribute_name)
      else
        set_attr(attribute_name, new_kws.join(" "))
      end
      self
    end

    def keywordify(keywords)
      case keywords
      when Enumerable
        keywords
      when String
        keywords.scan(/\S+/)
      else
        raise ArgumentError,
          "Keyword attributes must be passed as either a String or an Enumerable, but received #{keywords.class}"
      end
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