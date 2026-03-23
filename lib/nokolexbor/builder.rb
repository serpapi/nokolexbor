# frozen_string_literal: true

require 'set'

module Nokolexbor
  # A DSL for programmatically building HTML documents.
  #
  # == No-arg block (instance_eval style) – tag names are called bare:
  #
  #   Nokolexbor do
  #     body do
  #       h1 'Hello world'
  #       p 'This little p'
  #       ul do
  #         li 'Go to market'
  #         li 'Go to bed'
  #       end
  #     end
  #   end
  #
  # == Block-parameter style – tag names are called on the builder argument:
  #
  #   Nokolexbor::Builder.new do |b|
  #     b.body do
  #       b.h1 'Hello world'
  #     end
  #   end
  #
  # == Building into an existing node:
  #
  #   Nokolexbor::Builder.with(existing_node) do
  #     span 'injected'
  #   end
  #
  class Builder
    # The Document used to create new nodes
    attr_accessor :doc

    # The node that currently receives new children
    attr_accessor :parent

    # Arity of the outermost block (drives instance_eval vs yield dispatch)
    attr_accessor :arity

    # Build into an existing +root+ node.
    def self.with(root, &block)
      new(root, &block)
    end

    # @param root [Node, nil]
    #   When given, new nodes are appended under +root+ and +root.document+
    #   is used as the owning document.  When omitted a fresh {Document} and
    #   a {DocumentFragment} container are created.
    def initialize(root = nil, &block)
      if root
        @doc    = root.document
        @parent = root
      else
        @doc    = Document.new
        @parent = DocumentFragment.new(@doc)
      end

      @context = nil
      @arity   = nil

      return unless block

      @arity = block.arity
      if @arity <= 0
        # Capture outer self so that outer methods / ivars remain accessible
        # via method_missing delegation while the builder acts as self.
        @context = eval("self", block.binding)
        instance_eval(&block)
      else
        yield self
      end
    end

    # Insert a Text node containing +string+.
    def text(string)
      insert(@doc.create_text_node(string))
    end

    # Insert a CDATA node containing +string+.
    def cdata(string)
      insert(@doc.create_cdata(string))
    end

    # Insert a Comment node containing +string+.
    def comment(string)
      insert(@doc.create_comment(string))
    end

    # Append raw HTML (parsed into nodes) under the current parent.
    def <<(string)
      @doc.fragment(string).children.each { |x| insert(x) }
    end

    # Ruby's Kernel#p, Kernel#pp, etc. are defined on Object and therefore on
    # Builder itself.  They must be overridden so they fall through to
    # method_missing and create the corresponding HTML element instead of
    # printing values to stdout.
    def p(*args, &block) # :nodoc:
      method_missing(:p, *args, &block)
    end

    def method_missing(method, *args, &block) # :nodoc:
      # Only delegate to the outer context for methods the user defined there.
      # Standard Object / Kernel methods (like :p, :pp, :puts …) must not be
      # forwarded to @context because all objects respond to them, which would
      # bypass element creation entirely.
      if @context && !OBJECT_INSTANCE_METHODS.include?(method) && @context.respond_to?(method)
        @context.send(method, *args, &block)
      else
        node = @doc.create_element(method.to_s.sub(/[_!]$/, ""), *args)
        insert(node, &block)
      end
    end

    def respond_to_missing?(method, include_private = false) # :nodoc:
      (@context && !OBJECT_INSTANCE_METHODS.include?(method) && @context.respond_to?(method)) || super
    end

    # Methods defined on plain Object that should never be forwarded to @context
    # as HTML element creation fallbacks.
    OBJECT_INSTANCE_METHODS = Object.instance_methods.to_set.freeze

    private

    # Add +node+ as a child of @parent; if a block is given, push @parent
    # to +node+ for the duration of that block, then restore it.
    # Returns a {NodeBuilder} for the inserted node so attributes / classes
    # can be chained: <tt>div.container.hero!</tt>
    def insert(node, &block)
      node = @parent.add_child(node)
      if block
        old_parent = @parent
        @parent    = node
        @arity   ||= block.arity
        begin
          if @arity <= 0
            instance_eval(&block)
          else
            yield(self)
          end
        ensure
          @parent = old_parent
        end
      end
      NodeBuilder.new(node, self)
    end

    # Wraps a built node and exposes a fluent API for setting classes and ids:
    #
    #   div.container        # => <div class="container">
    #   div.box.highlight    # => <div class="box highlight">
    #   div.thing!           # => <div id="thing">
    #   div.container.hero!  # => <div class="container" id="hero">
    #
    class NodeBuilder # :nodoc:
      def initialize(node, doc_builder)
        @node        = node
        @doc_builder = doc_builder
      end

      def []=(k, v)
        @node[k] = v
      end

      def [](k)
        @node[k]
      end

      def method_missing(method, *args, &block)
        opts = args.last.is_a?(Hash) ? args.pop : {}

        case method.to_s
        when /^(.*)!$/
          @node["id"]  = Regexp.last_match(1)
          @node.content = args.first if args.first
        when /^(.*)=$/
          @node[Regexp.last_match(1)] = args.first
        else
          @node["class"] =
            ((@node["class"] || "").split(/\s/) + [method.to_s]).join(" ")
          @node.content = args.first if args.first
        end

        opts.each do |k, v|
          @node[k.to_s] = ((@node[k.to_s] || "").split(/\s/) + [v.to_s]).join(" ")
        end

        if block
          old_parent           = @doc_builder.parent
          @doc_builder.parent  = @node
          arity = @doc_builder.arity || block.arity
          value = if arity <= 0
            @doc_builder.instance_eval(&block)
          else
            yield(@doc_builder)
          end
          @doc_builder.parent = old_parent
          return value
        end

        self
      end

      def respond_to_missing?(_method, _include_private = false) # :nodoc:
        true
      end
    end
  end
end
