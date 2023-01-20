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

    # @return [Document] The associated {Document} of this node
    attr_reader :document

    LOOKS_LIKE_XPATH = %r{^(\./|/|\.\.|\.$)}

    # @return true if this is a Comment
    def comment?
      type == COMMENT_NODE
    end

    # @return true if this is a CDATA
    def cdata?
      type == CDATA_SECTION_NODE
    end

    # @return true if this is a ProcessingInstruction
    def processing_instruction?
      type == PI_NODE
    end

    # @return true if this is a Text
    def text?
      type == TEXT_NODE
    end

    # @return true if this is a DocumentFragment
    def fragment?
      type == DOCUMENT_FRAG_NODE
    end

    # @return true if this is an Element
    def element?
      type == ELEMENT_NODE
    end

    # @return true if this is a Document
    def document?
      is_a?(Nokolexbor::Document)
    end

    # Get a list of ancestor Node of this Node
    #
    # @param [String, nil] selector The selector to match ancestors
    #
    # @return [NodeSet] A set of matched ancestor nodes
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

    # Wrap this Node with another node.
    #
    # @param node [String, Node] A string or a node
    #  - when {String}:
    #    The markup that is parsed and used as the wrapper. If the parsed
    #    fragment has multiple roots, the first root node is used as the wrapper.
    #  - when {Node}:
    #    An element that is cloned and used as the wrapper.
    #
    # @return +self+
    #
    # @see NodeSet#wrap
    #
    # @example with a {String} argument:
    #
    #   doc = Nokolexbor::HTML('<body><a>123</a></body>')
    #   doc.at_css('a').wrap('<div></div>')
    #   doc.at_css('body').inner_html
    #   # => "<div><a>123</a></div>"
    #
    # @example with a {Node} argument:
    #
    #   doc = Nokolexbor::HTML('<body><a>123</a></body>')
    #   doc.at_css('a').wrap(doc.create_element('div'))
    #   doc.at_css('body').inner_html
    #   # => "<div><a>123</a></div>"
    #
    def wrap(node)
      case node
      when String
        new_parent = fragment(node).child
      when DocumentFragment
        new_parent = node.child
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

    # Insert +node_or_tags+ before this Node (as a sibling).
    #
    # @param node_or_tags [Node, DocumentFragment, NodeSet, String] The node to be added.
    #
    # @return [Node,NodeSet] The reparented {Node} (if +node_or_tags+ is a {Node}), or {NodeSet} (if +node_or_tags+ is a {DocumentFragment}, {NodeSet}, or {String}).
    #
    # @see #before
    def add_previous_sibling(node_or_tags)
      raise ArgumentError,
        "A document may not have multiple root nodes." if parent&.document? && !(node_or_tags.comment? || node_or_tags.processing_instruction?)

      add_sibling(:previous, node_or_tags)
    end

    # Insert +node_or_tags+ after this Node (as a sibling).
    #
    # @param node_or_tags [Node, DocumentFragment, NodeSet, String] The node to be added.
    #
    # @return [Node,NodeSet] The reparented {Node} (if +node_or_tags+ is a {Node}), or {NodeSet} (if +node_or_tags+ is a {DocumentFragment}, {NodeSet}, or {String}).
    #
    # @see #after
    def add_next_sibling(node_or_tags)
      raise ArgumentError,
        "A document may not have multiple root nodes." if parent&.document? && !(node_or_tags.comment? || node_or_tags.processing_instruction?)

      add_sibling(:next, node_or_tags)
    end

    # Insert +node_or_tags+ before this Node (as a sibling).
    #
    # @param node_or_tags [Node, DocumentFragment, NodeSet, String] The node to be added.
    #
    # @return +self+, to support chaining of calls.
    #
    # @see #before
    def before(node_or_tags)
      add_previous_sibling(node_or_tags)
      self
    end

    # Insert +node_or_tags+ after this Node (as a sibling).
    #
    # @param node_or_tags [Node, DocumentFragment, NodeSet, String] The node to be added.
    #
    # @return +self+, to support chaining of calls.
    #
    # @see #after
    def after(node_or_tags)
      add_next_sibling(node_or_tags)
      self
    end

    alias_method :next_sibling, :next
    alias_method :previous_sibling, :previous
    alias_method :next=, :add_next_sibling
    alias_method :previous=, :add_previous_sibling

    # Add +node_or_tags+ as a child of this Node.
    #
    # @param node_or_tags [Node, DocumentFragment, NodeSet, String] The node to be added.
    #
    # @return +self+, to support chaining of calls.
    #
    # @see #add_child
    def <<(node_or_tags)
      add_child(node_or_tags)
      self
    end

    # Add +node+ as the first child of this Node.
    #
    # @param node [Node, DocumentFragment, NodeSet, String] The node to be added.
    #
    # @return [Node,NodeSet] The reparented {Node} (if +node+ is a {Node}), or {NodeSet} (if +node+ is a {DocumentFragment}, {NodeSet}, or {String}).
    #
    # @see #add_child
    def prepend_child(node)
      if (first = children.first)
        # Mimic the error add_child would raise.
        raise "Document already has a root node" if document? && !(node.comment? || node.processing_instruction?)

        first.add_sibling(:previous, node)
      else
        add_child(node)
      end
    end

    # Traverse self and all children.
    # @yield self and all children to +block+ recursively.
    def traverse(&block)
      children.each { |j| j.traverse(&block) }
      yield(self)
    end

    # @param selector [String] The selector to match
    #
    # @return true if this Node matches +selector+
    def matches?(selector)
      ancestors.last.css(selector).any? { |node| node == self }
    end

    # Fetch this node's attributes.
    #
    # @return Hash containing attributes belonging to +self+. The hash keys are String attribute names, and the hash values are {Nokolexbor::Attribute}.
    def attributes
      attribute_nodes.each_with_object({}) do |node, hash|
        hash[node.name] = node
      end
    end

    # Replace this Node with +node+.
    #
    # @param node [Node, DocumentFragment, NodeSet, String]
    #
    # @return The reparented {Node} (if +node+ is a {Node}), or {NodeSet} (if +node+ is a {DocumentFragment}, {NodeSet}, or {String}).
    #
    # @see #swap
    def replace(node)
      ret = add_sibling(:previous, node)
      remove
      ret
    end

    # Swap this Node for +node+
    #
    # @param node [Node, DocumentFragment, NodeSet, String]
    #
    # @return +self+, to support chaining of calls.
    #
    # @see #replace
    def swap(node)
      replace(node)
      self
    end

    # Set the content of this Node.
    #
    # @param node [Node, DocumentFragment, NodeSet, String] The node to be added.
    #
    # @see #inner_html=
    def children=(node)
      children.remove
      add_child(node)
    end

    # Set the parent Node of this Node.
    #
    # @param parent_node [Node] The parent node.
    def parent=(parent_node)
      parent_node.add_child(self)
    end

    # Iterate over each attribute name and value pair of this Node.
    def each
      attributes.each do |name, node|
        yield [name, node.value]
      end
    end

    # Create a {DocumentFragment} containing +tags+ that is relative to _this_
    # context node.
    def fragment(tags)
      Nokolexbor::DocumentFragment.new(document, tags, self)
    end

    alias_method :inner_html=, :children=

    # Search this object for CSS +rules+. +rules+ must be one or more CSS
    # selectors.
    #
    # This method uses Lexbor as the selector engine. Its performance is much higher than {#nokogiri_css}
    #
    # @example
    #   node.css('title')
    #   node.css('body h1.bold')
    #   node.css('div + p.green', 'div#one')
    #
    # @return [NodeSet] The matched set of Nodes.
    #
    # @see #nokogiri_css
    def css(*args)
      css_impl(args.join(', '))
    end

    # Like {#css}, but returns the first match.
    #
    # This method uses Lexbor as the selector engine. Its performance is much higher than {#nokogiri_at_css}
    #
    # @return [Node, nil] The first matched Node.
    #
    # @see #css
    # @see #nokogiri_at_css
    def at_css(*args)
      at_css_impl(args.join(', '))
    end

    # Search this object for CSS +rules+. +rules+ must be one or more CSS
    # selectors. It supports a mixed syntax of CSS selectors and XPath.
    #
    # This method uses libxml2 as the selector engine. It behaves the same as {Nokogiri::Node#css}
    #
    # @return [NodeSet] The matched set of Nodes.
    def nokogiri_css(*args)
      rules, handler, ns, _ = extract_params(args)

      nokogiri_css_internal(self, rules, handler, ns)
    end

    # Like {#nokogiri_css}, but returns the first match.
    #
    # This method uses libxml2 as the selector engine. It behaves the same as {Nokogiri::Node#at_css}
    #
    # @return [Node, nil] The first matched Node.
    #
    # @see #nokogiri_at_css
    # @see #at_css
    def nokogiri_at_css(*args)
      nokogiri_css(*args).first
    end

    # Search this node for XPath +paths+. +paths+ must be one or more XPath
    # queries.
    #
    # It behaves the same as {Nokogiri::Node#xpath}
    #
    # @example
    #   node.xpath('.//title')
    #
    # @return [NodeSet] The matched set of Nodes.
    def xpath(*args)
      paths, handler, ns, binds = extract_params(args)

      xpath_internal(self, paths, handler, ns, binds)
    end

    # Like {#xpath}, but returns the first match.
    #
    # It behaves the same as {Nokogiri::Node#at_xpath}
    #
    # @return [Node, nil] The first matched Node.
    #
    # @see #xpath
    def at_xpath(*args)
      xpath(*args).first
    end

    # Search this object for +paths+. +paths+ must be one or more XPath or CSS selectors.
    #
    # @return [NodeSet] The matched set of Nodes.
    def search(*args)
      paths, handler, ns, binds = extract_params(args)

      if paths.size == 1 && !LOOKS_LIKE_XPATH.match?(paths.first)
        return css(paths.first)
      end

      xpath(*(paths + [ns, handler, binds].compact))
    end

    alias_method :/, :search

    # Like {#search}, but returns the first match.
    #
    # @return [Node, nil] The first matched Node.
    #
    # @see #search
    def at(*args)
      paths, handler, ns, binds = extract_params(args)

      if paths.size == 1 && !LOOKS_LIKE_XPATH.match?(paths.first)
        return at_css(paths.first)
      end

      at_xpath(*(paths + [ns, handler, binds].compact))
    end

    alias_method :%, :at

    # Fetch CSS class names of a Node.
    #
    # This is a convenience function and is equivalent to:
    #
    #   node.kwattr_values("class")
    #
    # @see #kwattr_values
    # @see #add_class
    # @see #append_class
    # @see #remove_class
    #
    # @return [Array]
    #   The CSS classes present in the Node's "class" attribute. If the
    #   attribute is empty or non-existent, the return value is an empty array.
    #
    # @example
    #   node.classes # => ["section", "title", "header"]
    def classes
      kwattr_values("class")
    end

    # Ensure CSS classes are present on +self+. Any CSS classes in +names+ that already exist
    # in the "class" attribute are _not_ added. Note that any existing duplicates in the
    # "class" attribute are not removed. Compare with {#append_class}.
    #
    # This is a convenience function and is equivalent to:
    #
    #   node.kwattr_add("class", names)
    #
    # @see #kwattr_add
    # @see #classes
    # @see #append_class
    # @see #remove_class
    #
    # @param [String, Array<String>] names
    #   CSS class names to be added to the Node's "class" attribute. May be a string containing
    #   whitespace-delimited names, or an Array of String names. Any class names already present
    #   will not be added. Any class names not present will be added. If no "class" attribute
    #   exists, one is created.
    #
    # @return [Node] +self+ for ease of chaining method calls.
    #
    # @example
    #   node.add_class("section") # => <div class="section"></div>
    #   node.add_class("section") # => <div class="section"></div> # duplicate not added
    #   node.add_class("section header") # => <div class="section header"></div>
    #   node.add_class(["section", "header"]) # => <div class="section header"></div>
    def add_class(names)
      kwattr_add("class", names)
    end

    # Add CSS classes to +self+, regardless of duplication. Compare with {#add_class}.
    #
    # This is a convenience function and is equivalent to:
    #
    #   node.kwattr_append("class", names)
    #
    # @see #kwattr_append
    # @see #classes
    # @see #add_class
    # @see #remove_class
    #
    # @return self
    def append_class(names)
      kwattr_append("class", names)
    end

    # Remove CSS classes from this node. Any CSS class names in +css_classes+ that exist in
    # this node's "class" attribute are removed, including any multiple entries.
    #
    # If no CSS classes remain after this operation, or if +css_classes+ is +nil+, the "class"
    # attribute is deleted from the node.
    #
    # This is a convenience function and is equivalent to:
    #
    #   node.kwattr_remove("class", css_classes)
    #
    # @see #kwattr_remove
    # @see #classes
    # @see #add_class
    # @see #append_class
    #
    # @param names [String, Array<String>]
    #   CSS class names to be removed from the Node's
    #   "class" attribute. May be a string containing whitespace-delimited names, or an Array of
    #   String names. Any class names already present will be removed. If no CSS classes remain,
    #   the "class" attribute is deleted.
    #
    # @return [Node] +self+ for ease of chaining method calls.
    #
    # @example
    #   node.remove_class("section")
    #   node.remove_class(["section", "float"])
    def remove_class(names = nil)
      kwattr_remove("class", names)
    end

    # Fetch values from a keyword attribute of a Node.
    #
    # A "keyword attribute" is a node attribute that contains a set of space-delimited
    # values. Perhaps the most familiar example of this is the HTML "class" attribute used to
    # contain CSS classes. But other keyword attributes exist, for instance
    # {the "rel" attribute}[https://developer.mozilla.org/en-US/docs/Web/HTML/Attributes/rel].
    #
    # @see #kwattr_add
    # @#kwattr_append
    # @#kwattr_remove
    #
    # @param attribute_name [String]
    #   The name of the keyword attribute to be inspected.
    #
    # @return [Array<String>]
    #   The values present in the Node's +attribute_name+ attribute. If the
    #   attribute is empty or non-existent, the return value is an empty array.
    def kwattr_values(attribute_name)
      keywordify(attr(attribute_name) || [])
    end

    # Ensure that values are present in a keyword attribute.
    #
    # Any values in +keywords+ that already exist in the Node's attribute values are _not_
    # added. Note that any existing duplicates in the attribute values are not removed. Compare
    # with {#kwattr_append}.
    #
    # A "keyword attribute" is a node attribute that contains a set of space-delimited
    # values. Perhaps the most familiar example of this is the HTML "class" attribute used to
    # contain CSS classes. But other keyword attributes exist, for instance
    # {the "rel" attribute}[https://developer.mozilla.org/en-US/docs/Web/HTML/Attributes/rel].
    #
    # @see #add_class
    # @see #kwattr_values
    # @see #kwattr_append
    # @see #kwattr_remove
    #
    # @param attribute_name [String] The name of the keyword attribute to be modified.
    # @param keywords [String, Array<String>]
    #   Keywords to be added to the attribute named +attribute_name+. May be a string containing
    #   whitespace-delimited values, or an Array of String values. Any values already present will
    #   not be added. Any values not present will be added. If the named attribute does not exist,
    #   it is created.
    #
    # @return [Node] +self+ for ease of chaining method calls.
    def kwattr_add(attribute_name, keywords)
      keywords = keywordify(keywords)
      current_kws = kwattr_values(attribute_name)
      new_kws = (current_kws + (keywords - current_kws)).join(" ")
      set_attr(attribute_name, new_kws)
      self
    end

    # Add keywords to a Node's keyword attribute, regardless of duplication. Compare with
    # {#kwattr_add}.
    #
    # A "keyword attribute" is a node attribute that contains a set of space-delimited
    # values. Perhaps the most familiar example of this is the HTML "class" attribute used to
    # contain CSS classes. But other keyword attributes exist, for instance
    # {the "rel" attribute}[https://developer.mozilla.org/en-US/docs/Web/HTML/Attributes/rel].
    #
    # @see #add_class
    # @see #kwattr_values
    # @see #kwattr_add
    # @see #kwattr_remove
    #
    # @param attribute_name [String] The name of the keyword attribute to be modified.
    # @param keywords [String, Array<String>]
    #   Keywords to be added to the attribute named +attribute_name+. May be a string containing
    #   whitespace-delimited values, or an Array of String values. Any values already present will
    #   not be added. Any values not present will be added. If the named attribute does not exist,
    #   it is created.
    #
    # @return [Node] +self+ for ease of chaining method calls.
    def kwattr_append(attribute_name, keywords)
      keywords = keywordify(keywords)
      current_kws = kwattr_values(attribute_name)
      new_kws = (current_kws + keywords).join(" ")
      set_attr(attribute_name, new_kws)
      self
    end

    # Remove keywords from a keyword attribute. Any matching keywords that exist in the named
    # attribute are removed, including any multiple entries.
    #
    # If no keywords remain after this operation, or if +keywords+ is +nil+, the attribute is
    # deleted from the node.
    #
    # A "keyword attribute" is a node attribute that contains a set of space-delimited
    # values. Perhaps the most familiar example of this is the HTML "class" attribute used to
    # contain CSS classes. But other keyword attributes exist, for instance
    # {the "rel" attribute}[https://developer.mozilla.org/en-US/docs/Web/HTML/Attributes/rel].
    #
    # @see #remove_class
    # @see #kwattr_values
    # @see #kwattr_add
    # @see #kwattr_append
    #
    # @param attribute_name [String] The name of the keyword attribute to be modified.
    # @param keywords [String, Array<String>]
    #   Keywords to be added to the attribute named +attribute_name+. May be a string containing
    #   whitespace-delimited values, or an Array of String values. Any values already present will
    #   not be added. Any values not present will be added. If the named attribute does not exist,
    #   it is created.
    #
    # @return [Node] +self+ for ease of chaining method calls.
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

    # Write Node to +io+.
    def write_to(io, *options)
      io.write(to_html(*options))
    end

    alias_method :write_html_to, :write_to

    private

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
