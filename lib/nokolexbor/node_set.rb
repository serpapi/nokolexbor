# frozen_string_literal: true

module Nokolexbor
  class NodeSet < Nokolexbor::Node
    include Enumerable

    def self.new(document, list = [])
      obj = allocate
      obj.instance_variable_set(:@document, document)
      list.each { |x| obj << x }
      yield obj if block_given?
      obj
    end

    def each
      return to_enum unless block_given?

      0.upto(length - 1) do |x|
        yield self[x]
      end
      self
    end

    def first(n = nil)
      return self[0] unless n

      list = []
      [n, length].min.times { |i| list << self[i] }
      list
    end

    def last
      self[-1]
    end

    def empty?
      length == 0
    end

    def index(node = nil)
      if node
        each_with_index { |member, j| return j if member == node }
      elsif block_given?
        each_with_index { |member, j| return j if yield(member) }
      end
      nil
    end

    def content
      self.map(&:content).join
    end

    alias_method :text, :content
    alias_method :inner_text, :content
    alias_method :to_str, :content

    def inner_html
      self.map(&:inner_html).join
    end

    def outer_html
      self.map(&:outer_html).join
    end

    alias_method :to_s, :outer_html
    alias_method :to_html, :outer_html

    def remove
      self.each(&:remove)
    end

    alias_method :unlink, :remove
    alias_method :to_ary, :to_a

    def destroy
      self.each(&:destroy)
    end

    def pop
      return nil if length == 0

      delete(last)
    end

    def shift
      return nil if length == 0

      delete(first)
    end

    def ==(other)
      return false unless other.is_a?(NodeSet)
      return false unless length == other.length

      each_with_index do |node, i|
        return false unless node == other[i]
      end
      true
    end

    def children
      node_set = NodeSet.new(@document)
      each do |node|
        node.children.each { |n| node_set.push(n) }
      end
      node_set
    end

    def reverse
      node_set = NodeSet.new(@document)
      (length - 1).downto(0) do |x|
        node_set.push(self[x])
      end
      node_set
    end

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
      
  end
end