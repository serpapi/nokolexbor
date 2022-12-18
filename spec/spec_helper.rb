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