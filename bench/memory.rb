require "bundler/inline"

current_dir = File.dirname(__FILE__)

gemfile do
  source "https://rubygems.org"
  gem "nokolexbor", path: File.join(current_dir, "..")
end

require 'nokolexbor'

def get_resident_memory
  status_file = "/proc/self/status"
  rss_line = File.readlines(status_file).find { |line| line.start_with?("VmRSS:") }
  rss_kb = rss_line.split[1].to_i # RSS value in kilobytes
  rss_kb
end

html_file = File.join(current_dir, 'coffee.html')
html = File.open(html_file, "r:UTF-8", &:read)

css_selector = 'div.g div[data-ved] a[href]:not([href="#"])'

iter = 0

1000.times do
  50.times.map do
    Thread.new do
      nokolex = Nokolexbor::HTML(html)

      100.times do
        nokolex.at_css(css_selector)
        nokolex.css(css_selector)
      end
    end
  end.each(&:join)

  iter += 1
  puts "##{"%04d" % iter}: RSS: #{get_resident_memory / 1024.0} MB"
end
