# frozen_string_literal: true

require 'rake/testtask'
require 'open3'

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
end
