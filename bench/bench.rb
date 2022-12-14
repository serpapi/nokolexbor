require "bundler/inline"

current_dir = File.dirname(__FILE__)

gemfile do
  source "https://rubygems.org"
  gem "nokolexbor", path: File.join(current_dir, "..")
  gem "nokogiri"
  gem "benchmark-ips"
end

require 'benchmark/ips'
require 'nokolexbor'
require 'nokogiri'

html_file = File.join(current_dir, 'coffee.html')
html = File.open(html_file, "r:UTF-8", &:read)

Benchmark.ips do |x|
  x.warmup = 5
  x.time = 20
    
  x.report("Nokolexbor parse") do
    Nokolexbor::HTML(html)
  end
  x.report("Nokogiri parse") do
    Nokogiri::HTML(html)
  end
  x.compare!
end

nokolex = Nokolexbor::HTML(html)
nokogiri = Nokogiri::HTML(html)
css_selector = 'div.g div[data-ved] a[href]:not([href="#"])'
xpath_selector = '//div[@class="g"]//div[@data-ved]//a[@href]'

Benchmark.ips do |x|
  x.warmup = 5
  x.time = 20

  x.report("Nokolexbor at_css") do
    nokolex.at_css(css_selector)
  end
  x.report("Nokogiri at_css") do
    nokogiri.at_css(css_selector)
  end
  x.compare!
end

Benchmark.ips do |x|
  x.warmup = 5
  x.time = 20

  x.report("Nokolexbor css") do
    nokolex.css(css_selector)
  end
  x.report("Nokogiri css") do
    nokogiri.css(css_selector)
  end
  x.compare!
end

Benchmark.ips do |x|
  x.warmup = 5
  x.time = 20

  x.report("Nokolexbor at_xpath") do
    nokolex.at_xpath(xpath_selector)
  end
  x.report("Nokogiri at_xpath") do
    nokogiri.at_xpath(xpath_selector)
  end
  x.compare!
end

Benchmark.ips do |x|
  x.warmup = 5
  x.time = 20

  x.report("Nokolexbor xpath") do
    nokolex.xpath(xpath_selector)
  end
  x.report("Nokogiri xpath") do
    nokogiri.xpath(xpath_selector)
  end
  x.compare!
end
