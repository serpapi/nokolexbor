# frozen_string_literal: true

require 'rake/testtask'
require 'open3'
require 'timeout'
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
      bin = File.join(tmpdir, 'tsan_test')

      compile_cmd = "#{cc} #{tsan_flags} -I#{lexbor_include} -o #{bin} #{src} #{lexbor_static} #{link_flags}"
      puts "Compiling..."
      unless system(compile_cmd)
        abort "test:tsan: failed to compile. Command:\n  #{compile_cmd}"
      end

      puts "Running with ThreadSanitizer..."
      _out, err, status = Open3.capture3(bin)
      puts _out unless _out.empty?

      if err.include?('ThreadSanitizer: data race')
        puts err
        abort "test:tsan FAILED: TSan detected data race(s)"
      elsif !status.success?
        puts err unless err.empty?
        abort "test:tsan FAILED: exit code #{status.exitstatus}"
      else
        puts "PASS: no data races detected"
      end
    end
  end
end
