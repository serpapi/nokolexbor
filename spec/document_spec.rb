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

  describe 'parse' do
    it 'works with string' do
      doc = Nokolexbor::Document.parse('<div>Hello</div>')
      _(doc.to_html).must_equal '<html><head></head><body><div>Hello</div></body></html>'
    end

    it 'works with io' do
      doc = Nokolexbor::Document.parse(StringIO.new('<div>Hello</div>'))
      _(doc.to_html).must_equal '<html><head></head><body><div>Hello</div></body></html>'
    end

    describe 'with non-utf-8 encoding' do
      before do
        @html_utf8 = '<html><head></head><body><div title="你好">世界！</div></body></html>'
        @html = @html_utf8.encode(Encoding::GBK)
        @doc = Nokolexbor::Document.parse(@html)
      end

      it 'source encoding is correct' do
        _(@html_utf8.encoding).must_equal Encoding::UTF_8
        _(@html.encoding).must_equal Encoding::GBK
      end

      it 'encoding of Document#to_html is utf-8' do
        output_html = @doc.to_html
        _(output_html.encoding).must_equal Encoding::UTF_8
        _(output_html).must_equal @html_utf8
      end

      it 'encoding of Node#text is utf-8' do
        text = @doc.at_css('div').text
        _(text.encoding).must_equal Encoding::UTF_8
        _(text).must_equal '世界！'
      end

      it 'encoding of Node#inner_html is utf-8' do
        inner_html = @doc.at_css('body').inner_html
        _(inner_html.encoding).must_equal Encoding::UTF_8
        _(inner_html).must_equal '<div title="你好">世界！</div>'
      end

      it 'encoding of Node#attr is utf-8' do
        attribute = @doc.at_css('div')['title']
        _(attribute.encoding).must_equal Encoding::UTF_8
        _(attribute).must_equal '你好'
      end

      it 'encoding of Attribute#value is utf-8' do
        attribute = @doc.at_css('div').attribute('title')
        value = attribute.value
        _(value.encoding).must_equal Encoding::UTF_8
        _(value).must_equal '你好'
      end
    end

    describe 'with invalid bytes' do
      it 'replaces invalid characters with �' do
        html = "<div>\xF0</div>"
        html.force_encoding(Encoding::GBK)
        doc = Nokolexbor::Document.parse(html)
        _(doc.to_html).must_equal '<html><head></head><body><div>�</div></body></html>'
      end
    end
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

    it 'Other attr that responds to to_s' do
      node = @doc.create_element('span', 1)
      _(node.to_html).must_equal '<span>1</span>'
      _(node.document).must_equal @doc
    end
  end

  it 'create_text_node' do
    doc = Nokolexbor::HTML('')
    node = doc.create_text_node('This is text')
    _(node).must_be_instance_of Nokolexbor::Text
    _(node.text?).must_equal true
    _(node.to_html).must_equal 'This is text'
    _(node.document).must_equal doc
  end

  it 'create_cdata' do
    doc = Nokolexbor::HTML('')
    node = doc.create_cdata('This is cdata')
    _(node).must_be_instance_of Nokolexbor::CDATA
    _(node.cdata?).must_equal true
    _(node.document).must_equal doc
  end

  it 'create_comment' do
    doc = Nokolexbor::HTML('')
    node = doc.create_comment('This is comment')
    _(node).must_be_instance_of Nokolexbor::Comment
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

  describe 'root' do
    before do
      @doc = Nokolexbor::HTML('<!DOCTYPE html><!--comment--><html><div></div></html>')
    end

    it 'is the first element node' do
      _(@doc.child.to_html).must_equal '<!DOCTYPE html>'
      _(@doc.root).must_equal @doc.at_css('html')
    end

    it 'root parent is Document' do
      parent = @doc.root.parent
      _(parent).must_equal @doc
      _(parent).must_be_instance_of Nokolexbor::Document
    end
  end
end