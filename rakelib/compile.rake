# frozen_string_literal: true

require 'rake/extensiontask'

native_gemspec = eval(File.read 'nokolexbor.gemspec').tap do |spec|
  spec.extensions = []
  spec.files = Dir.glob("lib/**/*.rb")
  spec.metadata.delete('msys2_mingw_dependencies')
end

ENV['RUBY_CC_VERSION'] = %w{2.6.0 2.7.0 3.0.0 3.1.0 3.2.0}.join(':')
cross_platforms = %w[x86-mingw32 x64-mingw-ucrt x64-mingw32 x86-linux x86_64-linux aarch64-linux x86_64-darwin arm64-darwin]

Rake::ExtensionTask.new('nokolexbor', native_gemspec) do |ext|
  ext.lib_dir = "lib/nokolexbor"
  ext.cross_compile = true
  ext.cross_platform = cross_platforms
end

namespace "gem" do
  cross_platforms.each do |platform|
    namespace platform do
      task "build" do
        ENV['NOKOLEXBOR_CROSS_COMPILE'] = '1'
        Rake::Task["native:#{platform}"].invoke
        Rake::Task["pkg/#{native_gemspec.full_name}-#{Gem::Platform.new(platform)}.gem"].invoke
      end
    end
  end
end

namespace :clean do
  task :lexbor do
    FileUtils.rm_rf("vendor/lexbor/build")
    FileUtils.rm_rf("vendor/lexbor/dist")
    sh "git submodule foreach git reset --hard"
  end
  task :libxml2 do
    FileUtils.rm_rf("ext/nokolexbor/build")
    FileUtils.rm_f("ext/nokolexbor/config.h")
    FileUtils.rm_f("ext/nokolexbor/libxml/xmlversion.h")
  end
end

Rake::Task[:clean].prerequisites << "clean:lexbor" << "clean:libxml2"