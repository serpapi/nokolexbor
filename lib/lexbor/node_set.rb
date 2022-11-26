# frozen_string_literal: true

module Lexbor
  class NodeSet < Array
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
      self.map { |node| node.css(selector) }.flatten(1)
    end
      
  end
end