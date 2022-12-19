require 'spec_helper'

describe Nokolexbor::NodeSet do
  before do
    @doc = Nokolexbor::HTML <<-HTML
      <a>
        <div class='a'></div>
        <div class='b'></div>
        <div class='c'></div>
        <div class='d'></div>
        <div class='e'></div>
        <div class='f'></div>
        <h1><a></a></h1>
      </a>
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
      nodes = @root.css('a, h1, div.a')
      _(nodes.size).must_equal 4
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
end