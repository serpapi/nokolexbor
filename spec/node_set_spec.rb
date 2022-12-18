require 'spec_helper'

describe Nokolexbor::NodeSet do
  describe 'css' do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <div>
          <div class='a'>
          </div>
          <div class='b'>
          </div>
        </div>
      HTML
    end

    it 'searches direct children' do
      nodes = @doc.css('div')
      nodes2 = nodes.css('div').css('div').css('div')
      _(nodes.size).must_equal(nodes2.size)
      nodes.each_with_index do |node, index|
        _(node).must_equal nodes2[index]
      end
    end
  end
end