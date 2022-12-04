# frozen_string_literal: true

module Nokolexbor
  class Attribute
    attr_accessor :name
    attr_accessor :value

    def initialize(name, value)
      @name = name
      @value = value
    end

    alias_method :text, :value
    alias_method :content, :value
    alias_method :to_s, :value
  end
end