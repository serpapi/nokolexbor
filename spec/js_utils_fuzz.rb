# Builds unescape_js inputs out of the token shapes the eight passes react to,
# so the generated corpus keeps hitting pass boundaries, cascades, lookbehinds
# and surrogate edges instead of random bytes that every pass ignores.
module JSUtilsFuzz
  TOKENS = [
    '\\', '\\\\', '\\\\\\', 'a', 'ab', ' ', '"', "'", '>', '<', '&', '=', '-', ';', '`',
    '\x41', '\x5c', '\x5C', '\X41', '\xg', '\x4', '\xa', '\xAb', '\x00', '\xff', '\xFF',
    '\101', '\134', '\000', '\377', '\400', '\8', '\07',
    '\u0041', '\u005c', '\u005C', '\u0027', '\u0022', '\u003e', '\u0026', '\u002d',
    '\u0003', '\u000b', '\u0019', '\u001c', '\u001d', '\U0041', '\u00e9', '\u00E9',
    '\\\\u005c', '\\\\u0027', '\\\\U0027', '\\\\u0003', '\\\\u001C', '\\\\u0041',
    '\ud83d', '\ude00', '\udc00', '\udbff', '\ud800', '\ud83d\ude00', '\ud83d\u0041',
    '\n', '\t', '\r', '\0', '\a', '\b', '\v', '\f', '\u', '\x', '\q', '\é', '\😀',
    "\n", "\t", "\u0003", "\u000b", "\u001c", "\u001d", "\u0019", "\0",
    'é', '日', '😀', 'ü', "\u00a0",
    '\\u', '\\\\x', '\\x5c\\u0027', '\\134\\u0027', '\\x5c\\x5c', '%', '{', '}', '[', ']',
  ].freeze

  module_function

  # Yields `count` inputs, reproducible for a given seed. Half are built from
  # TOKENS, half are mutations of `seeds` (slices of real captured inputs) when
  # seeds are given.
  def each(count, seed, seeds: nil)
    random = Random.new(seed)
    count.times do |index|
      yield(seeds && !seeds.empty? && index.odd? ? mutate(seeds[random.rand(seeds.size)], random) : synthesize(random))
    end
  end

  def synthesize(random)
    string = +''
    random.rand(1..12).times { string << TOKENS[random.rand(TOKENS.size)] }
    string.force_encoding('UTF-8')
  end

  # Escape-aware edits: the interesting inputs are the ones that cut a seed in
  # the middle of an escape, double a backslash or flip the case of a hex digit.
  def mutate(seed, random)
    string = seed.dup.force_encoding('UTF-8')
    random.rand(1..3).times do
      case random.rand(8)
      when 0
        at = string.empty? ? 0 : random.rand(string.bytesize)
        string = string.byteslice(0, at).to_s + TOKENS[random.rand(TOKENS.size)] + string.byteslice(at..-1).to_s
      when 1
        at = string.empty? ? 0 : random.rand(string.bytesize)
        string = string.byteslice(0, at).to_s + string.byteslice(at + random.rand(1..8)..-1).to_s
      when 2
        at = string.index('\\')
        string = at ? string[0, at].to_s + '\\' + string[at..-1].to_s : string + '\\'
      when 3
        next unless string.valid_encoding?

        at = string.empty? ? 0 : random.rand(string.size)
        string = string[0, at].to_s + string[at, 8].to_s.swapcase + string[(at + 8)..-1].to_s
      when 4
        string = string.byteslice(0, random.rand(string.bytesize + 1)).to_s
      when 5
        at = string.empty? ? 0 : random.rand(string.bytesize)
        string = string.byteslice(0, at).to_s + ["\u0003", "\u000b", "\u001c", "\n", "\0"].sample(random: random) + string.byteslice(at..-1).to_s
      when 6
        string = string * 2 if string.bytesize < 512
      else
        at = string.empty? ? 0 : random.rand(string.bytesize)
        string = string.byteslice(at, random.rand(1..96)).to_s
      end
    end
    string.force_encoding('UTF-8')
  end

  # Random escape-dense slices of a capture, usable as mutation seeds.
  def seeds_from(dir, count, seed, method_id = 1)
    random = Random.new(seed)
    seeds = []
    each_captured(dir, method_id) do |input|
      break if seeds.size >= count
      next if input.bytesize < 64

      3.times do
        at = random.rand(input.bytesize - 48)
        at -= 1 while at.positive? && input.getbyte(at) != 0x5c && at > 0 && input.getbyte(at - 1) != 0x5c
        seeds << input.byteslice(at, random.rand(8..160)).to_s
      end
    end
    seeds
  end

  # Yields every input in a capture written by SerpApi's tmp/jsu/capture.rb:
  # records of [u8 method][u8 flags][u8 encoding][u8 pad][u32le length][bytes].
  def each_captured(dir, method_id = 1)
    encodings = ['UTF-8', 'ASCII-8BIT', 'US-ASCII']
    Dir[File.join(dir, 'part-*.bin')].sort.each do |path|
      File.open(path, 'rb') do |file|
        while (header = file.read(8)) && header.bytesize == 8
          record_method, _flags, encoding, _pad, length = header.unpack('CCCCV')
          bytes = file.read(length)
          next unless record_method == method_id

          yield bytes.force_encoding(encodings[encoding] || 'UTF-8')
        end
      end
    end
  end
end
