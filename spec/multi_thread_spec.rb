require 'spec_helper'

describe "Multi threading" do
  it 'should be thread-safe' do
    1000.times.map do
      Thread.new do
        random_str = (0...32).map { (65 + rand(58)).chr }.join
        doc = Nokolexbor::HTML('<div><a class="a">' + random_str + '</a></div>')
        _(doc.css("div ::text").text).must_equal random_str
      end
    end.each(&:join)
  end
end