require 'spec_helper'

describe Nokolexbor::JSUtils do
  def unescape(input)
    Nokolexbor::JSUtils.unescape_js(input)
  end

  it 'decodes single-character escapes' do
    _(unescape('a\b\f\n\r\t\vz')).must_equal "a\b\f\n\r\t\vz"
    _(unescape(%q(\'\"\\\\\/))).must_equal %q('"\\/)
    _(unescape('\a\c\d\e\g\-\=\<\>\&\`')).must_equal 'acdeg-=<>&`'
  end

  it 'decodes \xHH' do
    _(unescape('\x41\x5c\x5C\x22\xe9\xFF\x00')).must_equal "A\\\\\"\u00e9\u00ff\u0000"
  end

  it 'decodes \uHHHH and surrogate pairs' do
    _(unescape('\u0041\u005c\u00e9\u65e5\uFFFF')).must_equal "A\\\u00e9\u65e5\uffff"
    _(unescape('\ud83d\ude00')).must_equal "\u{1f600}"
    _(unescape('\uD83D\uDE00!')).must_equal "\u{1f600}!"
  end

  it 'replaces unpaired surrogates with U+FFFD' do
    _(unescape('\ud83d')).must_equal "\ufffd"
    _(unescape('\ude00')).must_equal "\ufffd"
    _(unescape('\ud83d\u0041')).must_equal "\ufffdA"
    _(unescape('\ud83dx\ude00')).must_equal "\ufffdx\ufffd"
    _(unescape('\ude00\ud83d')).must_equal "\ufffd\ufffd"
    _(unescape('\u{d800}')).must_equal "\ufffd"
  end

  it 'decodes \u{...} code points' do
    _(unescape('\u{41}\u{0000041}\u{1F600}\u{10FFFF}')).must_equal "AA\u{1f600}\u{10ffff}"
  end

  it 'decodes legacy octal escapes' do
    _(unescape('\0')).must_equal "\u0000"
    _(unescape('\08\09')).must_equal "\u00008\u00009"
    _(unescape('\1\7\12\101\377')).must_equal "\u0001\u0007\nA\u00ff"
    _(unescape('\400\477\1234')).must_equal " 0'7S4"
    _(unescape('\8\9')).must_equal '89'
  end

  it 'removes line continuations' do
    _(unescape("a\\\nb\\\r\nc\\\rd\\\u2028e\\\u2029f")).must_equal 'abcdef'
  end

  it 'decodes each escape once, left to right' do
    _(unescape('\\\\x41')).must_equal '\x41'
    _(unescape('\\\\u0041')).must_equal '\u0041'
    _(unescape('\x5cx41')).must_equal '\x41'
    _(unescape('\u005cu0041')).must_equal '\u0041'
    _(unescape('\\\\\\\\')).must_equal '\\\\'
  end

  it 'keeps malformed escapes as written' do
    _(unescape('\x\x4\xg1\u\u12\u12g4\u{\u{}\u{110000}\u{41')).must_equal '\x\x4\xg1\u\u12\u12g4\u{\u{}\u{110000}\u{41'
    _(unescape('end\\')).must_equal 'end\\'
    _(unescape('\\')).must_equal '\\'
  end

  it 'escapes multibyte characters to themselves and copies everything else' do
    _(unescape("\\\u00e9\\\u{1f600}\u65e5")).must_equal "\u00e9\u{1f600}\u65e5"
    _(unescape("no escapes \u00e9 \u0003")).must_equal "no escapes \u00e9 \u0003"
    _(unescape('')).must_equal ''
  end

  it 'returns a new UTF-8 string and leaves the argument alone' do
    argument = +'\x41'.encode('US-ASCII')
    result = unescape(argument)
    _(result).must_equal 'A'
    _(result.encoding).must_equal Encoding::UTF_8
    _(argument).must_equal '\x41'
    _(unescape('plain'.dup.freeze)).must_equal 'plain'
    _(unescape('\x41'.b)).must_equal 'A'
  end

  it 'rejects invalid or non-UTF-8 input' do
    _ { unescape("\xff".dup.force_encoding('UTF-8')) }.must_raise ArgumentError
    _ { unescape("caf\u00e9".encode('ISO-8859-1')) }.must_raise ArgumentError
    _ { unescape(nil) }.must_raise TypeError
  end

  # Random strings built from escape-heavy tokens, checked against a plain Ruby
  # reading of the same rules.
  def reference(input)
    out = +''
    s = input
    i = 0
    while i < s.length
      c = s[i]
      if c != '\\'
        out << c
        i += 1
        next
      end
      n = s[i + 1]
      if n.nil?
        out << '\\'
        break
      end
      rest = s[(i + 1)..]
      case n
      when 'b' then out << "\b"; i += 2
      when 'f' then out << "\f"; i += 2
      when 'n' then out << "\n"; i += 2
      when 'r' then out << "\r"; i += 2
      when 't' then out << "\t"; i += 2
      when 'v' then out << "\v"; i += 2
      when "\n", "\u2028", "\u2029" then i += 2
      when "\r" then i += s[i + 2] == "\n" ? 3 : 2
      when '8', '9' then out << n; i += 2
      when '0'..'7'
        digits = rest[/\A[0-7]{1,#{n <= '3' ? 3 : 2}}/]
        out << digits.to_i(8).chr(Encoding::UTF_8)
        i += 1 + digits.length
      when 'x'
        if (hex = rest[/\Ax\h{2}/])
          out << hex[1..].to_i(16).chr(Encoding::UTF_8)
          i += 4
        else
          out << '\\'
          i += 1
        end
      when 'u'
        if (m = rest.match(/\Au\{(\h+)\}/)) && m[1].to_i(16) <= 0x10FFFF
          cp = m[1].to_i(16)
          out << ((0xD800..0xDFFF).cover?(cp) ? "\ufffd" : cp.chr(Encoding::UTF_8))
          i += 1 + m[0].length
        elsif (m = rest.match(/\Au(\h{4})/))
          unit = m[1].to_i(16)
          low = rest[/\Au\h{4}\\u(\h{4})/, 1]&.to_i(16)
          if (0xD800..0xDBFF).cover?(unit) && low && (0xDC00..0xDFFF).cover?(low)
            out << (0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00)).chr(Encoding::UTF_8)
            i += 12
          else
            out << ((0xD800..0xDFFF).cover?(unit) ? "\ufffd" : unit.chr(Encoding::UTF_8))
            i += 6
          end
        else
          out << '\\'
          i += 1
        end
      else
        out << n
        i += 2
      end
    end
    out
  end

  it 'agrees with the reference rules on 20,000 random strings' do
    tokens = ['\\', '\\\\', 'a', 'é', '😀', "\n", "\r", "\u2028", '"', "'", 'x', 'u', '{', '}', '0', '4', '7', '8',
              '\x41', '\x5c', '\xg', '\x4', '\u0041', '\u005c', '\ud83d', '\ude00', '\ud83d\ude00', '\u{41}',
              '\u{1f600}', '\u{110000}', '\u{', '\0', '\08', '\12', '\377', '\400', '\8', '\n', '\t', '\q', '\é']
    random = Random.new(20261015)
    20_000.times do
      input = Array.new(random.rand(1..12)) { tokens[random.rand(tokens.length)] }.join
      _(unescape(input)).must_equal reference(input), "input: #{input.inspect}"
    end
  end
end
