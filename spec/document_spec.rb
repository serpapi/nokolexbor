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
    _(node).must_be_instance_of Nokolexbor::Node
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
    _(node).must_be_instance_of Nokolexbor::Node
    _(node.to_html).must_equal @first_li_html
  end

  it 'works with xpath' do
    nodes = @doc.xpath('//li')
    _(nodes).must_be_instance_of Nokolexbor::NodeSet
    _(nodes.size).must_equal 3
    _(nodes.first.to_html).must_equal @first_li_html
  end
end