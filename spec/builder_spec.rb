require 'spec_helper'

describe Nokolexbor::Builder do
  describe 'top-level Nokolexbor() method' do
    it 'returns a DocumentFragment' do
      result = Nokolexbor { h1 'hi' }
      _(result).must_be_instance_of Nokolexbor::DocumentFragment
    end

    it 'returns nil when called without a block' do
      _(Nokolexbor()).must_be_nil
    end
  end

  describe 'no-arg block (instance_eval) style' do
    it 'creates a single element with text content' do
      result = Nokolexbor { h1 'Hello world' }
      _(result.to_html).must_equal '<h1>Hello world</h1>'
    end

    it 'creates nested elements' do
      result = Nokolexbor do
        ul do
          li 'one'
          li 'two'
        end
      end
      _(result.to_html).must_equal '<ul><li>one</li><li>two</li></ul>'
    end

    it 'builds the example from the README' do
      result = Nokolexbor do
        body do
          h1 'Hello world'
          p 'This little p'
          ul do
            li 'Go to market'
            li 'Go to bed'
          end
          table do
            tr do
              td 'Lindsay Lohan'
              td 'Paris Hilton'
            end
          end
        end
      end

      html = result.to_html
      _(html).must_include '<h1>Hello world</h1>'
      _(html).must_include '<p>This little p</p>'
      _(html).must_include '<li>Go to market</li>'
      _(html).must_include '<li>Go to bed</li>'
      _(html).must_include '<td>Lindsay Lohan</td>'
      _(html).must_include '<td>Paris Hilton</td>'
    end

    it 'sets attributes via hash argument' do
      result = Nokolexbor { div 'content', 'class' => 'box', 'id' => 'main' }
      node = result.at_css('div')
      _(node['class']).must_equal 'box'
      _(node['id']).must_equal 'main'
      _(node.text).must_equal 'content'
    end

    it 'creates multiple top-level elements' do
      result = Nokolexbor do
        h1 'Title'
        p 'Body'
      end
      _(result.to_html).must_equal '<h1>Title</h1><p>Body</p>'
    end
  end

  describe 'block-parameter style' do
    it 'creates elements via the builder argument' do
      result = Nokolexbor::Builder.new do |b|
        b.ul do
          b.li 'item one'
          b.li 'item two'
        end
      end
      _(result.parent.to_html).must_equal '<ul><li>item one</li><li>item two</li></ul>'
    end
  end

  describe 'NodeBuilder chaining' do
    it 'adds a class via method chain' do
      result = Nokolexbor { div.container }
      _(result.at_css('div')['class']).must_equal 'container'
    end

    it 'sets an id via trailing ! convention' do
      result = Nokolexbor { div.hero! }
      _(result.at_css('div')['id']).must_equal 'hero'
    end

    it 'chains class and id' do
      result = Nokolexbor { div.box.main! }
      node = result.at_css('div')
      _(node['class']).must_equal 'box'
      _(node['id']).must_equal 'main'
    end
  end

  describe 'special node helpers' do
    it 'inserts a text node via #text' do
      result = Nokolexbor { p { text 'plain text' } }
      _(result.at_css('p').text).must_equal 'plain text'
    end

    it 'inserts a comment via #comment' do
      result = Nokolexbor { comment 'a note' }
      _(result.to_html).must_equal '<!--a note-->'
    end
  end

  describe 'Builder.with' do
    it 'appends into an existing node' do
      doc  = Nokolexbor::HTML('<div id="root"></div>')
      root = doc.at_css('#root')
      Nokolexbor::Builder.with(root) do
        span 'injected'
      end
      _(root.inner_html).must_equal '<span>injected</span>'
    end
  end

  describe 'trailing _ tag name disambiguation' do
    it 'strips a trailing underscore from the tag name' do
      result = Nokolexbor { id_ '42' }
      _(result.to_html).must_equal '<id>42</id>'
    end
  end

  describe 'outer scope access in instance_eval mode' do
    it 'delegates unknown methods to the outer context' do
      def helper_method
        'from outer'
      end

      result = Nokolexbor { p helper_method }
      _(result.at_css('p').text).must_equal 'from outer'
    end
  end
end
