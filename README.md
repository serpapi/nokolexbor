# Nokolexbor

[![CI](https://github.com/serpapi/nokolexbor/actions/workflows/ci.yml/badge.svg)](https://github.com/serpapi/nokolexbor/actions/workflows/ci.yml)

Nokolexbor is a drop-in replacement for Nokogiri. It's 5x faster at parsing HTML and up to 2393x faster at CSS selectors.

It's a performance-focused HTML5 parser for Ruby based on [Lexbor](https://github.com/lexbor/lexbor/). It supports both CSS selectors and XPath. Nokolexbor's API is designed to be 1:1 compatible as much as possible with [Nokogiri's API](https://github.com/sparklemotion/nokogiri).

## Requirements

Nokolexbor is shipped with pre-compiled gems on most common platforms:
* Linux: `x86_64` and `aarch64`
* macOS: `x86_64` and `arm64`
* Windows: `ucrt64`

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

CPU: Apple M1 Pro (macOS). Run with: `ruby bench/bench.rb`

### Ruby 3.4.6

|              | Nokolexbor (iters/s) | Nokogiri (iters/s) | Diff |
| ------------ | ------------- | ------------ | --------------- |
| parsing      | 1422.2        | 321.5        | 4.42x faster    |
| at_css       | 294440.7      | 141.0        | 2088.84x faster |
| css          | 14505.5       | 140.9        | 102.93x faster  |
| at_xpath     | 149.9         | 146.4        | same-ish        |
| xpath        | 149.7         | 146.2        | same-ish        |
| inner_html=  | 281967.9      | 49365.2      | 5.71x faster    |

<details>
<summary>Raw data</summary>

```
ruby 3.4.6 (2025-09-16 revision dbd83256b1) +PRISM [arm64-darwin24]
Warming up --------------------------------------
Nokolexbor parse (367 KB)
                       146.000 i/100ms
Nokogiri parse (367 KB)
                        32.000 i/100ms
Calculating -------------------------------------
Nokolexbor parse (367 KB)
                          1.422k (± 1.5%) i/s  (703.16 μs/i) -     28.470k in  20.023416s
Nokogiri parse (367 KB)
                        321.460 (± 5.3%) i/s    (3.11 ms/i) -      6.432k in  20.065421s

Comparison:
Nokolexbor parse (367 KB):     1422.2 i/s
Nokogiri parse (367 KB):      321.5 i/s - 4.42x  slower

Warming up --------------------------------------
   Nokolexbor at_css    29.584k i/100ms
     Nokogiri at_css    13.000 i/100ms
Calculating -------------------------------------
   Nokolexbor at_css    294.441k (± 0.8%) i/s    (3.40 μs/i) -      5.917M in  20.096388s
     Nokogiri at_css    140.959 (± 1.4%) i/s    (7.09 ms/i) -      2.821k in  20.016614s

Comparison:
   Nokolexbor at_css:   294440.7 i/s
     Nokogiri at_css:      141.0 i/s - 2088.84x  slower

Warming up --------------------------------------
      Nokolexbor css     1.455k i/100ms
        Nokogiri css    14.000 i/100ms
Calculating -------------------------------------
      Nokolexbor css     14.506k (± 1.1%) i/s   (68.94 μs/i) -    291.000k in  20.063744s
        Nokogiri css    140.921 (± 1.4%) i/s    (7.10 ms/i) -      2.828k in  20.073048s

Comparison:
      Nokolexbor css:    14505.5 i/s
        Nokogiri css:      140.9 i/s - 102.93x  slower

Warming up --------------------------------------
 Nokolexbor at_xpath    15.000 i/100ms
   Nokogiri at_xpath    14.000 i/100ms
Calculating -------------------------------------
 Nokolexbor at_xpath    149.877 (± 2.7%) i/s    (6.67 ms/i) -      3.000k in  20.029730s
   Nokogiri at_xpath    146.385 (± 1.4%) i/s    (6.83 ms/i) -      2.940k in  20.088223s

Comparison:
 Nokolexbor at_xpath:      149.9 i/s
   Nokogiri at_xpath:      146.4 i/s - same-ish: difference falls within error

Warming up --------------------------------------
    Nokolexbor xpath    14.000 i/100ms
      Nokogiri xpath    14.000 i/100ms
Calculating -------------------------------------
    Nokolexbor xpath    149.654 (± 2.7%) i/s    (6.68 ms/i) -      2.996k in  20.032150s
      Nokogiri xpath    146.179 (± 1.4%) i/s    (6.84 ms/i) -      2.926k in  20.020594s

Comparison:
    Nokolexbor xpath:      149.7 i/s
      Nokogiri xpath:      146.2 i/s - same-ish: difference falls within error

Warming up --------------------------------------
Nokolexbor inner_html=
                        43.045k i/100ms
Nokogiri inner_html=     5.335k i/100ms
Calculating -------------------------------------
Nokolexbor inner_html=
                        281.968k (±16.8%) i/s    (3.55 μs/i) -      5.510M in  20.031263s
Nokogiri inner_html=     49.365k (±46.1%) i/s   (20.26 μs/i) -    725.560k in  20.192729s

Comparison:
Nokolexbor inner_html=:   281967.9 i/s
Nokogiri inner_html=:    49365.2 i/s - 5.71x  slower
```
</details>

### Ruby 2.7.8

|              | Nokolexbor (iters/s) | Nokogiri (iters/s) | Diff |
| ------------ | ------------- | ------------ | --------------- |
| parsing      | 1609.2        | 321.3        | 5.01x faster    |
| at_css       | 329144.0      | 137.5        | 2393.44x faster |
| css          | 15701.1       | 136.3        | 115.21x faster  |
| at_xpath     | 149.1         | 142.1        | same-ish        |
| xpath        | 149.1         | 142.8        | same-ish        |
| inner_html=  | 427517.6      | 39887.9      | 10.72x faster   |

<details>
<summary>Raw data</summary>

```
ruby 2.7.8p225 (2023-03-30 revision 1f4d455848) [arm64-darwin24]
Warming up --------------------------------------
Nokolexbor parse (367 KB)
                       162.000 i/100ms
Nokogiri parse (367 KB)
                        32.000 i/100ms
Calculating -------------------------------------
Nokolexbor parse (367 KB)
                          1.609k (± 1.5%) i/s  (621.43 μs/i) -     32.238k in  20.038379s
Nokogiri parse (367 KB)
                        321.329 (± 8.7%) i/s    (3.11 ms/i) -      6.400k in  20.090350s

Comparison:
Nokolexbor parse (367 KB):     1609.2 i/s
Nokogiri parse (367 KB):      321.3 i/s - 5.01x  slower

Warming up --------------------------------------
   Nokolexbor at_css    32.872k i/100ms
     Nokogiri at_css    13.000 i/100ms
Calculating -------------------------------------
   Nokolexbor at_css    329.144k (± 0.6%) i/s    (3.04 μs/i) -      6.607M in  20.074781s
     Nokogiri at_css    137.519 (± 0.7%) i/s    (7.27 ms/i) -      2.756k in  20.043157s

Comparison:
   Nokolexbor at_css:   329144.0 i/s
     Nokogiri at_css:      137.5 i/s - 2393.44x  slower

Warming up --------------------------------------
      Nokolexbor css     1.571k i/100ms
        Nokogiri css    13.000 i/100ms
Calculating -------------------------------------
      Nokolexbor css     15.701k (± 1.0%) i/s   (63.69 μs/i) -    314.200k in  20.013204s
        Nokogiri css    136.285 (± 2.2%) i/s    (7.34 ms/i) -      2.730k in  20.039433s

Comparison:
      Nokolexbor css:    15701.1 i/s
        Nokogiri css:      136.3 i/s - 115.21x  slower

Warming up --------------------------------------
 Nokolexbor at_xpath    14.000 i/100ms
   Nokogiri at_xpath    14.000 i/100ms
Calculating -------------------------------------
 Nokolexbor at_xpath    149.054 (± 2.7%) i/s    (6.71 ms/i) -      2.982k in  20.020034s
   Nokogiri at_xpath    142.148 (± 1.4%) i/s    (7.03 ms/i) -      2.856k in  20.097134s

Comparison:
 Nokolexbor at_xpath:      149.1 i/s
   Nokogiri at_xpath:      142.1 i/s - 1.05x  slower

Warming up --------------------------------------
    Nokolexbor xpath    14.000 i/100ms
      Nokogiri xpath    14.000 i/100ms
Calculating -------------------------------------
    Nokolexbor xpath    149.121 (± 2.0%) i/s    (6.71 ms/i) -      2.982k in  20.007593s
      Nokogiri xpath    142.823 (± 2.1%) i/s    (7.00 ms/i) -      2.856k in  20.005681s

Comparison:
    Nokolexbor xpath:      149.1 i/s
      Nokogiri xpath:      142.8 i/s - 1.04x  slower

Warming up --------------------------------------
Nokolexbor inner_html=
                        53.261k i/100ms
Nokogiri inner_html=     5.338k i/100ms
Calculating -------------------------------------
Nokolexbor inner_html=
                        427.518k (±13.9%) i/s    (2.34 μs/i) -      8.415M in  20.161527s
Nokogiri inner_html=     39.888k (±33.0%) i/s   (25.07 μs/i) -    688.602k in  20.070862s

Comparison:
Nokolexbor inner_html=:   427517.6 i/s
Nokogiri inner_html=:    39887.9 i/s - 10.72x  slower
```
</details>
