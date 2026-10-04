# A verbatim copy of SerpApi's JSUtils (app/lib/js_utils.rb, 2026-10) plus a
# comparator. Nokolexbor::JSUtils.unescape_js must be indistinguishable from
# JSUtilsReference.unescape_js: same bytes, same encoding, same object identity,
# same mutation of the argument and the same exceptions.
#
# The \u branch inherits json's surrogate handling, and that changed after
# 2.6.2: given "\ud83d\u0041", json 2.6.2 pairs the high surrogate with whatever
# \uXXXX follows it and yields U+1F441, while json 2.18 raises. SerpApi's
# Gemfile.lock pins 2.6.2, so that is the version the C port reproduces.
JSUTILS_REFERENCE_JSON_VERSION = '2.6.2'
begin
  gem 'json', JSUTILS_REFERENCE_JSON_VERSION
rescue Gem::LoadError
  nil
end
require 'json'
# The tests skip rather than fail when another json is loaded: the difference is
# real behaviour, not a broken port.
JSUTILS_REFERENCE_JSON_MATCHES = JSON::VERSION == JSUTILS_REFERENCE_JSON_VERSION

class JSUtilsReference
  UNESCAPE_CHARS = Hash.new do |hash, char|
    if char[0, 1] == '\\'
      char[1, 1]
    else
      char
    end
  end

  JS_UNESCAPES = Hash.new

  UNESCAPE_CHARS['\0'] = "\0"
  UNESCAPE_CHARS['\a'] = "\a"
  UNESCAPE_CHARS['\b'] = "\b"
  UNESCAPE_CHARS['\t'] = "\t"
  UNESCAPE_CHARS['\n'] = "\n"
  UNESCAPE_CHARS['\v'] = "\v"
  UNESCAPE_CHARS['\f'] = "\f"
  UNESCAPE_CHARS['\r'] = "\r"
  UNESCAPE_CHARS['\u'] = '\u'
  UNESCAPE_CHARS['\x'] = '\x'

  JS_UNESCAPES['\u005c'] = "\\"
  JS_UNESCAPES['\u0027'] = "'"
  JS_UNESCAPES['\u0022'] = "\""
  JS_UNESCAPES['\u003e'] = ">"
  JS_UNESCAPES['\u003c'] = "<"
  JS_UNESCAPES['\u0026'] = "&"
  JS_UNESCAPES['\u003d'] = "="
  JS_UNESCAPES['\u002d'] = "-"
  JS_UNESCAPES['\u003b'] = ";"
  JS_UNESCAPES['\u0060'] = "`"

  ESCAPED_CONTROL_CHARS = ['\u0003', '\u000b', '\u0019', '\u001d', '\u001c']
  CONTROL_CHARS = ESCAPED_CONTROL_CHARS.map { |m| JSON.load(%Q("#{m}")) }

  HEX_LOOKUP = (0..255).each_with_object({}) do |byte, lookup|
    char = [byte].pack('U*')
    hex = byte.to_s(16).rjust(2, '0')

    [hex, hex.upcase, hex.capitalize, hex[0] + hex[1].upcase].each do |variant|
      lookup["\\x#{variant}"] = char
    end
  end.freeze
  HEX_LOOKUP_RE = /\\x[0-9a-fA-F]{2}/

  JS_UNESCAPES_RE = /(?<!\\)(\\(?:#{Regexp.union(JS_UNESCAPES.keys).source}))/i
  ESCAPED_CONTROL_CHARS_RE = /(?<!\\)(\\(?:#{Regexp.union(ESCAPED_CONTROL_CHARS).source}))/i
  CONTROL_CHARS_RE = /(#{Regexp.union(CONTROL_CHARS).source})/

  def self.unescape_js(string)
    string.gsub!(/\\([0-7]{3})/) { [$1.to_i(8)].pack('U*') }
    string.gsub!(HEX_LOOKUP_RE, HEX_LOOKUP)
    string.gsub!(/\\x([0-9a-fA-F])/) { [$1.to_i(16)].pack('U*') }

    if string.include?('\\u')
      if s = string.gsub(/(?<!\\)(\\u[0-9a-fA-F]{4})+/) { |m| JSON.load(%Q("#{m}")) rescue m } and s.valid_encoding?
        string = s
      else
        string = string.gsub(/(\\u[0-9a-fA-F]{4})+/) { |m| JSON.load(%Q("#{m}")) rescue m }
      end
    end

    string.gsub!(JS_UNESCAPES_RE) { |m| JS_UNESCAPES[m[1..-1].downcase] }
    string.gsub!(ESCAPED_CONTROL_CHARS_RE, "")
    string.gsub!(CONTROL_CHARS_RE, "")
    string.gsub!(/\\./) { |m| UNESCAPE_CHARS[m] }

    string
  end
end

module JSUtilsDiff
  module_function

  # Everything observable about one call.
  def observe(implementation, input)
    argument = input.dup
    begin
      returned = implementation.unescape_js(argument)
      {
        bytes: returned.b,
        encoding: returned.encoding.name,
        valid: returned.valid_encoding?,
        same_object: returned.equal?(argument),
        argument: argument.b,
      }
    rescue StandardError, FrozenError => error
      { error: "#{error.class}: #{error.message}", argument: argument.b }
    end
  end

  # nil when the two implementations agree, otherwise a description.
  def compare(input)
    expected = observe(JSUtilsReference, input)
    actual = observe(Nokolexbor::JSUtils, input)
    return nil if expected == actual

    key = (expected.keys | actual.keys).find { |k| expected[k] != actual[k] }
    format("%s differs for %p\n  ruby:   %p\n  native: %p", key, input, expected[key], actual[key])
  end
end
