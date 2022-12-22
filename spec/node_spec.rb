require 'spec_helper'

describe Nokolexbor::Node do
  it 'new' do
    doc = Nokolexbor::HTML('')
    node = Nokolexbor::Node.new('div', doc)
    _(node).must_be_instance_of Nokolexbor::Node
    _(node.name).must_equal 'div'
  end

  it 'content' do
    doc = Nokolexbor::HTML <<-HTML
      <div>
        <h1>Title</h1>
        <h2></h2>
        Text
      </div>
    HTML
    [:content, :text, :inner_text, :to_str].each do |method|
      _(doc.at_css('div').send(method).squish).must_equal "Title Text"
      _(doc.at_css('h2').send(method).squish).must_equal ''
    end
  end

  describe 'attr' do
    before do
      @doc = Nokolexbor::HTML('<div class="a" checked></div>')
      @node = @doc.at_css('div')
    end

    it 'has no effect on Nokolexbor::Document' do
      @doc['class'] = 'a'
      _(@doc['class']).must_be_nil
    end

    it 'get' do
      _(@node['class']).must_equal 'a'
      _(@node.attr('class')).must_equal 'a'
      _(@node['checked']).must_equal ''
    end

    it 'set' do
      _(@node['class'] = 'b').must_equal 'b'
      _(@node['class']).must_equal 'b'
      _(@node.set_attr('class', 'c')).must_equal 'c'
      _(@node['class']).must_equal 'c'
    end

    it 'delete' do
      _(@node.remove_attr('class')).must_equal true
      _(@node['class']).must_be_nil
      _(@node.delete('checked')).must_equal true
      _(@node['checked']).must_be_nil
    end
  end

  describe 'css' do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <div>
          <h1 class='top'>
            <a>
              <h1 class='inner'></h1>
            </a>
          </h1>
          <div class='a'>
          </div>
        </div>
      HTML
      @root = @doc.at_css('div')
    end

    it 'returns Nokolexbor::NodeSet' do
      _(@doc.css('h1')).must_be_instance_of Nokolexbor::NodeSet
    end

    it 'results are in document traversal order' do
      nodes = @root.css('div, h1, a')
      _(nodes.size).must_equal 4
      _(nodes[0].name).must_equal 'h1'
      _(nodes[0]['class']).must_equal 'top'
      _(nodes[1].name).must_equal 'a'
      _(nodes[2].name).must_equal 'h1'
      _(nodes[2]['class']).must_equal 'inner'
      _(nodes[3].name).must_equal 'div'
      _(nodes[3]['class']).must_equal 'a'
    end

    it 'supports top level relative selector' do
      nodes = @root.css('> h1')
      _(nodes.size).must_equal 1
      _(nodes[0].name).must_equal 'h1'
      _(nodes[0]['class']).must_equal 'top'
    end

    it 'nodes in results are unique' do
      nodes = @root.css('h1, .top, .inner')
      _(nodes.size).must_equal 2
      _(nodes[0].name).must_equal 'h1'
      _(nodes[0]['class']).must_equal 'top'
      _(nodes[1].name).must_equal 'h1'
      _(nodes[1]['class']).must_equal 'inner'
    end

    it 'raises if selector is invalid' do
      _{ @root.css('::text1') }.must_raise Nokolexbor::LexborError
    end
  end

  describe 'at_css' do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <div>
          <h1 class='top'>
            <a>
              <h1 class='inner'></h1>
            </a>
          </h1>
          <div class='a'>
          </div>
        </div>
      HTML
      @root = @doc.at_css('div')
    end

    it 'returns first match element' do
      node = @root.at_css('div, h1, a')
      _(node.name).must_equal 'h1'
      _(node['class']).must_equal 'top'
    end

    it 'returns nil if not found' do
      node = @root.at_css('section')
      _(node).must_be_nil
    end
  end

  it 'inner_html' do
    doc = Nokolexbor::HTML('<div><div class="a"></div></div>')
    _(doc.at_css('div').inner_html).must_equal '<div class="a"></div>'
  end

  it 'outer_html' do
    doc = Nokolexbor::HTML('<div><div class="a"></div></div>')
    [:outer_html, :to_html, :to_s].each do |method|
      _(doc.at_css('div').send(method)).must_equal '<div><div class="a"></div></div>'
    end
  end

  it 'key?' do
    doc = Nokolexbor::HTML('<div><div class="a"></div></div>')
    _(doc.at_css('div').key?('class')).must_equal false
    _(doc.at_css('div').at_css('div').key?('class')).must_equal true
  end

  describe 'attribute collection' do
    before do
      @doc = Nokolexbor::HTML('<div attr1 attr2="a" attr3="b"></div>')
    end

    it 'keys' do
      _(@doc.at_css('div').keys).must_equal %w{attr1 attr2 attr3}
    end

    it 'values' do
      _(@doc.at_css('div').values).must_equal ['', 'a', 'b']
    end

    it 'attrs' do
      _(@doc.at_css('div').attrs).must_equal({'attr1' => '', 'attr2' => 'a', 'attr3' => 'b'})
    end
  end

  it 'parent' do
    doc = Nokolexbor::HTML <<-HTML
      <html>
        <body>
          <div>
          </div>
        </body>
      </html>
    HTML
    node = doc.at_css('div')
    _(node.parent.name).must_equal 'body'
    _(node.parent.parent.name).must_equal 'html'
    _(node.parent.parent.parent.name).must_equal '#document'
    _(node.parent.parent.parent.parent).must_be_nil
  end

  describe "previous and next" do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <div><span class='a'></span>123<span class='b'></span></div>
      HTML
    end

    it 'previous' do
      node = @doc.at_css('.b')
      _(node.previous.name).must_equal '#text'
      _(node.previous.text).must_equal '123'
      _(node.previous.previous.name).must_equal 'span'
      _(node.previous.previous['class']).must_equal 'a'
      _(node.previous.previous.previous).must_be_nil
    end

    it 'previous_element' do
      node = @doc.at_css('.b')
      _(node.previous_element.name).must_equal 'span'
      _(node.previous_element['class']).must_equal 'a'
      _(node.previous_element.previous_element).must_be_nil
    end

    it 'next' do
      node = @doc.at_css('.a')
      _(node.next.name).must_equal '#text'
      _(node.next.text).must_equal '123'
      _(node.next.next.name).must_equal 'span'
      _(node.next.next['class']).must_equal 'b'
      _(node.next.next.next).must_be_nil
    end

    it 'next_element' do
      node = @doc.at_css('.a')
      _(node.next_element.name).must_equal 'span'
      _(node.next_element['class']).must_equal 'b'
      _(node.next_element.next_element).must_be_nil
    end
  end

  it 'children' do
    doc = Nokolexbor::HTML <<-HTML
      <div><span class='a'></span>123<span class='b'></span></div>
    HTML
    children = doc.at_css('div').children
    _(children.size).must_equal 3
    _(children[0]['class']).must_equal 'a'
    _(children[1].name).must_equal '#text'
    _(children[2]['class']).must_equal 'b'
  end

  it 'child' do
    doc = Nokolexbor::HTML <<-HTML
      <div><span class='a'></span>123<span class='b'></span></div>
    HTML
    _(doc.at_css('div').child['class']).must_equal 'a'
    _(doc.at_css('span').child).must_be_nil
  end

  it 'remove' do
    doc = Nokolexbor::HTML <<-HTML
      <div><span class='a'></span>123<span class='b'></span></div>
    HTML
    root = doc.at_css('div')
    root.at_css('span').remove
    _(root.inner_html).must_equal '123<span class="b"></span>'
    root.at_css('::text').unlink
    _(root.inner_html).must_equal '<span class="b"></span>'
  end

  it 'equals' do
    doc = Nokolexbor::HTML <<-HTML
      <div><span class='a'></span></div>
    HTML
    node1 = doc.at_css('span')
    node2 = doc.at_css('.a')
    _(node1).must_equal node2
  end

  it 'name' do
    doc = Nokolexbor::HTML <<-HTML
      <DiV></DiV>
    HTML
    _(doc.at_css('div').name).must_equal 'div'
  end

  describe 'fragment' do
    before do
      @doc = Nokolexbor::HTML('')
      @frag = @doc.fragment('<div></div><span></span>')
    end

    it 'returns Nokolexbor::Node' do
      _(@frag).must_be_instance_of Nokolexbor::Node
    end

    it 'to_html works' do
      _(@frag.children.to_html).must_equal '<div></div><span></span>'
    end

    it 'can be inserted to doc' do
      @doc.at_css('body').children = @frag.children
      _(@doc.at_css('body').inner_html).must_equal '<div></div><span></span>'
    end
  end

  describe 'add_sibling' do
    before do
      @doc = Nokolexbor::HTML('<div><a></a><span></span></div>')
      @root = @doc.at_css('div')
    end

    describe 'to next' do
      before do
        @node = @doc.at_css('span')
      end

      it 'with String' do
        @node.add_sibling(:next, '<span class="a"></span>')
        _(@root.inner_html).must_equal '<a></a><span></span><span class="a"></span>'
      end

      it 'with fragment' do
        @node.add_sibling(:next, @node.fragment('<span class="a"></span>').child)
        _(@root.inner_html).must_equal '<a></a><span></span><span class="a"></span>'
      end

      it 'with existing node' do
        @node.add_sibling(:next, @root.at_css('a'))
        _(@root.inner_html).must_equal '<span></span><a></a>'
      end
    end

    describe 'to previous' do
      before do
        @node = @doc.at_css('a')
      end

      it 'with String' do
        @node.add_sibling(:previous, '<span class="a"></span>')
        _(@root.inner_html).must_equal '<span class="a"></span><a></a><span></span>'
      end

      it 'with fragment' do
        @node.add_sibling(:previous, @node.fragment('<span class="a"></span>').child)
        _(@root.inner_html).must_equal '<span class="a"></span><a></a><span></span>'
      end

      it 'with existing node' do
        @node.add_sibling(:previous, @root.at_css('span'))
        _(@root.inner_html).must_equal '<span></span><a></a>'
      end
    end
  end

  describe 'add_child' do
    before do
      @doc = Nokolexbor::HTML('<div><span></span></div><a></a>')
      @node = @doc.at_css('div')
    end

    it 'with String' do
      @node.add_child('<span class="a"></span>')
      _(@node.inner_html).must_equal '<span></span><span class="a"></span>'
    end

    it 'with fragment' do
      @node.add_child(@node.fragment('<span class="a"></span>').child)
      _(@node.inner_html).must_equal '<span></span><span class="a"></span>'
    end

    it 'with existing node' do
      @node.add_child(@doc.at_css('a'))
      _(@doc.at_css('body').inner_html).must_equal '<div><span></span><a></a></div>'
    end
  end

  it 'type' do
    @doc = Nokolexbor::HTML('<div><span></span>123</a>')
    @node = @doc.at_css('div')
    _(@node.children[0].type).must_equal Nokolexbor::Node::ELEMENT_NODE
    _(@node.children[1].type).must_equal Nokolexbor::Node::TEXT_NODE
  end

  describe 'element_child' do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <div>
          123
          456
          <span>
          </span>
          <a>
          </a>
          789
          012
        </div>
      HTML
      @root = @doc.at_css('div')
    end

    it 'first_element_child' do
      _(@root.first_element_child.name).must_equal 'span'
    end

    it 'lat_element_child' do
      _(@root.last_element_child.name).must_equal 'a'
    end
  end

  describe 'clone' do
    before do
      @doc = Nokolexbor::HTML('<div class="a">123</div>')
      @node = @doc.at_css('div')
    end

    it 'clones correctly' do
      _(@node.clone.to_html).must_equal @node.to_html
    end

    it 'does not equal' do
      _(@node.clone).wont_equal @node
    end

    it 'can be added to doc' do
      @node.add_sibling(:next, @node.dup)
      _(@doc.at_css('body').inner_html).must_equal '<div class="a">123</div><div class="a">123</div>'
    end
  end

  describe 'ancestors' do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <html>
          <body>
            <div class='a'>
              <div>
                <div class='a'>
                  <span></span>
                </div>
              </div>
            </div>
          </body>
        </html>
      HTML
      @node = @doc.at_css('span')
    end

    it 'without selector' do
      _(@node.ancestors.size).must_equal 6
    end

    it 'with selector' do
      _(@node.ancestors('.a').size).must_equal 2
    end
  end

  describe 'wrap' do
    before do
      @doc = Nokolexbor::HTML('<span>123</span>')
      @node = @doc.at_css('span')
    end

    it 'with String' do
      @node.wrap('<div></div>')
      _(@doc.at_css('body').inner_html).must_equal '<div><span>123</span></div>'
    end

    it 'with fragment' do
      @node.wrap(@node.fragment('<div></div>').child)
      _(@doc.at_css('body').inner_html).must_equal '<div><span>123</span></div>'
    end
  end

  it 'matches?' do
    doc = Nokolexbor::HTML('<span class="a"><div>123<div></span>')
    node = doc.at_css('span')
    _(node.matches?('span.a:has(div)')).must_equal true
  end

  it 'attribute' do
    doc = Nokolexbor::HTML('<div attr1="1" attr2="2" attr3="3"></div>')
    a = doc.at_css('div').attribute('attr1')
    _(a).must_be_instance_of Nokolexbor::Attribute
    _(a.name).must_equal 'attr1'
    _(a.value).must_equal '1'
  end

  it 'attributes' do
    doc = Nokolexbor::HTML('<div attr1="1" attr2="2" attr3="3"></div>')
    attrs = doc.at_css('div').attributes
    _(attrs).must_be_instance_of Hash
    _(attrs.values.size).must_equal 3
  end

  describe 'replace' do
    before do
      @doc = Nokolexbor::HTML('<div><span></span></div>')
      @node = @doc.at_css('div')
      @parent = @node.parent
    end

    it 'with String' do
      @node.replace('<section class="a"></section>')
      _(@parent.inner_html).must_equal '<section class="a"></section>'
    end

    it 'with fragment' do
      @node.replace(@node.fragment('<section class="a"></section>').child)
      _(@parent.inner_html).must_equal '<section class="a"></section>'
    end

    it 'with NodeSet' do
      @node.replace(@node.fragment('<section class="a"></section><section class="b"></section').children)
      _(@parent.inner_html).must_equal '<section class="a"></section><section class="b"></section>'
    end
  end

  describe 'replace' do
    before do
      @doc = Nokolexbor::HTML('')
      @node = @doc.at_css('body')
    end

    it 'with String' do
      @node.children = '<section class="a"></section>'
      _(@node.inner_html).must_equal '<section class="a"></section>'
    end

    it 'with fragment' do
      @node.children = @node.fragment('<section class="a"></section>').child
      _(@node.inner_html).must_equal '<section class="a"></section>'
    end

    it 'with NodeSet' do
      @node.children = @node.fragment('<section class="a"></section><section class="b"></section').children
      _(@node.inner_html).must_equal '<section class="a"></section><section class="b"></section>'
    end
  end

  it 'is enumerable' do
    doc = Nokolexbor::HTML('<div attr1="1" attr2="2" attr3="3"></div>')
    node = doc.at_css('div')
    _(node.map {|k, v| v}.join).must_equal '123'
  end

  describe 'xpath' do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <div>
          <h1 id='topid' class='top'>
            <a>
              <h1 class='inner'>Text</h1>
            </a>
          </h1>
          <div class='a'>
          </div>
        </div>
      HTML
      @root = @doc.at_css('div')
    end

    it 'basic usage' do
      _(@root.xpath('./h1').size).must_equal 1
      _(@root.xpath('.//h1').size).must_equal 2
      _(@root.xpath('.//h1[@class="inner"]').size).must_equal 1
      _(@root.xpath('.//h1[@class="inner"]/text()').first.type).must_equal Nokolexbor::Node::TEXT_NODE
      _(@root.xpath('id("topid")').size).must_equal 1
    end

    it 'nodes in results are unique' do
      nodes = @root.xpath('.//h1', './/h1[@class="top"]', './/h1[@class="inner"]')
      _(nodes.size).must_equal 2
      _(nodes[0].name).must_equal 'h1'
      _(nodes[0]['class']).must_equal 'top'
      _(nodes[1].name).must_equal 'h1'
      _(nodes[1]['class']).must_equal 'inner'
    end

    it 'raises if expression is invalid' do
      _{ @root.xpath('.//text1()') }.must_raise Nokolexbor::XPath::SyntaxError
    end
  end
end