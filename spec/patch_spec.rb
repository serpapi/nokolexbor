require 'spec_helper'

describe "Patches to lexbor" do
  describe "::text selector" do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <div>
          Text 1
          <a class='a'>
            Text 2
          </a>
          Text 3
          Text 4
        </div>
      HTML
    end

    it "selects descendants" do
      _(@doc.css("div ::text").to_html.squish).must_equal "Text 1 Text 2 Text 3 Text 4"
    end

    it "selects children" do
      _(@doc.css("div > ::text").to_html.squish).must_equal "Text 1 Text 3 Text 4"
    end

    it "selects siblings" do
      _(@doc.css(".a ~ ::text").to_html.squish).must_equal "Text 3 Text 4"
    end
  end

  describe "case" do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <DiV id='Id'>
          <a class='kLs'>
          </a>
        </DiV>
      HTML
    end

    it "sensitive on id" do
      _(@doc.at_css("#Id")).wont_be_nil
      _(@doc.at_css("#id")).must_be_nil
    end

    it "sensitive on class name" do
      _(@doc.at_css(".kLs")).wont_be_nil
      _(@doc.at_css(".kls")).must_be_nil
    end

    it "insensitive on tag name" do
      _(@doc.at_css("DiV")).wont_be_nil
      _(@doc.at_css("div")).wont_be_nil
    end
  end

  describe "search respects <template> contents" do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <div class='a'>
          <div class='b'>
          </div>
          <template>
            <div class='c'>
            </div>
            <div class='d'>
              <div class='e'>
              </div>
            </div>
          </template>
        </DiV>
      HTML
    end

    it "with css" do
      _(@doc.css('div').size).must_equal 5
      _(@doc.css('div').map {|n| n['class']}).must_equal %w{a b c d e}
    end

    it "with xpath" do
      _(@doc.xpath('//div').size).must_equal 5
      _(@doc.xpath('//div').map {|n| n['class']}).must_equal %w{a b c d e}
    end

    it 'doc can be serialized without error' do
      _(@doc.to_html).must_be_instance_of String
    end
  end

  describe "clone element including <template>" do
    before do
      doc = Nokolexbor::HTML('<div class="a">123<template><span class="a">456</span><a href="b">789</a></template></div>')
      @node_with_template = doc.at_css('div')
      @the_template = doc.at_css('template')
    end

    it 'serialization works' do
      _(@node_with_template.to_html).must_equal @node_with_template.clone.to_html
      _(@the_template.to_html).must_equal @the_template.clone.to_html
    end

    it 'is not a shallow clone' do
      cloned_node = @node_with_template.clone
      cloned_node.at_css('span')['class'] = 'c'
      cloned_node.at_css('a').inner_html = '0000'
      _(@node_with_template.to_html).must_equal '<div class="a">123<template><span class="a">456</span><a href="b">789</a></template></div>'
      _(cloned_node.to_html).must_equal '<div class="a">123<template><span class="c">456</span><a href="b">0000</a></template></div>'
    end

    it 'cloned template should have only one child' do
      _(@the_template.children.size).must_equal 1
      _(@the_template.clone.children.size).must_equal 1
    end
  end

  describe "source_location" do
    before do
      @doc = Nokolexbor::HTML('<body><!--comment--><div class="a">123</div></body>')
      @ele = @doc.at_css('div')
      @text = @doc.at_css('::text')
      @comment = @doc.at_css('body').child
      @attr = @doc.at_css('div').attribute('class')
    end

    it 'element' do
      _(@ele.source_location).must_equal 21
    end

    it 'text node' do
      _(@text.source_location).must_equal 35
    end

    it 'comment' do
      _(@comment.source_location).must_equal 10
    end

    it 'attribute' do
      _(@attr.source_location).must_equal 25
    end

    describe 'their clones' do
      it 'element' do
        cloned_ele = @ele.clone
        _(cloned_ele.source_location).must_equal 21
        _(cloned_ele.attribute('class').source_location).must_equal 25
        _(cloned_ele.at_css('::text').source_location).must_equal 35
      end

      it 'text node' do
        _(@text.clone.source_location).must_equal 35
      end

      it 'comment' do
        _(@comment.clone.source_location).must_equal 10
      end

      it 'attribute' do
        _(@attr.clone.source_location).must_equal 25
      end
    end
  end

  describe "sibling combinators inside pseudo-class functions" do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <div>
          <div class="newscard position1"></div>
          <div class="newscard position2"></div>
          <div class="more-news"></div>
          <div class="newscard position3"></div>
          <div class="newscard position4"></div>
        </div>
      HTML
    end

    it "~ inside :not() excludes following siblings" do
      nodes = @doc.css(".newscard:not(.more-news ~ .newscard)")
      _(nodes.size).must_equal 2
      _(nodes[0]['class']).must_include 'position1'
      _(nodes[1]['class']).must_include 'position2'
    end

    it "+ inside :not() excludes adjacent sibling" do
      nodes = @doc.css(".newscard:not(.more-news + .newscard)")
      _(nodes.size).must_equal 3
      _(nodes.map { |n| n['class'] }).wont_include 'newscard position3'
    end

    it "~ inside :is() matches following siblings" do
      nodes = @doc.css(":is(.more-news ~ .newscard)")
      _(nodes.size).must_equal 2
      _(nodes[0]['class']).must_include 'position3'
      _(nodes[1]['class']).must_include 'position4'
    end

    it "> inside :not() checks parent correctly" do
      doc = Nokolexbor::HTML('<div id="a"><div id="b"><span></span></div></div>')
      # span IS a direct child of #b
      _(doc.css("span:not(#b > span)").size).must_equal 0
      # span is NOT a direct child of #a
      _(doc.css("span:not(#a > span)").size).must_equal 1
    end

    it "descendant inside :not() checks ancestors correctly" do
      doc = Nokolexbor::HTML('<div id="a"><div id="b"><span></span></div></div>')
      # span IS a descendant of #a
      _(doc.css("span:not(#a span)").size).must_equal 0
    end

    describe "backtracking" do
      it "retries farther ancestor when nearest candidate fails (.a > .b .c)" do
        # Two .b ancestors: only the outer one is a child of .a.
        doc = Nokolexbor::HTML('<div class="a"><div class="b"><div class="x"><div class="b"><span class="c"></span></div></div></div></div>')
        _(doc.css(":is(.a > .b .c)").size).must_equal 1
        _(doc.css("span:not(.a > .b .c)").size).must_equal 0
      end

      it "retries farther preceding sibling when nearest candidate fails (.a + .b ~ .c)" do
        # Two .b siblings before .c: only the first is adjacent to .a.
        doc = Nokolexbor::HTML('<div><i class="a"></i><i class="b"></i><i class="x"></i><i class="b"></i><i class="c"></i></div>')
        _(doc.css(":is(.a + .b ~ .c)").size).must_equal 1
        _(doc.css(".c:not(.a + .b ~ .c)").size).must_equal 0
      end

      it "does not match when no candidate satisfies the full chain" do
        doc = Nokolexbor::HTML('<div class="z"><div class="b"><div class="x"><div class="b"><span class="c"></span></div></div></div></div>')
        _(doc.css(":is(.a > .b .c)").size).must_equal 0
        _(doc.css("span:not(.a > .b .c)").size).must_equal 1
      end

      it "works with nested pseudo-class functions" do
        doc = Nokolexbor::HTML('<div><div class="newscard"></div><div class="more-news"></div><div class="newscard"></div></div>')
        _(doc.css(".newscard:not(:is(.more-news ~ .newscard))").size).must_equal 1
      end
    end
  end
end
