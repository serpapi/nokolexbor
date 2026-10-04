# Builds the corpus js_utils_spec.rb reads through JSU_CORPUS: every
# JSUtils.unescape_js and JSUtils.unescape input the SerpApi Google parser sees
# while parsing the 2,760 searches its specs parse, deduplicated.
#
# Copy it into a SerpApi checkout (it needs that app, not this gem) and run it
# from there; the output is far too large for git.
#
#   cp spec/capture_serpapi_inputs.rb <serpapi>/tmp/jsu/capture.rb
#   cd <serpapi> && GATE_PROCESSES=4 bundle exec ruby tmp/jsu/capture.rb
#   JSU_CORPUS=<serpapi>/tmp/jsu/corpus rake test
#
# Records are [u8 method][u8 flags][u8 encoding][u8 pad][u32le length][bytes],
# which is what JSUtilsFuzz.each_captured reads.
require "json"
require "digest"
require "fileutils"

root = File.expand_path("../..", __dir__)
corpus_path = ENV.fetch("CORPUS", File.join(root, "tmp/search-bench.noindex/corpus.jsonl"))
unless File.exist?(corpus_path)
  FileUtils.mkdir_p(File.dirname(corpus_path))
  File.write(corpus_path, `#{RbConfig.ruby} #{File.join(root, "ext/search-bench/corpus.rb")}`)
end
entries = File.readlines(corpus_path).map { |line| JSON.parse(line) }

require_relative "../../ext/search-bench/app/boot"

out_dir = File.join(root, "tmp/jsu/corpus")
FileUtils.rm_rf(out_dir)
FileUtils.mkdir_p(out_dir)

ENCODINGS = ["UTF-8", "ASCII-8BIT", "US-ASCII"].freeze

module JSUtilsCapture
  class << self
    attr_accessor :io, :seen

    def record(method_id, flags, string)
      return unless io
      return unless string.is_a?(String)

      digest = Digest::SHA256.digest("#{method_id}#{flags}#{string.encoding.name}#{string}")
      return if seen.include?(digest)

      seen << digest
      bytes = string.b
      enc = ENCODINGS.index(string.encoding.name) || 255
      io.write([method_id, flags, enc, 0, bytes.bytesize].pack("CCCCV"))
      io.write(bytes)
    end
  end

  def unescape_js(string)
    JSUtilsCapture.record(1, 0, string)
    super
  end

  def unescape(string, preserve_control_characters: false)
    JSUtilsCapture.record(2, preserve_control_characters ? 1 : 0, string)
    super
  end
end
JSUtils.singleton_class.prepend(JSUtilsCapture)

processes = Integer(ENV.fetch("GATE_PROCESSES", "4"))
started = Process.clock_gettime(Process::CLOCK_MONOTONIC)
pids = processes.times.map do |worker|
  fork do
    JSUtilsCapture.seen = Set.new
    JSUtilsCapture.io = File.open(File.join(out_dir, "part-#{worker}.bin"), "wb")
    entries.each_with_index do |entry, index|
      next unless index % processes == worker

      params = entry["params"].transform_keys(&:to_sym)
      begin
        Search.new(params).hash_with_base64_images
      rescue StandardError
        nil
      end
    end
    JSUtilsCapture.io.close
  end
end
pids.each { |pid| Process.wait(pid) }
elapsed = Process.clock_gettime(Process::CLOCK_MONOTONIC) - started
size = Dir[File.join(out_dir, "part-*.bin")].sum { |path| File.size(path) }
puts format("captured %.1f MB in %.1f s into %s", size / 1e6, elapsed, out_dir)
