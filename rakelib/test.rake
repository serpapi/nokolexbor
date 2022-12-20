# frozen_string_literal: true

require 'rake/testtask'

Rake::TestTask.new do |t|
  t.libs << 'spec'
  t.pattern = 'spec/**/*_spec.rb'
end

namespace :test do
  Rake::TestTask.new('gem') do |t|
    t.libs << 'spec'
    t.libs.delete('lib')
    t.pattern = 'spec/**/*_spec.rb'
  end
end
