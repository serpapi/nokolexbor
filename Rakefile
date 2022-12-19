require 'rake/extensiontask'
require 'rake/testtask'

Rake::ExtensionTask.new('nokolexbor') do |ext|
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

Rake::TestTask.new do |t|
  t.libs << 'spec'
  t.pattern = 'spec/**/*_spec.rb'
end

Rake::TestTask.new('test-gem') do |t|
  t.libs << 'spec'
  t.libs.delete('lib')
  t.pattern = 'spec/**/*_spec.rb'
end

task :default => [:compile, :test]
