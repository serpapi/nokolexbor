require_relative 'js_utils_reference'
require 'spec_helper'
require_relative 'js_utils_fuzz'

# Nokolexbor::JSUtils.unescape_js is a C port of SerpApi's JSUtils.unescape_js.
# These tests compare it against the Ruby original (js_utils_reference.rb) on
# hand-picked corners, on a seeded fuzz corpus and, when JSU_CORPUS points at a
# capture, on every real input the SerpApi Google parser produced.
describe Nokolexbor::JSUtils do
  before do
    unless JSUTILS_REFERENCE_JSON_MATCHES
      skip "json #{JSON::VERSION} is loaded; the port reproduces #{JSUTILS_REFERENCE_JSON_VERSION}"
    end
  end

  def assert_same_as_ruby(input)
    difference = JSUtilsDiff.compare(input)
    assert_nil difference, difference
  end

  it 'leaves a string without escapes alone and returns the same object' do
    string = +'nothing to do here'
    assert_same_as_ruby(string)
    _(Nokolexbor::JSUtils.unescape_js(string)).must_be_same_as string
  end

  it 'returns a new object once the \u branch runs, and the argument keeps the first three passes' do
    argument = +'a\x41\u0041b'
    returned = Nokolexbor::JSUtils.unescape_js(argument)
    _(returned).must_equal 'aAAb'
    _(argument).must_equal 'aA\u0041b'
    _(returned.equal?(argument)).must_equal false
  end

  it 'cascades passes: a backslash one pass produces is consumed by the next' do
    [
      '\134x41',      # octal -> \x41 -> A
      '\x5cx41',      # \x5c  -> \    -> \x41 -> A
      '\x5cu0027',    # \x5c  -> \    -> \u0027 -> '
      '\x5c\u0027',   # \x5c  -> \    -> \\u0027 -> JS_UNESCAPES -> '
      '\x5c\u0003',   # ESCAPED_CONTROL_CHARS
      '\134\u0027',
    ].each { |input| assert_same_as_ruby(+input) }
  end

  it 'matches the negative lookbehind' do
    ['\\\\u0041', '\\\\\\u0041', 'a\\\\u0027', '\\\\u0027\\u0027', '\u0041\\\\u0041'].each do |input|
      assert_same_as_ruby(+input)
    end
  end

  it 'matches json 2.6.2 on surrogates' do
    [
      '\ud83d\ude00',  # a pair
      '\ud83d\u0041',  # json pairs a high surrogate with whatever follows
      '\udc00',        # a lone low surrogate: invalid UTF-8, then ArgumentError
      '\ud83d',        # a high surrogate at the end: JSON raises, `rescue m`
      'a\ud83db',
      '\ud800\udbff',
      '\u0000\uffff',
    ].each { |input| assert_same_as_ruby(+input) }
  end

  it 'falls back to the pass without the lookbehind when the first one breaks the encoding' do
    assert_same_as_ruby(+'\\\\ud83d\\ude00')
    assert_same_as_ruby(+'a\\\\udc00\\ud83d')
  end

  it 'raises ArgumentError like Onigmo, after having mutated the argument' do
    argument = +'\x41\udc00'
    error = _ { Nokolexbor::JSUtils.unescape_js(argument) }.must_raise ArgumentError
    _(error.message).must_equal 'invalid byte sequence in UTF-8'
    _(argument).must_equal 'A\udc00'
  end

  it 'raises ArgumentError on a broken argument without touching it' do
    argument = (+"a\xED\xA0\xBD\\x41").force_encoding('UTF-8')
    _ { Nokolexbor::JSUtils.unescape_js(argument) }.must_raise ArgumentError
    _(argument.b).must_equal "a\xED\xA0\xBD\\x41".b
  end

  it 'raises FrozenError on a frozen argument, matched or not' do
    _ { Nokolexbor::JSUtils.unescape_js('nothing'.freeze) }.must_raise FrozenError
    _ { Nokolexbor::JSUtils.unescape_js('\x41'.freeze) }.must_raise FrozenError
  end

  it 'applies UNESCAPE_CHARS, including the entries that keep the backslash' do
    ['\0', '\a', '\b', '\t', '\n', '\v', '\f', '\r', '\u', '\x', '\q', '\\\\', '\é', '\😀', 'a\\'].each do |input|
      assert_same_as_ruby(+input)
    end
  end

  it 'does not let "." match a newline' do
    assert_same_as_ruby(+"a\\\nb")
    assert_same_as_ruby(+"\\\n\\t")
  end

  it 'removes control characters, escaped and literal' do
    assert_same_as_ruby(+"a\u0003b\u001cc")
    assert_same_as_ruby(+'a\\\\u0003b\\\\U001Cc')
  end

  it 'handles the empty string' do
    assert_same_as_ruby(+'')
  end

  it 'agrees with Ruby on a seeded fuzz corpus' do
    cases = Integer(ENV.fetch('JSU_FUZZ_CASES', '20000'))
    failures = 0
    JSUtilsFuzz.each(cases, Integer(ENV.fetch('JSU_FUZZ_SEED', '20261004'))) do |input|
      difference = JSUtilsDiff.compare(input)
      next unless difference

      failures += 1
      warn difference if failures <= 5
    end
    assert_equal 0, failures, "#{failures} of #{cases} fuzz cases differ"
  end

  it 'agrees with Ruby on every captured real input' do
    dir = ENV['JSU_CORPUS']
    skip 'set JSU_CORPUS to a capture directory' if dir.nil? || Dir[File.join(dir, 'part-*.bin')].empty?

    checked = 0
    failures = 0
    JSUtilsFuzz.each_captured(dir) do |input|
      checked += 1
      difference = JSUtilsDiff.compare(input)
      next unless difference

      failures += 1
      warn difference if failures <= 5
    end
    assert_operator checked, :>, 0
    assert_equal 0, failures, "#{failures} of #{checked} real inputs differ"
  end
end
