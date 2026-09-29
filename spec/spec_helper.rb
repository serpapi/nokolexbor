require 'simplecov'

SimpleCov.start do
  add_filter '/spec/'
  coverage_dir 'coverage/ruby'
end

require "minitest/autorun"
require 'nokolexbor'

class String
  def squish
    dup.squish!
  end

  def squish!
    gsub!(/[[:space:]]+/, " ")
    strip!
    self
  end
end