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
end