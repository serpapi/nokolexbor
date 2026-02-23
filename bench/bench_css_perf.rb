#!/usr/bin/env ruby
# frozen_string_literal: true
#
# Focused benchmark for css/at_css performance pre- and post-fix.
# Uses only stdlib Benchmark — no extra gems required.
#
# Run:
#   bundle exec ruby bench/bench_css_perf.rb
#
require 'benchmark'
require 'nokolexbor'

current_dir = File.dirname(__FILE__)
html = File.read(File.join(current_dir, 'coffee.html'), encoding: 'UTF-8')

DOC        = Nokolexbor::HTML(html)
SELECTORS  = [
  'div',
  'a[href]',
  'div.g div[data-ved] a[href]:not([href="#"])',
  'h1, h2, h3',
  'span:nth-child(2)',
  '#search',
].freeze

ITERATIONS = 5_000

puts "nokolexbor #{Nokolexbor::VERSION}  —  #{RUBY_DESCRIPTION}"
puts "HTML fixture: #{html.size / 1024} KB"
puts "Iterations per selector: #{ITERATIONS}"
puts

Benchmark.bm(55) do |x|
  SELECTORS.each do |sel|
    x.report("css    #{sel.length > 45 ? sel[0..44] + '…' : sel}") do
      ITERATIONS.times { DOC.css(sel) }
    end
  end

  SELECTORS.each do |sel|
    x.report("at_css #{sel.length > 45 ? sel[0..44] + '…' : sel}") do
      ITERATIONS.times { DOC.at_css(sel) }
    end
  end
end
