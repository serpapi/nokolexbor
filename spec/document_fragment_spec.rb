require 'spec_helper'

describe Nokolexbor::DocumentFragment do
  describe 'new' do
    before do
      @doc = Nokolexbor::HTML('')
    end

    it 'with context node' do
      fragment = Nokolexbor::DocumentFragment.new(@doc, '<div>Node</div>', @doc.at_css('body'))
      _(fragment).must_be_instance_of Nokolexbor::DocumentFragment
      _(fragment.to_html).must_equal '<div>Node</div>'
    end

    it 'without context node' do
      fragment = Nokolexbor::DocumentFragment.new(@doc, '<div>Node</div>')
      _(fragment).must_be_instance_of Nokolexbor::DocumentFragment
      _(fragment.to_html).must_equal '<div>Node</div>'
    end
  end

  it 'parse' do
    doc = Nokolexbor::DocumentFragment.parse("<span>3:30pm</span>")
    _(doc.to_s).must_equal '<span>3:30pm</span>'
  end

  it 'inner_html=' do
    fragment = Nokolexbor::DocumentFragment.parse("<hr />")
    fragment.inner_html = "hello"
    _(fragment.inner_html).must_equal "hello"
  end

  it 'have document' do
    fragment = Nokolexbor::DocumentFragment.new(Nokolexbor::HTML(''))
    _(fragment.document).must_be_instance_of Nokolexbor::Document
  end

  describe 'search' do
    before do
      @fragment = Nokolexbor::DocumentFragment.parse('<div></div><div></div>')
    end

    it 'by css' do
      _(@fragment.css('div').size).must_equal 2
    end

    it 'by at_css' do
      _(@fragment.at_css('div')).must_be_instance_of Nokolexbor::Element
    end

    it 'by xpath' do
      _(@fragment.xpath('.//div').size).must_equal 2
    end

    it 'by at_xpath' do
      _(@fragment.at_xpath('.//div')).must_be_instance_of Nokolexbor::Element
    end
  end

  it 'outer_html' do
    doc = "<div>foo<br><span></span>bar</div>"
    fragment = Nokolexbor::DocumentFragment.parse(doc)
    [:outer_html, :to_html, :to_s, :serialize].each do |method|
      _(fragment.send(method)).must_equal doc
    end
  end

  it 'can be inserted to doc' do
    doc = Nokolexbor::HTML('')
    frag = Nokolexbor::DocumentFragment.parse('<div></div><span></span>')
    doc.at_css('body').children = frag.children
    _(doc.at_css('body').inner_html).must_equal '<div></div><span></span>'
  end

end