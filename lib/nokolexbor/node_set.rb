# frozen_string_literal: true

module Nokolexbor
  class NodeSet < Nokolexbor::Node
    include Enumerable

    # Create a NodeSet with +document+ defaulting to +list+.
    #
    # @yield [Document]
    #
    # @return [Document]
    def self.new(document, list = [])
      obj = allocate
      obj.instance_variable_set(:@document, document)
      list.each { |x| obj << x }
      yield obj if block_given?
      obj
    end

    # Iterate over each node.
    #
    # @yield [Node]
    def each
      return to_enum unless block_given?

      0.upto(length - 1) do |x|
        yield self[x]
      end
      self
    end

    # Get the first +n+ elements of the NodeSet.
    #
    # @param n [Numeric,nil]
    #
    # @return [Node,Array<Node>] {Node} if +n+ is nil, otherwise {Array<Node>}
    def first(n = nil)
      return self[0] unless n

      list = []
      [n, length].min.times { |i| list << self[i] }
      list
    end

    # Get the last element of the NodeSet.
    #
    # @return [Node,nil]
    def last
      self[-1]
    end

    # @return [Boolean] true if this NodeSet is empty.
    def empty?
      length == 0
    end

    # Insert +node+ before the first Node in this NodeSet
    def before(node)
      first.before(node)
    end

    # Insert +node+ after the last Node in this NodeSet
    def after(node)
      last.after(node)
    end

    # @return [Integer] The index of the first node in this NodeSet that is equal to +node+ or meets the given block. Returns nil if no match is found.
    def index(node = nil)
      if node
        each_with_index { |member, j| return j if member == node }
      elsif block_given?
        each_with_index { |member, j| return j if yield(member) }
      end
      nil
    end

    # Get the content of all contained Nodes.
    #
    # @return [String]
    def content
      self.map(&:content).join
    end

    alias_method :text, :content
    alias_method :inner_text, :content
    alias_method :to_str, :content

    # Get the inner html of all contained Nodes.
    #
    # @return [String]
    def inner_html(*args)
      self.map { |n| n.inner_html(*args) }.join
    end

    # Convert this NodeSet to HTML.
    #
    # @return [String]
    def outer_html(*args)
      self.map { |n| n.outer_html(*args) }.join
    end

    alias_method :to_s, :outer_html
    alias_method :to_html, :outer_html
    alias_method :serialize, :outer_html

    # Remove all nodes in this NodeSet.
    #
    # @see Node#remove
    def remove
      self.each(&:remove)
    end

    alias_method :unlink, :remove
    alias_method :to_ary, :to_a

    # Destroy all nodes in the NodeSet.
    #
    # @see Node#destroy
    def destroy
      self.each(&:destroy)
    end

    # @return [Node,nil] The last element of this NodeSet and removes it. Returns
    #   +nil+ if the set is empty.
    def pop
      return nil if length == 0

      delete(last)
    end

    # @return [Node,nil] The first element of this NodeSet and removes it. Returns
    #   +nil+ if the set is empty.
    def shift
      return nil if length == 0

      delete(first)
    end

    # @return [Boolean] true if two NodeSets contain the same number
    #   of elements and each element is equal to the corresponding
    #   element in the other NodeSet.
    def ==(other)
      return false unless other.is_a?(NodeSet)
      return false unless length == other.length

      each_with_index do |node, i|
        return false unless node == other[i]
      end
      true
    end

    # @return [NodeSet] A new NodeSet containing all the children of all the nodes in
    #   the NodeSet.
    def children
      node_set = NodeSet.new(@document)
      each do |node|
        node.children.each { |n| node_set.push(n) }
      end
      node_set
    end

    # @return [NodeSet] A new NodeSet containing all the nodes in the NodeSet
    #   in reverse order.
    def reverse
      node_set = NodeSet.new(@document)
      (length - 1).downto(0) do |x|
        node_set.push(self[x])
      end
      node_set
    end

    # Wrap all nodes of this NodeSet with +node_or_tags+.
    #
    # @see Node#wrap
    #
    # @return [NodeSet] +self+, to support chaining.
    def wrap(node_or_tags)
      map { |node| node.wrap(node_or_tags) }
      self
    end

    # Add the class attribute +name+ to all containing nodes.
    #
    # @see Node#add_class
    def add_class(name)
      each do |el|
        el.add_class(name)
      end
      self
    end

    # Append the class attribute +name+ to all containing nodes.
    #
    # @see Node#append_class
    def append_class(name)
      each do |el|
        el.append_class(name)
      end
      self
    end

    # Remove the class attribute +name+ from all containing nodes.
    #
    # @see Node#remove_class
    def remove_class(name = nil)
      each do |el|
        el.remove_class(name)
      end
      self
    end

    # Remove the attributed named +name+ from all containing nodes.
    #
    # @see Node#remove_attr
    def remove_attr(name)
      each { |el| el.delete(name) }
      self
    end
    alias_method :remove_attribute, :remove_attr

    # Set attributes on each Node in the NodeSet, or get an
    # attribute from the first Node in the NodeSet.
    #
    # @example Get an attribute from the first Node in a NodeSet.
    #   node_set.attr("href")
    #
    # @example Set attributes on each node.
    #   node_set.attr("href" => "http://example.com", "class" => "a")
    #   node_set.attr("href", "http://example.com")
    #   node_set.attr("href") { |node| "http://example.com" }
    #
    # @return [NodeSet] +self+, to support chaining of calls.
    def attr(key, value = nil, &block)
      unless key.is_a?(Hash) || (key && (value || block))
        return first&.attribute(key)
      end

      hash = key.is_a?(Hash) ? key : { key => value }

      hash.each do |k, v|
        each do |node|
          node[k] = v || yield(node)
        end
      end

      self
    end
    alias_method :set, :attr
    alias_method :attribute, :attr

    # (see Node#xpath)
    def xpath(*args)
      paths, handler, ns, binds = extract_params(args)

      NodeSet.new(@document) do |set|
        each do |node|
          node.send(:xpath_internal, node, paths, handler, ns, binds).each do |inner_node|
            set << inner_node
          end
        end
      end
    end

    # (see Node#nokogiri_css)
    def nokogiri_css(*args)
      rules, handler, ns, _ = extract_params(args)
      paths = css_rules_to_xpath(rules, ns)

      NodeSet.new(@document) do |set|
        each do |node|
          node.send(:xpath_internal, node, paths, handler, ns, nil).each do |inner_node|
            set << inner_node
          end
        end
      end
    end

    def inspect
      "[#{map(&:inspect).join(', ')}]"
    end

    private

    IMPLIED_XPATH_CONTEXTS = [".//", "self::"].freeze # :nodoc:
      
  end
end