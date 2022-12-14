# Nokolexbor

A high performance HTML5 parser for Ruby based on [Lexbor](https://github.com/lexbor/lexbor/), with support for both CSS selectors and XPath. It's API is designed to be compatible with [Nokogiri](https://github.com/sparklemotion/nokogiri).

## Installation

Nokolexbor contains C extensions and requires `cmake` to compile the source, check it before installing the gem.

```
gem install nokolexbor
```

## Quick start

```ruby
require 'nokolexbor'

html = <<-HTML
<html>
  <body>
    <ul class='menu'>
      <li><a href='http://example1.com'>Example 1</a></li>
      <li><a href='http://example2.com'>Example 2</a></li>
      <li><a href='http://example3.com'>Example 3</a></li>
    </ul>
    <div>
      <article>
        <h1>Title</h1>
        Text content 1
        <h2>Sub title</h2>
        Text content 2
      </article>
    </div>
  </body>
</html>
HTML

# Parse HTML document
doc = Nokolexbor::HTML(html)

# Search for nodes by css
doc.css('ul.menu li a', 'article h2').each do |link|
  puts link.content
end

# Search for text nodes by css
doc.css('article > ::text').each do |text|
  puts text.content
end

# Search for nodes by xpath
doc.xpath('//ul//li/a', '//article//h2').each do |link|
  puts link.content
end
```


## Features
* A subset of Nokogiri compatible API.
* High performance HTML parsing, DOM manipulation and CSS selectors engine.
* XPath search engine (the algorithm is ported from libxml2).
* Selecting text nodes with CSS selectors using `::text`.

## Limitations
* Mixed expression of CSS selectors and XPath is not supported in Nokolexbor. Selectors like `div > a[last()]` won't work, use `div > a:last-of-type` instead.

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