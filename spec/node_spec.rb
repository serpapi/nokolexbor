require 'spec_helper'
require 'stringio'

describe Nokolexbor::Node do
  it 'new' do
    doc = Nokolexbor::HTML('')
    node = Nokolexbor::Node.new('div', doc)
    _(node).must_be_instance_of Nokolexbor::Element
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

  it 'content=' do
    doc = Nokolexbor::HTML('<div></div>')
    node = doc.at_css('div')
    node.content = '123<span></span>'
    _(node.inner_html).must_equal '123&lt;span&gt;&lt;/span&gt;'
    _{ node.content = 1 }.must_raise TypeError
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
      _{ @root.css('::text1') }.must_raise Nokolexbor::Lexbor::UnexpectedDataError
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

  describe 'inner_html' do
    it 'with indent' do
      doc = Nokolexbor::HTML('<span><div><div class="a"></div></div></span>')
      _(doc.at_css('span').inner_html(indent: 2)).must_equal <<-HTML
<div>
  <div class="a">
  </div>
</div>
HTML
    end

    it 'without indent' do
      doc = Nokolexbor::HTML('<span><div><div class="a"></div></div></span>')
      _(doc.at_css('span').inner_html).must_equal '<div><div class="a"></div></div>'
    end
  end

  describe 'outer_html' do
    it 'with indent' do
      doc = Nokolexbor::HTML('<div><div class="a"></div></div>')
      [:outer_html, :to_html, :to_s].each do |method|
        _(doc.at_css('div').send(method, indent: 2)).must_equal "<div>\n  <div class=\"a\">\n  </div>\n</div>\n"
      end
    end

    it 'without indent' do
      doc = Nokolexbor::HTML('<div><div class="a"></div></div>')
      [:outer_html, :to_html, :to_s, :serialize].each do |method|
        _(doc.at_css('div').send(method)).must_equal '<div><div class="a"></div></div>'
      end
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

    it 'value?' do
      _(@doc.at_css('div').value?('a')).must_equal true
      _(@doc.at_css('div').value?('c')).must_equal false
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

  it 'parent=' do
    doc = Nokolexbor::HTML('<div></div>')
    parent = Nokolexbor::Node.new('span', doc)
    doc.at_css('div').parent = parent
    _(doc.to_html).must_equal '<html><head></head><body></body></html>'
    doc.at_css('body') << parent
    _(doc.to_html).must_equal '<html><head></head><body><span><div></div></span></body></html>'
  end

  it 'traverse' do
    doc = Nokolexbor::HTML('<div>123<span></span><a><b></b></a>456</div>')
    nodes = []
    doc.at_css('div').traverse do |node|
      nodes << node
    end
    _(nodes.size).must_equal 6
    _(nodes[0].name).must_equal '#text'
    _(nodes[1].name).must_equal 'span'
    _(nodes[2].name).must_equal 'b'
    _(nodes[3].name).must_equal 'a'
    _(nodes[4].name).must_equal '#text'
    _(nodes[5].name).must_equal 'div'
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

  it 'elements' do
    doc = Nokolexbor::HTML <<-HTML
      <div><span class='a'></span>123<span class='b'></span></div>
    HTML
    [:elements, :element_children].each do |method|
      children = doc.at_css('div').send(method)
      _(children.size).must_equal 2
      _(children[0]['class']).must_equal 'a'
      _(children[1]['class']).must_equal 'b'
    end
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

  describe 'equals' do
    it 'between elements' do
      doc = Nokolexbor::HTML <<-HTML
        <div><span class='a'></span></div>
      HTML
      node1 = doc.at_css('span')
      node2 = doc.at_css('.a')
      _(node1).must_equal node2
    end

    it 'between text' do
      doc = Nokolexbor::HTML <<-HTML
        <div>123</div>
      HTML
      node = doc.at_css('div').child
      _(node).must_equal node
    end

    it 'between attributes' do
      doc = Nokolexbor::HTML <<-HTML
        <div class='a'></div>
      HTML
      attri = doc.at_css('div')['class']
      _(attri).must_equal attri
    end

    it 'against non-node type' do
      doc = Nokolexbor::HTML('<div></div>')
      node = doc.at_css('div')
      _(doc == nil).must_equal false
      _(node == nil).must_equal false
      _(node == 1).must_equal false
      _(node == 'string').must_equal false
      _(node == []).must_equal false
      _(node == {}).must_equal false
    end
  end

  it 'name' do
    doc = Nokolexbor::HTML <<-HTML
      <DiV></DiV>
    HTML
    [:name, :node_name].each do |method|
      _(doc.at_css('div').send(method)).must_equal 'div'
    end
  end

  describe 'fragment' do
    before do
      @doc = Nokolexbor::HTML('')
      @frag = @doc.fragment('<div></div><span></span>')
    end

    it 'returns Nokolexbor::DocumentFragment' do
      _(@frag).must_be_kind_of Nokolexbor::DocumentFragment
    end

    it 'to_html works' do
      _(@frag.to_html).must_equal '<div></div><span></span>'
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
        ret = @node.add_next_sibling('<span class="a"></span><div></div>')
        _(@root.inner_html).must_equal '<a></a><span></span><span class="a"></span><div></div>'
        _(ret).must_be_instance_of Nokolexbor::NodeSet
        _(ret.to_html).must_equal '<span class="a"></span><div></div>'
      end

      it 'with DocumentFragment' do
        ret = @node.add_next_sibling(@node.fragment('<span class="a"></span><div></div>'))
        _(@root.inner_html).must_equal '<a></a><span></span><span class="a"></span><div></div>'
        _(ret).must_be_instance_of Nokolexbor::NodeSet
        _(ret.to_html).must_equal '<span class="a"></span><div></div>'
      end

      it 'with NodeSet' do
        ret = @node.add_next_sibling(@node.fragment('<span class="a"></span><div></div>').children)
        _(@root.inner_html).must_equal '<a></a><span></span><span class="a"></span><div></div>'
        _(ret).must_be_instance_of Nokolexbor::NodeSet
        _(ret.to_html).must_equal '<span class="a"></span><div></div>'
      end

      it 'with existing node' do
        ret = @node.add_next_sibling(@root.at_css('a'))
        _(@root.inner_html).must_equal '<span></span><a></a>'
        _(ret).must_be_instance_of Nokolexbor::Element
      end
    end

    describe 'to previous' do
      before do
        @node = @doc.at_css('a')
      end

      it 'with String' do
        ret = @node.add_previous_sibling('<span class="a"></span><div></div>')
        _(@root.inner_html).must_equal '<span class="a"></span><div></div><a></a><span></span>'
        _(ret).must_be_instance_of Nokolexbor::NodeSet
        _(ret.to_html).must_equal '<span class="a"></span><div></div>'
      end

      it 'with DocumentFragment' do
        ret = @node.add_previous_sibling(@node.fragment('<span class="a"></span><div></div>'))
        _(@root.inner_html).must_equal '<span class="a"></span><div></div><a></a><span></span>'
        _(ret).must_be_instance_of Nokolexbor::NodeSet
        _(ret.to_html).must_equal '<span class="a"></span><div></div>'
      end

      it 'with NodeSet' do
        ret = @node.add_previous_sibling(@node.fragment('<span class="a"></span><div></div>').children)
        _(@root.inner_html).must_equal '<span class="a"></span><div></div><a></a><span></span>'
        _(ret).must_be_instance_of Nokolexbor::NodeSet
        _(ret.to_html).must_equal '<span class="a"></span><div></div>'
      end

      it 'with existing node' do
        ret = @node.add_previous_sibling(@root.at_css('span'))
        _(@root.inner_html).must_equal '<span></span><a></a>'
        _(ret).must_be_instance_of Nokolexbor::Element
      end
    end
  end

  describe 'add_child' do
    before do
      @doc = Nokolexbor::HTML('<div><span></span></div><a></a>')
      @node = @doc.at_css('div')
    end

    it 'with String' do
      ret = @node.add_child('<span class="a"></span><div></div>')
      _(@node.inner_html).must_equal '<span></span><span class="a"></span><div></div>'
      _(ret).must_be_instance_of Nokolexbor::NodeSet
      _(ret.to_html).must_equal '<span class="a"></span><div></div>'
    end

    it 'with DocumentFragment' do
      ret = @node.add_child(@node.fragment('<span class="a"></span><div></div>'))
      _(@node.inner_html).must_equal '<span></span><span class="a"></span><div></div>'
      _(ret).must_be_instance_of Nokolexbor::NodeSet
      _(ret.to_html).must_equal '<span class="a"></span><div></div>'
    end

    it 'with NodeSet' do
      ret = @node.add_child(@node.fragment('<span class="a"></span><div></div>').children)
      _(@node.inner_html).must_equal '<span></span><span class="a"></span><div></div>'
      _(ret).must_be_instance_of Nokolexbor::NodeSet
      _(ret.to_html).must_equal '<span class="a"></span><div></div>'
    end

    it 'with existing node' do
      ret = @node.add_child(@doc.at_css('a'))
      _(@doc.at_css('body').inner_html).must_equal '<div><span></span><a></a></div>'
      _(ret).must_be_instance_of Nokolexbor::Element
    end
  end

  describe 'prepend_child' do
    before do
      @doc = Nokolexbor::HTML('<div><span></span></div><a></a>')
      @node = @doc.at_css('div')
    end

    it 'with String' do
      ret = @node.prepend_child('<span class="a"></span><div></div>')
      _(@node.inner_html).must_equal '<span class="a"></span><div></div><span></span>'
      _(ret).must_be_instance_of Nokolexbor::NodeSet
      _(ret.to_html).must_equal '<span class="a"></span><div></div>'
    end

    it 'with DocumentFragment' do
      ret = @node.prepend_child(@node.fragment('<span class="a"></span><div></div>'))
      _(@node.inner_html).must_equal '<span class="a"></span><div></div><span></span>'
      _(ret).must_be_instance_of Nokolexbor::NodeSet
      _(ret.to_html).must_equal '<span class="a"></span><div></div>'
    end

    it 'with NodeSet' do
      ret = @node.prepend_child(@node.fragment('<span class="a"></span><div></div>').children)
      _(@node.inner_html).must_equal '<span class="a"></span><div></div><span></span>'
      _(ret).must_be_instance_of Nokolexbor::NodeSet
      _(ret.to_html).must_equal '<span class="a"></span><div></div>'
    end

    it 'with existing node' do
      ret = @node.prepend_child(@doc.at_css('a'))
      _(@doc.at_css('body').inner_html).must_equal '<div><a></a><span></span></div>'
      _(ret).must_be_kind_of Nokolexbor::Node
    end

    it 'when node has empty children' do
      node = @doc.at_css('span')
      ret = node.prepend_child('<b />')
      _(node.inner_html).must_equal '<b></b>'
      _(ret).must_be_instance_of Nokolexbor::NodeSet
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
        <div class='a'><!-- comment -->
          123 456
          <span></span>
          <a></a>
          789 012
          <template>
            345
            <b></b>
            <strong></strong>
            789
          </template>
        </div>
      HTML
      @root = @doc.at_css('div')
      @comment = @root.child
      @attr = @root.attribute('class')
      @text = @doc.at_css('::text')
      @frag = @doc.at_css('template').child
    end

    it 'check types' do
      _(@doc).must_be_instance_of Nokolexbor::Document
      _(@root).must_be_instance_of Nokolexbor::Element
      _(@comment).must_be_instance_of Nokolexbor::Comment
      _(@attr).must_be_instance_of Nokolexbor::Attribute
      _(@text).must_be_instance_of Nokolexbor::Text
      _(@frag).must_be_instance_of Nokolexbor::DocumentFragment
    end

    it 'first_element_child' do
      _(@doc.first_element_child.name).must_equal 'html'
      _(@root.first_element_child.name).must_equal 'span'
      _(@comment.first_element_child).must_be_nil
      _(@attr.first_element_child).must_be_nil
      _(@text.first_element_child).must_be_nil
      _(@frag.first_element_child.name).must_equal 'b'
    end

    it 'last_element_child' do
      _(@doc.last_element_child.name).must_equal 'html'
      _(@root.last_element_child.name).must_equal 'template'
      _(@comment.last_element_child).must_be_nil
      _(@attr.last_element_child).must_be_nil
      _(@text.last_element_child).must_be_nil
      _(@frag.last_element_child.name).must_equal 'strong'
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
      @node.wrap('<div></div><a></a>')
      _(@doc.at_css('body').inner_html).must_equal '<div><span>123</span></div>'
    end

    it 'with Node' do
      @node.wrap(@doc.create_element('div'))
      _(@doc.at_css('body').inner_html).must_equal '<div><span>123</span></div>'
    end

    it 'with DocumentFragment' do
      @node.wrap(@node.fragment('<div></div><a></a>'))
      _(@doc.at_css('body').inner_html).must_equal '<div><span>123</span></div>'
    end
  end

  it 'matches?' do
    doc = Nokolexbor::HTML('<span class="a"><div>123<div></span>')
    node = doc.at_css('span')
    _(node.matches?('span.a:has(div)')).must_equal true
  end

  describe 'attribute' do
    before do
      @doc = Nokolexbor::HTML('<div attr1="1" attr2="2" attr3="3">Text 1</div>')
    end
    it 'on Element' do
      a = @doc.at_css('div').attribute('attr1')
      _(a).must_be_instance_of Nokolexbor::Attribute
      _(a.name).must_equal 'attr1'
      _(a.value).must_equal '1'
    end

    it 'on Document' do
      _(@doc.attribute('attr1')).must_be_nil
    end

    it 'on Text' do
      _(@doc.at_css('div').child.attribute('attr1')).must_be_nil
    end

    it 'changes reflect to doc' do
      a = @doc.at_css('div').attribute('attr1')
      a.value = 'new1'
      _(@doc.at_css('div').to_html).must_equal '<div attr1="new1" attr2="2" attr3="3">Text 1</div>'
    end
  end

  it 'attributes' do
    doc = Nokolexbor::HTML('<div attr1="1" attr2="2" attr3="3"></div>')
    attrs = doc.at_css('div').attributes
    _(attrs).must_be_instance_of Hash
    _(attrs.values.size).must_equal 3
    attrs.each do |k, v|
      _(v).must_be_instance_of Nokolexbor::Attribute
    end
  end

  describe 'replace' do
    before do
      @doc = Nokolexbor::HTML('<div><span></span></div>')
      @node = @doc.at_css('div')
      @parent = @node.parent
    end

    it 'with String' do
      ret = @node.replace('<section class="a"></section><div></div>')
      _(@parent.inner_html).must_equal '<section class="a"></section><div></div>'
      _(ret).must_be_instance_of Nokolexbor::NodeSet
    end

    it 'with DocumentFragment' do
      ret = @node.replace(@node.fragment('<section class="a"></section><section class="b"></section'))
      _(@parent.inner_html).must_equal '<section class="a"></section><section class="b"></section>'
      _(ret).must_be_instance_of Nokolexbor::NodeSet
    end

    it 'with Node' do
      ret = @node.replace(@doc.create_element('section', '123', {'class' => 'a'}))
      _(@doc.at_css('body').inner_html).must_equal '<section class="a">123</section>'
      _(ret).must_be_kind_of Nokolexbor::Node
    end

    it 'with NodeSet' do
      ret = @node.replace(@node.fragment('<section class="a"></section><section class="b"></section').children)
      _(@parent.inner_html).must_equal '<section class="a"></section><section class="b"></section>'
      _(ret).must_be_instance_of Nokolexbor::NodeSet
    end
  end

  describe 'children=' do
    before do
      @doc = Nokolexbor::HTML('')
      @node = @doc.at_css('body')
    end

    it 'with String' do
      @node.children = '<section class="a"></section><div></div>'
      _(@node.inner_html).must_equal '<section class="a"></section><div></div>'
    end

    it 'with DocumentFragment' do
      @node.children = @node.fragment('<section class="a"></section><section class="b"></section')
      _(@node.inner_html).must_equal '<section class="a"></section><section class="b"></section>'
    end

    it 'with Node' do
      @node.children = @doc.create_element('section', '123', {'class' => 'a'})
      _(@node.inner_html).must_equal '<section class="a">123</section>'
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

  describe "Element" do
    it "can be newed directly" do
      node = Nokolexbor::Element.new('div', Nokolexbor::HTML(''))
      _(node).must_be_instance_of Nokolexbor::Element
      _(node.element?).must_equal true
      _(node.to_html).must_equal '<div></div>'
    end

    it 'can be added to html' do
      doc = Nokolexbor::HTML('')
      node = Nokolexbor::Element.new('div', doc)
      doc.at_css('body').add_child node
      _(doc.at_css('body').inner_html).must_equal '<div></div>'
    end

    it 'selection be of correct type' do
      doc = Nokolexbor::HTML('<div></div>')
      _(doc.at_css('div')).must_be_instance_of Nokolexbor::Element
    end

    it 'raises TypeError if name is not String' do
      _{ Nokolexbor::Element.new(1, Nokolexbor::HTML('')) }.must_raise TypeError
    end
  end

  describe "Text" do
    it "can be newed directly" do
      node = Nokolexbor::Text.new('this is a text', Nokolexbor::HTML(''))
      _(node).must_be_instance_of Nokolexbor::Text
      _(node.text?).must_equal true
      _(node.to_html).must_equal 'this is a text'
    end

    it 'can be added to html' do
      doc = Nokolexbor::HTML('')
      node = Nokolexbor::Text.new('this is a text', doc)
      doc.at_css('body').add_child node
      _(doc.at_css('body').inner_html).must_equal 'this is a text'
    end

    it 'selection be of correct type' do
      doc = Nokolexbor::HTML('<div>this is a text</div>')
      _(doc.at_css('div ::text')).must_be_instance_of Nokolexbor::Text
    end

    it 'raises TypeError if name is not String' do
      _{ Nokolexbor::Text.new(1, Nokolexbor::HTML('')) }.must_raise TypeError
    end
  end

  describe "Comment" do
    it "can be newed directly" do
      node = Nokolexbor::Comment.new('this is a comment', Nokolexbor::HTML(''))
      _(node).must_be_instance_of Nokolexbor::Comment
      _(node.comment?).must_equal true
      _(node.to_html).must_equal '<!--this is a comment-->'
    end

    it 'can be added to html' do
      doc = Nokolexbor::HTML('')
      node = Nokolexbor::Comment.new('this is a comment', doc)
      doc.at_css('body').add_child node
      _(doc.at_css('body').inner_html).must_equal '<!--this is a comment-->'
    end

    it 'selection be of correct type' do
      doc = Nokolexbor::HTML('<div><!-- this is a comment --></div>')
      _(doc.at_css('div').child).must_be_instance_of Nokolexbor::Comment
    end

    it 'raises TypeError if name is not String' do
      _{ Nokolexbor::Comment.new(1, Nokolexbor::HTML('')) }.must_raise TypeError
    end
  end

  describe "ProcessingInstruction" do
    it "can be newed directly" do
      node = Nokolexbor::ProcessingInstruction.new('pi_name', 'pi_content', Nokolexbor::HTML(''))
      _(node).must_be_instance_of Nokolexbor::ProcessingInstruction
      _(node.processing_instruction?).must_equal true
      _(node.to_html).must_equal '<?pi_name pi_content>'
    end

    it 'can be added to html' do
      doc = Nokolexbor::HTML('')
      node = Nokolexbor::ProcessingInstruction.new('pi_name', 'pi_content', Nokolexbor::HTML(''))
      doc.at_css('body').add_child node
      _(doc.at_css('body').inner_html).must_equal '<?pi_name pi_content>'
    end

    it 'raises TypeError if name is not String' do
      _{ Nokolexbor::ProcessingInstruction.new(1, 2, Nokolexbor::HTML('')) }.must_raise TypeError
    end
  end

  describe 'write_to' do
    it 'with indent' do
      io = StringIO.new
      doc = Nokolexbor::HTML('')
      doc.write_to(io, indent: 2)
      _(io.string).must_equal <<-HTML
<html>
  <head>
  </head>
  <body>
  </body>
</html>
HTML
    end

    it 'without indent' do
      io = StringIO.new
      doc = Nokolexbor::HTML('')
      doc.write_to(io)
      _(io.string).must_equal '<html><head></head><body></body></html>'
    end
  end

  describe 'inspect' do
    it 'Document' do
      inspect_str = Nokolexbor::HTML('<div></div>').inspect
      _(inspect_str).must_match /#<Nokolexbor::Document:0x[0-9a-f]+>/
    end

    it 'DocumentFragment' do
      inspect_str = Nokolexbor::HTML('').fragment('<div></div>').inspect
      _(inspect_str).must_match /#<Nokolexbor::DocumentFragment:0x[0-9a-f]+ .+>/
    end

    it 'Element' do
      inspect_str = Nokolexbor::HTML('<div class="a b">123</div>').at_css('div').inspect
      _(inspect_str).must_equal '#<Nokolexbor::Element <div class="a b">>'
    end

    it 'Text' do
      inspect_str = Nokolexbor::HTML('<div>123 abc</div>').at_css('div').child.inspect
      _(inspect_str).must_equal '#<Nokolexbor::Text 123 abc>'
    end

    it 'CDATA' do
      inspect_str = Nokolexbor::CDATA.new('123 abc', Nokolexbor::HTML('')).inspect
      _(inspect_str).must_match /#<Nokolexbor::CDATA:0x[0-9a-f]+ .+>/
    end

    it 'Comment' do
      inspect_str = Nokolexbor::HTML('<!-- 123 abd -->').child.inspect
      _(inspect_str).must_equal '#<Nokolexbor::Comment <!-- 123 abd -->>'
    end

    it 'ProcessingInstruction' do
      inspect_str = Nokolexbor::ProcessingInstruction.new('xml', '123 abc', Nokolexbor::HTML('')).inspect
      _(inspect_str).must_equal '#<Nokolexbor::ProcessingInstruction <?xml 123 abc>>'
    end

    it 'Attribute' do
      inspect_str = Nokolexbor::HTML('<div class="a b"></div>').at_css('div').attribute('class').inspect
      _(inspect_str).must_equal '#<Nokolexbor::Attribute class="a b">'
      inspect_str = Nokolexbor::HTML('<div enabled></div>').at_css('div').attribute('enabled').inspect
      _(inspect_str).must_equal '#<Nokolexbor::Attribute enabled="">'
    end

    it 'NodeSet' do
      inspect_str = Nokolexbor::HTML('<div class="a b"></div><div style="c d"></div>').css('div').inspect
      _(inspect_str).must_equal '[#<Nokolexbor::Element <div class="a b">>, #<Nokolexbor::Element <div style="c d">>]'
    end
  end

  it 'path' do
    doc = Nokolexbor::HTML("<div><span><a class='a'>123</a></div>")
    _(doc.at_css('.a').path).must_equal '/html/body/div/span/a'
    _(doc.at_css('.a').attribute('class').path).must_equal '/html/body/div/span/a/@class'
    _(doc.at_css('.a').child.path).must_equal '/html/body/div/span/a/text()'

    doc = Nokolexbor::HTML("<div><span><a class='a'></a><a class='b'></a></div>")
    _(doc.at_css('.a').path).must_equal '/html/body/div/span/a[1]'
    _(doc.at_css('.b').path).must_equal '/html/body/div/span/a[2]'
  end
end