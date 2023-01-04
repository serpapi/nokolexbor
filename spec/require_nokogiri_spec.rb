require 'spec_helper'

describe "Requires nokogiri" do
  it 'has no problem when nokogiri is required' do
    require 'nokogiri'
    doc = Nokogiri::HTML('<div></div>')
    doc = Nokolexbor::HTML('')
    _(doc.xpath('//body').to_html).must_equal '<body></body>'
  end

  describe "nokogiri_css" do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <div class="a"><span>Text 1</span>Text 2</div>
        <div class="a"><span>Text 3</span>Text 4</div>
        <div class="ab"><span>Text 5</span>Text 6</div>
      HTML
    end

    it 'Node#nokogiri_css' do
      nodes = @doc.nokogiri_css('.a /text()')
      _(nodes.size).must_equal 2
      _(nodes.first.text?).must_equal true
      _(nodes.to_html).must_equal 'Text 2Text 4'
    end

    it 'Node#nokogiri_at_css' do
      node = @doc.nokogiri_at_css('.a >text()')
      _(node).must_be_instance_of Nokolexbor::Text
      _(node.to_html).must_equal 'Text 2'
    end

    it 'NodeSet#nokogiri_css' do
      nodes = @doc.nokogiri_css('.a')
      _(nodes).must_be_instance_of Nokolexbor::NodeSet
      nodes = nodes.nokogiri_css('div text()')
      _(nodes.size).must_equal 4
      _(nodes.first.text?).must_equal true
      _(nodes.to_html).must_equal 'Text 1Text 2Text 3Text 4'
    end

    it 'NodeSet#nokogiri_at_css' do
      nodes = @doc.nokogiri_css('.a')
      _(nodes).must_be_instance_of Nokolexbor::NodeSet
      node = nodes.nokogiri_at_css('div text()')
      _(node).must_be_instance_of Nokolexbor::Text
      _(node.to_html).must_equal 'Text 1'
    end
  end
end