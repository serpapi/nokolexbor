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

    def document?
      is_a?(Nokolexbor::Document)
    end

    def ancestors(selector = nil)
      return NodeSet.new(@document) unless respond_to?(:parent)
      return NodeSet.new(@document) unless parent

      parents = [parent]

      while parents.last.respond_to?(:parent)
        break unless (ctx_parent = parents.last.parent)

        parents << ctx_parent
      end

      return NodeSet.new(@document, parents) unless selector

      root = parents.last
      search_results = root.search(selector)

      NodeSet.new(@document, parents.find_all do |parent|
        search_results.include?(parent)
      end)
    end

    def wrap(node)
      case node
      when String
        new_parent = fragment(node).child
      when Node
        new_parent = node.dup
      else
        raise ArgumentError, "Requires a String or Node argument, and cannot accept a #{node.class}"
      end

      if parent
        add_sibling(:next, new_parent)
      else
        new_parent.remove
      end
      new_parent.add_child(self)

      self
    end

    def add_previous_sibling(node_or_tags)
      raise ArgumentError,
        "A document may not have multiple root nodes." if parent&.document? && !(node_or_tags.comment? || node_or_tags.processing_instruction?)

      add_sibling(:previous, node_or_tags)
    end

    def add_next_sibling(node_or_tags)
      raise ArgumentError,
        "A document may not have multiple root nodes." if parent&.document? && !(node_or_tags.comment? || node_or_tags.processing_instruction?)

      add_sibling(:next, node_or_tags)
    end

    def before(node_or_tags)
      add_previous_sibling(node_or_tags)
      self
    end

    def after(node_or_tags)
      add_next_sibling(node_or_tags)
      self
    end

    alias_method :next_sibling, :next
    alias_method :previous_sibling, :previous
    alias_method :next=, :add_next_sibling
    alias_method :previous=, :add_previous_sibling

    def <<(node_or_tags)
      add_child(node_or_tags)
      self
    end

    def prepend_child(node)
      if (first = children.first)
        # Mimic the error add_child would raise.
        raise "Document already has a root node" if document? && !(node.comment? || node.processing_instruction?)

        first.add_sibling(:previous, node)
      else
        add_child(node)
      end
    end

    def traverse(&block)
      children.each { |j| j.traverse(&block) }
      yield(self)
    end

    def matches?(selector)
      ancestors.last.css(selector).any? { |node| node == self }
    end

    def attributes
      attribute_nodes.each_with_object({}) do |node, hash|
        hash[node.name] = node
      end
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

    def parent=(parent_node)
      parent_node.add_child(self)
    end

    def each
      attributes.each do |name, node|
        yield [name, node.value]
      end
    end

    def fragment(tags)
      Nokolexbor::DocumentFragment.new(document, tags, self)
    end

    alias_method :inner_html=, :children=

    def css(*args)
      css_impl(args.join(', '))
    end

    def at_css(*args)
      at_css_impl(args.join(', '))
    end

    def nokogiri_css(*args)
      rules, handler, ns, _ = extract_params(args)

      nokogiri_css_internal(self, rules, handler, ns)
    end

    def nokogiri_at_css(*args)
      nokogiri_css(*args).first
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

    def write_to(io, *options)
      io.write(to_html(*options))
    end

    alias_method :write_html_to, :write_to

    private

    def nokogiri_css_internal(node, rules, handler, ns)
      xpath_internal(node, css_rules_to_xpath(rules, ns), handler, ns, nil)
    end

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

    def css_rules_to_xpath(rules, ns)
      rules.map { |rule| xpath_query_from_css_rule(rule, ns) }
    end

    def ensure_nokogiri
      unless defined?(Nokogiri) && defined?(Nokogiri::CSS)
        require 'nokogiri'
      end
    rescue LoadError
      fail('nokogiri_css and nokogiri_at_css require Nokogiri to be installed')
    end

    def xpath_query_from_css_rule(rule, ns)
      ensure_nokogiri
      if defined? Nokogiri::CSS::XPathVisitor::BuiltinsConfig
        visitor = Nokogiri::CSS::XPathVisitor.new(
          builtins: Nokogiri::CSS::XPathVisitor::BuiltinsConfig::OPTIMAL,
          doctype: :html4,
        )
      else
        visitor = Nokogiri::CSS::XPathVisitorOptimallyUseBuiltins.new
      end
      self.class::IMPLIED_XPATH_CONTEXTS.map do |implied_xpath_context|
        Nokogiri::CSS.xpath_for(rule.to_s, { prefix: implied_xpath_context, ns: ns,
                                   visitor: visitor, })
      end.join(" | ")
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

    IMPLIED_XPATH_CONTEXTS = [".//"].freeze
  end
end
