# frozen_string_literal: true

require 'rake/extensiontask'

def native_gemspec
  eval(File.read 'nokolexbor.gemspec').tap do |spec|
    spec.extensions = []
    spec.files = Dir.glob("lib/**/*.rb")
  end
end

Rake::ExtensionTask.new('nokolexbor', native_gemspec) do |ext|
  ext.lib_dir = "lib/nokolexbor"
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