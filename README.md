# Nokolexbor

[![CI](https://github.com/serpapi/nokolexbor/actions/workflows/ci.yml/badge.svg)](https://github.com/serpapi/nokolexbor/actions/workflows/ci.yml)

Nokolexbor is a drop-in replacement for Nokogiri. It's 5.2x faster at parsing HTML and up to 997x faster at CSS selectors.

It's a performance-focused HTML5 parser for Ruby based on [Lexbor](https://github.com/lexbor/lexbor/). It supports both CSS selectors and XPath. Nokolexbor's API is designed to be 1:1 compatible as much as possible with [Nokogiri's API](https://github.com/sparklemotion/nokogiri).

## Requirements

Nokolexbor is shipped with pre-compiled gems on most common platforms:
* Linux: `x86_64`, with glibc >= 2.17
* macOS: `x86_64` and `arm64`
* Windows: `ucrt64`, `mingw32` and `mingw64`

If you are on a supported platform, just jump to the [Installation](#installation) section. Otherwise, you need to install CMake to compile C extensions:

### macOS

```
brew install cmake
```

### Linux (Debian, Ubuntu, etc.)

```
sudo apt-get install cmake
```

## Installation

Add to your Gemfile:

```ruby
gem 'nokolexbor'
```

Then, run `bundle install`.

Or, install the gem directly:

```
gem install nokolexbor
```

## Quick start

```ruby
require 'nokolexbor'
require 'open-uri'

# Parse HTML document
doc = Nokolexbor::HTML(URI.open('https://github.com/serpapi/nokolexbor'))

# Search for nodes by css
doc.css('#readme h1', 'article h2', 'p[dir=auto]').each do |node|
  puts node.content
end

# Search for text nodes by css
doc.css('#readme p > ::text').each do |text|
  puts text.content
end

# Search for nodes by xpath
doc.xpath('//div[@id="readme"]//h1', '//article//h2').each do |node|
  puts node.content
end
```

## Features
* Nokogiri-compatible APIs.
* High performance HTML parsing, DOM manipulation and CSS selectors engine.
* XPath search engine (ported from libxml2).
* Text nodes CSS selector support: `::text`.

## Searching methods overview
* `css` and `at_css`
  * Based on Lexbor.
  * Only accepts CSS selectors, doesn't support mixed syntax like `div#abc /text()`.
  * To select text nodes, use pseudo element `::text`. e.g. `div#abc > ::text`.
  * Performance is much higher than libxml2 based methods.
* `xpath` and `at_xpath`
  * Based on libxml2.
  * Only accepts XPath syntax.
  * Works in the same way as Nokogiri's `xpath` and `at_xpath`.
* `nokogiri_css` and `nokogiri_at_css` (requires Nokogiri installed)
  * Based on libxml2.
  * Accept mixed syntax like `div#abc /text()`.
  * Works in the same way as Nokogiri's `css` and `at_css`.

## Different behaviors from Nokogiri
* For selector `:nth-of-type(n)`, `n` is not affected by prior filter. For example, if we want to select the 3rd `div` excluding class `a` and class `b`, which will be the last `div` in the following HTML:
  ```
  <body>
    <div></div>
    <div class="a"></div>
    <div class="b"></div>
    <div></div>
    <div></div>
  </body>
  ```
  In Nokogiri, the selector should be `div:not(.a):not(.b):nth-of-type(3)`

  In Nokolexbor, `:not` does affect the place of the last `div` (same in browsers), the selector should be `div:not(.a):not(.b):nth-of-type(5)`, but this losts the purpose of filtering though.

## Benchmarks

Benchmark parsing google result page (368 KB) and selecting nodes using CSS and XPath. Run on MacBook Pro (2019) 2.3 GHz 8-Core Intel Core i9.

Run with: `ruby bench/bench.rb`

|            | Nokolexbor (iters/s) | Nokogiri (iters/s) | Diff |
| ---------- | ------------- | ----------- | -------------- |
| parsing    | 487.6         | 93.5        | 5.22x faster   |
| at_css     | 50798.8       | 50.9        | 997.87x faster |
| css        | 7437.6        | 52.3        | 142.11x faster |
| at_xpath   | 57.077        | 53.176      | same-ish       |
| xpath      | 51.523        | 58.438      | same-ish       |

<details>
<summary>Raw data</summary>

```
Warming up --------------------------------------
    Nokolexbor parse    56.000  i/100ms
      Nokogiri parse     8.000  i/100ms
Calculating -------------------------------------
    Nokolexbor parse    487.564  (±10.9%) i/s -      9.688k in  20.117173s
      Nokogiri parse     93.470  (±21.4%) i/s -      1.736k in  20.024163s

Comparison:
    Nokolexbor parse:      487.6 i/s
      Nokogiri parse:       93.5 i/s - 5.22x  (± 0.00) slower

Warming up --------------------------------------
   Nokolexbor at_css     5.548k i/100ms
     Nokogiri at_css     6.000  i/100ms
Calculating -------------------------------------
   Nokolexbor at_css     50.799k (±13.8%) i/s -    987.544k in  20.018481s
     Nokogiri at_css     50.907  (±35.4%) i/s -    828.000  in  20.666258s

Comparison:
   Nokolexbor at_css:    50798.8 i/s
     Nokogiri at_css:       50.9 i/s - 997.87x  (± 0.00) slower

Warming up --------------------------------------
      Nokolexbor css   709.000  i/100ms
        Nokogiri css     4.000  i/100ms
Calculating -------------------------------------
      Nokolexbor css      7.438k (±14.7%) i/s -    145.345k in  20.083833s
        Nokogiri css     52.338  (±36.3%) i/s -    816.000  in  20.042053s

Comparison:
      Nokolexbor css:     7437.6 i/s
        Nokogiri css:       52.3 i/s - 142.11x  (± 0.00) slower

Warming up --------------------------------------
 Nokolexbor at_xpath     2.000  i/100ms
   Nokogiri at_xpath     4.000  i/100ms
Calculating -------------------------------------
 Nokolexbor at_xpath     57.077  (±31.5%) i/s -    920.000  in  20.156393s
   Nokogiri at_xpath     53.176  (±35.7%) i/s -    876.000  in  20.036717s

Comparison:
 Nokolexbor at_xpath:       57.1 i/s
   Nokogiri at_xpath:       53.2 i/s - same-ish: difference falls within error

Warming up --------------------------------------
    Nokolexbor xpath     3.000  i/100ms
      Nokogiri xpath     3.000  i/100ms
Calculating -------------------------------------
    Nokolexbor xpath     51.523  (±31.1%) i/s -    903.000  in  20.102568s
      Nokogiri xpath     58.438  (±35.9%) i/s -    852.000  in  20.001408s

Comparison:
      Nokogiri xpath:       58.4 i/s
    Nokolexbor xpath:       51.5 i/s - same-ish: difference falls within error
```
</details>
