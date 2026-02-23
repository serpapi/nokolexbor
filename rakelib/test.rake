# frozen_string_literal: true

require 'rake/testtask'
require 'open3'
require 'tmpdir'

# Task to detect memory leaks with ASAN
# The extension should be compiled with `NOKOLEXBOR_ASAN=1 rake compile`
# This task can only be run on linux
class ASanTestTask < Rake::TestTask
  def filter_leak_message(output)
    # Discard ruby only leaks (false positives)
    results = output.scan(/(?:Direct|Indirect).*?\n\n/m).select { |r| r.include? 'lexbor' }
    results.join
  end

  def ruby(*args, **options, &block)
    unless RbConfig::CONFIG['host_os'].include?('linux')
      puts 'Error: test:asan can only be run on linux.'
      return
    end

    asan_so = `gcc -print-file-name=libasan.so`.strip
    env = {"LD_PRELOAD" => asan_so}
    if args.length > 1
      stdout, stderr, status = Open3.capture3(env, FileUtils::RUBY, *args, **options, &block)
    else
      stdout, stderr, status = Open3.capture3(env, "#{FileUtils::RUBY} #{args.first}", **options, &block)
    end

    puts stdout
    unless (leaks = filter_leak_message(stderr)).empty?
      puts
      puts leaks
      yield false, status
    end
  end
end

Rake::TestTask.new do |t|
  t.libs << 'spec'
  t.pattern = 'spec/**/*_spec.rb'
end

namespace :test do
  Rake::TestTask.new('gem:run') do |t|
    t.libs << 'spec'
    t.libs.delete('lib')
    t.pattern = 'spec/**/*_spec.rb'
  end

  task :gem do
    begin
      ENV['NOKOLEXBOR_TEST_GEM'] = '1'
      FileUtils.mv('lib', 'lib_tmp', force: true)
      Rake::Task["test:gem:run"].invoke
    ensure
      FileUtils.mv('lib_tmp', 'lib', force: true)
    end
  end

  ASanTestTask.new('asan') do |t|
    t.libs << 'spec'
    t.pattern = 'spec/**/*_spec.rb'
  end

  # Task to prove the static-singleton data race with ThreadSanitizer.
  # Compiles test/tsan_race_test.c in both buggy and fixed variants and
  # verifies that TSan detects races in the buggy variant and none in the fixed.
  # Can only run on Linux or macOS (TSan not supported on Windows).
  task :tsan do
    if RbConfig::CONFIG['host_os'].match?(/mingw|mswin|cygwin/)
      puts 'Skipping test:tsan — ThreadSanitizer is not supported on Windows.'
      next
    end

    lexbor_static = File.expand_path('vendor/lexbor/dist/lib/liblexbor_static.a')
    lexbor_include = File.expand_path('vendor/lexbor/dist/include')
    src = File.expand_path('test/tsan_race_test.c')

    unless File.exist?(lexbor_static)
      abort "test:tsan requires vendor/lexbor/dist to be built first. Run: rake compile"
    end

    cc = ENV['CC'] || (system('which clang > /dev/null 2>&1') ? 'clang' : 'gcc')
    tsan_flags = "-fsanitize=thread -g -O1"
    link_flags = RbConfig::CONFIG['host_os'].include?('linux') ? "-lpthread -lm" : "-lpthread"

    Dir.mktmpdir('tsan_test') do |tmpdir|
      buggy_bin = File.join(tmpdir, 'tsan_buggy')
      fixed_bin = File.join(tmpdir, 'tsan_fixed')

      # Compile buggy variant
      compile_buggy = "#{cc} #{tsan_flags} -I#{lexbor_include} -o #{buggy_bin} #{src} #{lexbor_static} #{link_flags}"
      puts "Compiling buggy variant..."
      unless system(compile_buggy)
        abort "test:tsan: failed to compile buggy variant. Command:\n  #{compile_buggy}"
      end

      # Compile fixed variant
      compile_fixed = "#{cc} #{tsan_flags} -DFIXED -I#{lexbor_include} -o #{fixed_bin} #{src} #{lexbor_static} #{link_flags}"
      puts "Compiling fixed variant..."
      unless system(compile_fixed)
        abort "test:tsan: failed to compile fixed variant. Command:\n  #{compile_fixed}"
      end

      # Run buggy — expect TSan warnings
      puts "\nRunning buggy variant (expect TSan data race warnings)..."
      _buggy_out, buggy_err, _buggy_status = Open3.capture3(buggy_bin)
      buggy_has_race = buggy_err.include?('ThreadSanitizer: data race')

      # Run fixed — expect clean output
      puts "Running fixed variant (expect no TSan warnings)..."
      _fixed_out, fixed_err, fixed_status = Open3.capture3(fixed_bin)
      fixed_has_race = fixed_err.include?('ThreadSanitizer: data race')

      puts "\n--- TSan results ---"
      if buggy_has_race
        puts "PASS: buggy variant correctly shows data race(s)"
      else
        puts "WARN: buggy variant showed no TSan warnings (race not triggered this run)"
      end

      if fixed_has_race
        puts "FAIL: fixed variant has TSan warnings — the fix is broken!"
        puts fixed_err
        abort "test:tsan failed: fixed variant has data races"
      else
        puts "PASS: fixed variant is clean (exit #{fixed_status.exitstatus})"
      end
    end
  end
end
