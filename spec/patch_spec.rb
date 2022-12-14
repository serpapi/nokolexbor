require 'spec_helper'

describe "Patches to lexbor" do
  describe "::text selector" do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <div>
          Text 1
          <a class='a'>
            Text 2
          </a>
          Text 3
          Text 4
        </div>
      HTML
    end

    it "selects descendants" do
      _(@doc.css("div ::text").to_html.squish).must_equal "Text 1 Text 2 Text 3 Text 4"
    end

    it "selects children" do
      _(@doc.css("div > ::text").to_html.squish).must_equal "Text 1 Text 3 Text 4"
    end

    it "selects siblings" do
      _(@doc.css(".a ~ ::text").to_html.squish).must_equal "Text 3 Text 4"
    end
  end

  describe "case" do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <DiV id='Id'>
          <a class='kLs'>
          </a>
        </DiV>
      HTML
    end

    it "sensitive on id" do
      _(@doc.at_css("#Id")).wont_be_nil
      _(@doc.at_css("#id")).must_be_nil
    end

    it "sensitive on class name" do
      _(@doc.at_css(".kLs")).wont_be_nil
      _(@doc.at_css(".kls")).must_be_nil
    end

    it "insensitive on tag name" do
      _(@doc.at_css("DiV")).wont_be_nil
      _(@doc.at_css("div")).wont_be_nil
    end
  end

  describe "search contains results of template tag" do
    before do
      @doc = Nokolexbor::HTML <<-HTML
        <div class='a'>
          <div class='b'>
          </div>
          <template>
            <div class='c'>
            </div>
            <div class='d'>
              <div class='e'>
              </div>
            </div>
          </template>
        </DiV>
      HTML
    end

    it "on css" do
      _(@doc.css('div').size).must_equal 5
      _(@doc.css('div').map {|n| n['class']}).must_equal %w{a b c d e}
    end

    it "on xpath" do
      _(@doc.xpath('//div').size).must_equal 5
      _(@doc.xpath('//div').map {|n| n['class']}).must_equal %w{a b c d e}
    end

    it 'doc can be serialized without error' do
      _(@doc.to_html).must_be_instance_of String
    end
  end

end