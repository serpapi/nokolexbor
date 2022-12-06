# frozen_string_literal: true

module Nokolexbor
  class NodeSet
    include Enumerable

    def initialize(document, list = [])
      @document = document
      list.each { |x| self << x }
      yield self if block_given?
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

    alias_method :to_html, :outer_html

    def remove
      self.each(&:remove)
    end

    alias_method :destroy, :remove

    def at_css(selector)
      self.each do |node|
        if child = node.at_css(selector)
          return child
        end
      end
      nil
    end

    def css(selector)
      ret = []
      each do |node|
        node.css(selector).each do |inner_node|
          ret << inner_node
        end
      end
      NodeSet.new(@document, ret)
      # self.map { |node| node.css(selector) }.flatten(1)
    end
      
  end
end