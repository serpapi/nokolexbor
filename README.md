# Nokolexbor

[![CI](https://github.com/serpapi/nokolexbor/actions/workflows/ci.yml/badge.svg)](https://github.com/serpapi/nokolexbor/actions/workflows/ci.yml)

Nokolexbor is a drop-in replacement for Nokogiri. It's 4.7x faster at parsing HTML and up to 1352x faster at CSS selectors.

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

Benchmarks of parsing Google search result page (367 KB) and finding nodes using CSS selectors and XPath.

CPU: AMD Ryzen 5 5600 (Ubuntu 20.04 on Windows 10 WSL 2).

Run with: `ruby bench/bench.rb`

|            | Nokolexbor (iters/s) | Nokogiri (iters/s) | Diff |
| ---------- | ------------- | ------------ | --------------- |
| parsing    | 994.8         | 211.8        | 4.70x faster    |
| at_css     | 202963.7      | 150.1        | 1352.33x faster |
| css        | 9787.9        | 150.0        | 65.27x faster   |
| at_xpath   | 154.6         | 153.2        | same-ish        |
| xpath      | 154.3         | 153.2        | same-ish        |

<details>
<summary>Raw data</summary>

```
Warming up --------------------------------------
Nokolexbor parse (367 KB)
                       100.000  i/100ms
Nokogiri parse (367 KB)
                        20.000  i/100ms
Calculating -------------------------------------
Nokolexbor parse (367 KB)
                        994.773  (± 0.9%) i/s -     19.900k in  20.006124s
Nokogiri parse (367 KB)
                        211.793  (±12.3%) i/s -      4.180k in  20.093299s

Comparison:
Nokolexbor parse (367 KB):      994.8 i/s
Nokogiri parse (367 KB):      211.8 i/s - 4.70x  (± 0.00) slower

Warming up --------------------------------------
   Nokolexbor at_css    20.195k i/100ms
     Nokogiri at_css    15.000  i/100ms
Calculating -------------------------------------
   Nokolexbor at_css    202.964k (± 0.7%) i/s -      4.059M in  20.000626s
     Nokogiri at_css    150.084  (± 0.7%) i/s -      3.015k in  20.089207s

Comparison:
   Nokolexbor at_css:   202963.7 i/s
     Nokogiri at_css:      150.1 i/s - 1352.33x  (± 0.00) slower

Warming up --------------------------------------
      Nokolexbor css   977.000  i/100ms
        Nokogiri css    15.000  i/100ms
Calculating -------------------------------------
      Nokolexbor css      9.788k (± 0.4%) i/s -    196.377k in  20.063658s
        Nokogiri css    149.956  (± 0.7%) i/s -      3.000k in  20.006363s

Comparison:
      Nokolexbor css:     9787.9 i/s
        Nokogiri css:      150.0 i/s - 65.27x  (± 0.00) slower

Warming up --------------------------------------
 Nokolexbor at_xpath    15.000  i/100ms
   Nokogiri at_xpath    15.000  i/100ms
Calculating -------------------------------------
 Nokolexbor at_xpath    153.190  (± 0.7%) i/s -      3.075k in  20.073628s
   Nokogiri at_xpath    154.588  (± 0.6%) i/s -      3.105k in  20.086664s

Comparison:
   Nokogiri at_xpath:      154.6 i/s
 Nokolexbor at_xpath:      153.2 i/s - same-ish: difference falls within error

Warming up --------------------------------------
    Nokolexbor xpath    15.000  i/100ms
      Nokogiri xpath    15.000  i/100ms
Calculating -------------------------------------
    Nokolexbor xpath    153.159  (± 0.7%) i/s -      3.075k in  20.077580s
      Nokogiri xpath    154.322  (± 1.3%) i/s -      3.090k in  20.026288s

Comparison:
      Nokogiri xpath:      154.3 i/s
    Nokolexbor xpath:      153.2 i/s - same-ish: difference falls within error
```
</details>
