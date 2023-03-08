require 'spec_helper'

describe Nokolexbor::NodeSet do
  before do
    @doc = Nokolexbor::HTML <<-HTML
      <section>
        <div class='a'><span>A</span></div>
        <div class='b'>B</div>
        <div class='c'>C</div>
        <div class='d'>D</div>
        <div class='e'>E</div>
        <div class='f'>F</div>
        <h1><a></a></h1>
      </section>
    HTML
    @nodes = @doc.css('div')
  end

  describe 'css' do
    it 'searches direct children' do
      _(@nodes.css('.a').size).must_equal 1
      nodes2 = @nodes.css('div').css('div').css('div')
      _(@nodes.size).must_equal(nodes2.size)
      @nodes.each_with_index do |node, index|
        _(node).must_equal nodes2[index]
      end
    end

    it 'results are in document traversal order' do
      nodes = @doc.css('a, h1, div.a')
      _(nodes.size).must_equal 3
      _(nodes[0].name).must_equal 'div'
      _(nodes[0]['class']).must_equal 'a'
      _(nodes[1].name).must_equal 'h1'
      _(nodes[2].name).must_equal 'a'
    end
  end

  describe 'at_css' do
    it 'returns first match element' do
      node = @nodes.at_css('a, h1, div')
      _(node.name).must_equal 'div'
      _(node['class']).must_equal 'a'
    end

    it 'returns nil if not found' do
      node = @nodes.at_css('section')
      _(node).must_be_nil
    end
  end

  it 'length' do
    _(@nodes.length).must_equal 6
    _(@nodes.size).must_equal 6
  end

  describe 'slice' do
    it 'single index' do
      _(@nodes[0]['class']).must_equal 'a'
    end

    it 'single negative index' do
      _(@nodes[-1]['class']).must_equal 'f'
    end

    it 'range' do
      subset = @nodes[1..3]
      _(subset).must_be_instance_of Nokolexbor::NodeSet
      _(subset.size).must_equal 3
      _(subset[0]['class']).must_equal 'b'
    end
  end

  it 'to_a' do
    ary = @nodes.to_a
    _(ary).must_be_instance_of Array
    _(ary.size).must_equal @nodes.size
  end

  it 'delete' do
    @nodes.delete(@nodes[1])
    _(@nodes.map {|n| n['class']}.join).must_equal 'acdef'
  end

  it 'include?' do
    _(@nodes.include?(@nodes[-1])).must_equal true
    _(@nodes.include?(Nokolexbor::Node.new('div', @nodes[0].document))).must_equal false
  end

  it 'is enumerable' do
    _(@nodes.map {|n| n['class']}.join).must_equal 'abcdef'
  end

  it 'first' do
    _(@nodes.first['class']).must_equal 'a'
    _(@nodes.first(2).last['class']).must_equal 'b'
  end

  it 'last' do
    _(@nodes.last['class']).must_equal 'f'
  end

  it 'empty?' do
    _(@nodes.empty?).must_equal false
    _(Nokolexbor::NodeSet.new(@nodes.first.document).empty?).must_equal true
  end

  it 'index' do
    _(@nodes.index(@nodes[2])).must_equal 2
  end

  it 'content' do
    [:content, :text, :inner_text, :to_str].each do |method|
      _(@nodes.send(method)).must_equal 'ABCDEF'
    end
  end

  describe 'inner_html' do
    it 'with indent' do
      _(@nodes.inner_html(indent: 2)).must_equal <<-HTML
<span>
  "A"
</span>
"B"
"C"
"D"
"E"
"F"
HTML
    end

    it 'without indent' do
      _(@nodes.inner_html).must_equal '<span>A</span>BCDEF'
    end
  end

  describe 'outer_html' do
    it 'without indent' do
      [:outer_html, :to_s, :to_html, :serialize].each do |method|
        _(@nodes.send(method, indent: 2)).must_equal <<-HTML
<div class="a">
  <span>
    "A"
  </span>
</div>
<div class="b">
  "B"
</div>
<div class="c">
  "C"
</div>
<div class="d">
  "D"
</div>
<div class="e">
  "E"
</div>
<div class="f">
  "F"
</div>
HTML
      end
    end

    it 'without indent' do
      [:outer_html, :to_s, :to_html, :serialize].each do |method|
        _(@nodes.send(method)).must_equal '<div class="a"><span>A</span></div><div class="b">B</div><div class="c">C</div><div class="d">D</div><div class="e">E</div><div class="f">F</div>'
      end
    end
  end

  it 'remove' do
    @nodes.remove
    _(@doc.at_css('section').inner_html.squish).must_equal '<h1><a></a></h1>'
  end

  it 'pop' do
    @nodes.pop
    _(@nodes.size).must_equal 5
    _(@nodes.last['class']).must_equal 'e'
  end

  it 'shift' do
    @nodes.shift
    _(@nodes.size).must_equal 5
    _(@nodes.first['class']).must_equal 'b'
  end

  it 'equals' do
    other_node_set = Nokolexbor::NodeSet.new(@nodes.first.document)
    empty_node_set = Nokolexbor::NodeSet.new(@nodes.first.document)
    @nodes.each do |n|
      other_node_set << n
    end
    _(other_node_set).must_equal @nodes
    _(other_node_set).wont_equal empty_node_set
  end

  it 'children' do
    children = @nodes.children
    _(children.size).must_equal 6
    _(children[0].name).must_equal 'span'
    _(children[1].name).must_equal '#text'
    _(children[1].text).must_equal 'B'
  end

  it 'reverse' do
    reversed_nodes = @nodes.reverse
    _(reversed_nodes.size).must_equal 6
    _(reversed_nodes.first['class']).must_equal 'f'
  end

  describe 'wrap' do
    before do
      @doc = Nokolexbor::HTML('<span>123</span><span>456</span>')
      @nodes = @doc.css('span')
    end

    it 'with String' do
      @nodes.wrap('<div></div>')
      _(@doc.at_css('body').inner_html).must_equal '<div><span>123</span></div><div><span>456</span></div>'
    end

    it 'with fragment' do
      @nodes.wrap(@doc.create_element('div'))
      _(@doc.at_css('body').inner_html).must_equal '<div><span>123</span></div><div><span>456</span></div>'
    end
  end

  describe 'xpath' do
    it 'basic usage' do
      result = @nodes.xpath('.//text()')
      _(result.size).must_equal 6
      _(result[0].name).must_equal '#text'
      _(result[0].text).must_equal 'A'
      _(result[1].name).must_equal '#text'
      _(result[1].text).must_equal 'B'
    end
  end

  describe 'union' do
    before do
      @doc = Nokolexbor::HTML('<a><div class="a"></div><span class="a"></span><div class="b"></div></a>')
      @nodes1 = @doc.css("div")
      @nodes2 = @doc.css(".a")
    end

    it 'works' do
      _(@nodes1.size).must_equal 2
      _(@nodes2.size).must_equal 2
      [@nodes1 + @nodes2, @nodes1 | @nodes2].each do |new_nodes|
        _(new_nodes.size).must_equal 3
        _(new_nodes[0].to_html).must_equal '<div class="a"></div>'
        _(new_nodes[1].to_html).must_equal '<div class="b"></div>'
        _(new_nodes[2].to_html).must_equal '<span class="a"></span>'
      end
    end

    it 'when self is empty' do
      _(@nodes1 + Nokolexbor::NodeSet.new(@doc, [])).must_equal @nodes1
    end

    it 'when other is empty' do
      _(Nokolexbor::NodeSet.new(@doc, []) + @nodes2).must_equal @nodes2
    end

    it 'when both are empty' do
      new_nodes = Nokolexbor::NodeSet.new(@doc, []) + Nokolexbor::NodeSet.new(@doc, [])
      _(new_nodes.size).must_equal 0
    end

    it 'raises when other is not NodeSet' do
      _{ @nodes1 + 1 }.must_raise ArgumentError
    end
  end

  describe 'intersection' do
    before do
      @doc = Nokolexbor::HTML('<a><div class="a"></div><span class="a"></span><div class="b"></div></a>')
      @nodes1 = @doc.css("div")
      @nodes2 = @doc.css(".a")
    end

    it 'works' do
      _(@nodes1.size).must_equal 2
      _(@nodes2.size).must_equal 2
      new_nodes = @nodes1 & @nodes2
      _(new_nodes.size).must_equal 1
      _(new_nodes[0].to_html).must_equal '<div class="a"></div>'
    end

    it 'when self is empty' do
      new_node = @nodes1 & Nokolexbor::NodeSet.new(@doc, [])
      _(new_node.size).must_equal 0
    end

    it 'when other is empty' do
      new_node = Nokolexbor::NodeSet.new(@doc, []) & @nodes2
      _(new_node.size).must_equal 0
    end

    it 'when both are empty' do
      new_node = Nokolexbor::NodeSet.new(@doc, []) & Nokolexbor::NodeSet.new(@doc, [])
      _(new_node.size).must_equal 0
    end

    it 'raises when other is not NodeSet' do
      _{ @nodes1 & 1 }.must_raise ArgumentError
    end
  end

  describe 'difference' do
    before do
      @doc = Nokolexbor::HTML('<a><div class="a"></div><span class="a"></span><div class="b"></div></a>')
      @nodes1 = @doc.css("div")
      @nodes2 = @doc.css(".a")
    end

    it 'works' do
      _(@nodes1.size).must_equal 2
      _(@nodes2.size).must_equal 2
      new_nodes = @nodes1 - @nodes2
      _(new_nodes.size).must_equal 1
      _(new_nodes[0].to_html).must_equal '<div class="b"></div>'
    end

    it 'when self is empty' do
      _(@nodes1 - Nokolexbor::NodeSet.new(@doc, [])).must_equal @nodes1
    end

    it 'when other is empty' do
      new_node = Nokolexbor::NodeSet.new(@doc, []) - @nodes2
      _(new_node.size).must_equal 0
    end

    it 'when both are empty' do
      new_node = Nokolexbor::NodeSet.new(@doc, []) - Nokolexbor::NodeSet.new(@doc, [])
      _(new_node.size).must_equal 0
    end

    it 'raises when other is not NodeSet' do
      _{ @nodes1 - 1 }.must_raise ArgumentError
    end
  end

  it 'add_class' do
    doc = Nokolexbor::HTML('<div><a></a><a class="b"></a></div>')
    doc.css('a').add_class('b c')
    _(doc.at_css('div').inner_html).must_equal '<a class="b c"></a><a class="b c"></a>'
  end

  it 'append_class' do
    doc = Nokolexbor::HTML('<div><a></a><a class="b"></a></div>')
    doc.css('a').append_class('b c')
    _(doc.at_css('div').inner_html).must_equal '<a class="b c"></a><a class="b b c"></a>'
  end

  it 'remove_class' do
    doc = Nokolexbor::HTML('<div><a class="d"></a><a class="b"></a></div>')
    doc.css('a').remove_class('b c')
    _(doc.at_css('div').inner_html).must_equal '<a class="d"></a><a></a>'
  end

  it 'remove_attr' do
    doc = Nokolexbor::HTML('<div><a class="a" href="a"></a><a class="b"></a></div>')
    doc.css('a').remove_attr('class')
    _(doc.at_css('div').inner_html).must_equal '<a href="a"></a><a></a>'
  end

  describe 'attr' do
    before do
      @doc = Nokolexbor::HTML('<div><a class="a" href="b"></a><a class="c" style="d"></a></div>')
      @nodes = @doc.css('a')
    end

    it 'as a getter' do
      att = @nodes.attr('class')
      _(att).must_be_instance_of Nokolexbor::Attribute
      _(att.value).must_equal 'a'
    end

    it 'as a setter, with a hash' do
      @nodes.attr("href" => "http://example.com", "class" => "b")
      _(@doc.at_css('div').inner_html).must_equal '<a class="b" href="http://example.com"></a><a class="b" style="d" href="http://example.com"></a>'
    end

    it 'as a setter, passing key and value' do
      @nodes.attr("href", "http://example.com")
      _(@doc.at_css('div').inner_html).must_equal '<a class="a" href="http://example.com"></a><a class="c" style="d" href="http://example.com"></a>'
    end

    it 'as a setter, passing block' do
      @nodes.attr("href") {|node| "http://example.com"}
      _(@doc.at_css('div').inner_html).must_equal '<a class="a" href="http://example.com"></a><a class="c" style="d" href="http://example.com"></a>'
    end
  end
end