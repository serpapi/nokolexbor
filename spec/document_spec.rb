require 'spec_helper'

describe Nokolexbor::Document do
  before do
    @doc = Nokolexbor::HTML <<-HTML
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
    @first_li_html = '<li><a href="http://example1.com">Example 1</a></li>'
  end

  it 'is of type Nokolexbor::Document' do
    _(@doc).must_be_instance_of Nokolexbor::Document
  end

  it 'always wraps <html><head><body> when parsing incomplete HTML' do
    _(Nokolexbor::HTML('123').to_html).must_equal '<html><head></head><body>123</body></html>'
  end

  it 'new returns empty HTML' do
    _(Nokolexbor::Document.new.to_html).must_equal '<html><head></head><body></body></html>'
  end

  it 'works with at_css' do
    node = @doc.at_css('li')
    _(node).must_be_instance_of Nokolexbor::Element
    _(node.to_html).must_equal @first_li_html
  end

  it 'works with css' do
    nodes = @doc.css('li')
    _(nodes).must_be_instance_of Nokolexbor::NodeSet
    _(nodes.size).must_equal 3
    _(nodes.first.to_html).must_equal @first_li_html
  end

  it 'works with ::text selector' do
    nodes = @doc.css('article > ::text')
    _(nodes).must_be_instance_of Nokolexbor::NodeSet
    _(nodes.to_html.squish).must_equal 'Text content 1 Text content 2'
  end

  it 'works with at_xpath' do
    node = @doc.at_xpath('//li')
    _(node).must_be_instance_of Nokolexbor::Element
    _(node.to_html).must_equal @first_li_html
  end

  it 'works with xpath' do
    nodes = @doc.xpath('//li')
    _(nodes).must_be_kind_of Nokolexbor::NodeSet
    _(nodes.size).must_equal 3
    _(nodes.first.to_html).must_equal @first_li_html
  end

  describe 'title get' do
    it 'when <title> does not exist' do
      doc = Nokolexbor::HTML('')
      _(doc.title).must_equal ''
    end

    it 'when <title> exist' do
      doc = Nokolexbor::HTML('<html><head><title>This is a title</title></head></html>')
      _(doc.title).must_equal 'This is a title'
    end
  end

  describe 'title set' do
    it 'when <title> does not exist' do
      doc = Nokolexbor::HTML('')
      doc.title = 'This is a title'
      _(doc.at_css('title').to_html).must_equal '<title>This is a title</title>'
    end

    it 'when <title> exist' do
      doc = Nokolexbor::HTML('<html><head><title>This is a title</title></head></html>')
      doc.title = 'This is another title'
      _(doc.css('title').size).must_equal 1
      _(doc.at_css('title').to_html).must_equal '<title>This is another title</title>'
    end
  end

  it 'work with utf-8 chars' do
    doc = Nokolexbor::HTML('<div 属性1="值1"><span>文本1</span></div>')
    node = doc.at_css('div')
    _(node['属性1']).must_equal '值1'
    _(node.text).must_equal '文本1'
    _(node.inner_html).must_equal '<span>文本1</span>'
    _(node.at_css('::text').text).must_equal '文本1'
  end

  describe 'create_elemnt' do
    before do
      @doc = Nokolexbor::HTML('')
    end

    it 'Hash attr' do
      node = @doc.create_element('span', {'attr1' => 'value1', 'attr2' => 'value2'}, {'attr3' => 'value3'})
      _(node.to_html).must_equal '<span attr1="value1" attr2="value2" attr3="value3"></span>'
      _(node.document).must_equal @doc
    end

    it 'String attr' do
      node = @doc.create_element('span', "This is content")
      _(node.to_html).must_equal '<span>This is content</span>'
      _(node.document).must_equal @doc
    end

    it 'Other attr' do
      _{ @doc.create_element('span', 1) }.must_raise TypeError
    end
  end

  it 'create_text_node' do
    doc = Nokolexbor::HTML('')
    node = doc.create_text_node('This is text')
    _(node.text?).must_equal true
    _(node.to_html).must_equal 'This is text'
    _(node.document).must_equal doc
  end

  it 'create_cdata' do
    doc = Nokolexbor::HTML('')
    node = doc.create_cdata('This is cdata')
    _(node.cdata?).must_equal true
    _(node.document).must_equal doc
  end

  it 'create_comment' do
    doc = Nokolexbor::HTML('')
    node = doc.create_comment('This is comment')
    _(node.comment?).must_equal true
    _(node.to_html).must_equal '<!--This is comment-->'
    _(node.document).must_equal doc
  end

  describe 'meta_encoding' do
    it 'charset' do
      doc = Nokolexbor::HTML('<html><head><meta charset="utf-8"></head></html>')
      _(doc.meta_encoding).must_equal 'utf-8'
    end

    it 'http-equiv' do
      doc = Nokolexbor::HTML('<html><head><meta http-equiv="content-type" content="text/html; charset=utf-8"></head></html>')
      _(doc.meta_encoding).must_equal 'utf-8'
    end
  end

  describe 'meta_encoding=' do
    it 'charset' do
      doc = Nokolexbor::HTML('<html><head><meta charset="utf-8"></head></html>')
      doc.meta_encoding = 'aaa'
      _(doc.at_css('head').inner_html).must_equal '<meta charset="aaa">'
    end

    it 'http-equiv' do
      doc = Nokolexbor::HTML('<html><head><meta http-equiv="content-type" content="text/html; charset=utf-8"></head></html>')
      doc.meta_encoding = 'aaa'
      _(doc.at_css('head').inner_html).must_equal '<meta http-equiv="content-type" content="text/html; charset=aaa">'
    end

    it 'no meta tag' do
      doc = Nokolexbor::HTML('')
      doc.meta_encoding = 'aaa'
      _(doc.at_css('head').inner_html).must_equal '<meta charset="aaa">'
    end
  end

  describe 'scripting is turned on' do
    it 'when noscript tag is in head' do
      doc = Nokolexbor::HTML('<html><head><noscript><div>No</div></noscript></head></html>')
      _(doc.to_html).must_equal '<html><head><noscript><div>No</div></noscript></head><body></body></html>'
    end

    it 'when noscript tag is in body' do
      doc = Nokolexbor::HTML('<html><body><noscript><div>No</div></noscript></body></html>')
      _(doc.to_html).must_equal '<html><head></head><body><noscript><div>No</div></noscript></body></html>'
    end
  end
end