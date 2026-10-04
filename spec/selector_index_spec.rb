require "spec_helper"

describe "indexed CSS selectors" do
  before do
    html = 160.times.map do |i|
      "<article><div id=\"P#{i}\" class=\"target\" data-ix=\"#{i}\"><span class=\"child\">#{i}</span></div></article>"
    end.join
    @doc = Nokolexbor::HTML("<main>#{html}</main>")
    @selector = "article .child, div.target, #P42, [data-ix]"
  end

  def compare_with_original(scope, selector)
    previous_index = ENV["NOKOLEXBOR_SELECTOR_INDEX"]
    previous_shadow = ENV["NOKOLEXBOR_SELECTOR_SHADOW"]

    ENV["NOKOLEXBOR_SELECTOR_INDEX"] = "0"
    ENV.delete("NOKOLEXBOR_SELECTOR_SHADOW")
    expected_nodes = scope.css(selector).to_a
    expected_first = scope.at_css(selector)

    ENV["NOKOLEXBOR_SELECTOR_INDEX"] = "1"
    ENV["NOKOLEXBOR_SELECTOR_SHADOW"] = "1"
    assert_equal expected_nodes, scope.css(selector).to_a
    if expected_first
      assert_equal expected_first, scope.at_css(selector)
    else
      assert_nil scope.at_css(selector)
    end
  ensure
    previous_index ? ENV["NOKOLEXBOR_SELECTOR_INDEX"] = previous_index : ENV.delete("NOKOLEXBOR_SELECTOR_INDEX")
    previous_shadow ? ENV["NOKOLEXBOR_SELECTOR_SHADOW"] = previous_shadow : ENV.delete("NOKOLEXBOR_SELECTOR_SHADOW")
  end

  it "preserves document order, duplicates, classes, IDs and attributes" do
    compare_with_original(@doc, @selector)
    _(@doc.css(@selector).length).must_equal 320
    _(@doc.at_css(@selector)["id"]).must_equal "P0"
    compare_with_original(@doc, "#p42, .absent, [not-present]")
    _(@doc.css("#p42, .absent, [not-present]").length).must_equal 0
    compare_with_original(@doc, "article + article .child, article > .target, #P42, [data-ix]")
    compare_with_original(@doc, ".target ::text, .child, [data-ix], #P42")
  end

  it "preserves subtree and NodeSet searches after DOM mutations" do
    @doc.at_css("#P42")["class"] = "renamed"
    @doc.at_css("#P43").remove
    @doc.at_css("main").add_child("<div id=\"extra\" class=\"target\" data-ix=\"new\"><span class=\"child\">new</span></div>")

    compare_with_original(@doc.at_css("main"), @selector)
    compare_with_original(@doc.css("article"), @selector)
    compare_with_original(@doc, ".target:has(.child), .child, [data-ix], #extra")
    _(@doc.css(@selector).length).must_equal 319
  end

  it "matches case-sensitive multibyte and whitespace-separated classes" do
    @doc.at_css("#P42")["class"] = "  A\tcaf\u00e9  B "
    selector = ".caf\u00e9, .B, #P42, [data-ix='42']"

    compare_with_original(@doc, selector)
    _(@doc.css(selector).length).must_equal 1
    compare_with_original(@doc, ".CAF\u00c9, .a, #p42, [data-ix='missing']")
  end

  it "preserves match order when the GVL is released" do
    previous = Nokolexbor.release_gvl
    Nokolexbor.release_gvl = true
    compare_with_original(@doc, @selector)
    compare_with_original(@doc.css("article"), @selector)
  ensure
    Nokolexbor.release_gvl = previous
  end
end
