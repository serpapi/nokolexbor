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

  it "reuses the document index until a mutation or NodeSet reparenting" do
    previous_stats = ENV["NOKOLEXBOR_SELECTOR_INDEX_STATS"]
    ENV["NOKOLEXBOR_SELECTOR_INDEX_STATS"] = "1"

    compare_with_original(@doc, @selector)
    _(@doc.instance_variable_get(:@selector_index_rebuilds)).must_equal 1
    compare_with_original(@doc, ".target, #P42, [data-ix]")
    _(@doc.instance_variable_get(:@selector_index_rebuilds)).must_equal 1

    @doc.at_css("#P42")["class"] = "changed"
    compare_with_original(@doc, ".target, #P42, [data-ix]")
    _(@doc.instance_variable_get(:@selector_index_rebuilds)).must_equal 2

    @doc.css("article").css(".child, .target, [data-ix]")
    compare_with_original(@doc, @selector)
    _(@doc.instance_variable_get(:@selector_index_rebuilds)).must_equal 3

    @doc.css("article").at_css(".child, .target")
    compare_with_original(@doc, @selector)
    _(@doc.instance_variable_get(:@selector_index_rebuilds)).must_equal 4
  ensure
    previous_stats ? ENV["NOKOLEXBOR_SELECTOR_INDEX_STATS"] = previous_stats : ENV.delete("NOKOLEXBOR_SELECTOR_INDEX_STATS")
  end

  it "invalidates on attribute, content, fragment and structural edits" do
    node = @doc.at_css("#P42")
    attribute = node.attribute("data-ix")
    attribute.value = "new"
    compare_with_original(@doc, "[data-ix='new'], .target, #P42")

    attribute.name = "data-ref"
    compare_with_original(@doc, "[data-ref='new'], #P42, [data-ix='new']")

    node.remove_attr("class")
    node.at_css("span").content = "updated"
    compare_with_original(@doc, ".target, #P42, [data-ref]")

    node.inner_html = "<span class='fresh'>text</span>"
    node.add_next_sibling("<div id='extra' class='fresh'></div>")
    compare_with_original(@doc, ".fresh, #extra, [data-ref]")
    _(@doc.css(".fresh, #extra, [data-ref]").length).must_equal 3

    node.replace("<section id='replacement'></section>")
    compare_with_original(@doc, "#replacement, #P42, .fresh")
    _(@doc.at_css("#P42")).must_be_nil

    @doc.title = "Indexed"
    compare_with_original(@doc, "title, #replacement, .fresh")
    _(@doc.at_css("title").content).must_equal "Indexed"
  end

  it "invalidates both documents when nodes move between them" do
    other = Nokolexbor::HTML(@doc.to_html)
    selector = ".target, #P42, [data-ix]"
    @doc.css(selector)
    other.css(selector)

    @doc.at_css("main").add_child(other.at_css("#P42"))
    compare_with_original(@doc, selector)
    compare_with_original(other, selector)
    _(@doc.css(selector).length).must_equal 161
    _(other.css(selector).length).must_equal 159
  end

  it "invalidates for each native DOM writer" do
    previous_stats = ENV["NOKOLEXBOR_SELECTOR_INDEX_STATS"]
    ENV["NOKOLEXBOR_SELECTOR_INDEX_STATS"] = "1"
    compare_with_original(@doc, @selector)
    node = @doc.at_css("#P42")
    attribute = node.attribute("data-ix")
    victim = @doc.at_css("#P43")

    writers = [
      -> { attribute.value = "new" },
      -> { attribute.name = "data-ref" },
      -> { node["class"] = "changed" },
      -> { node.remove_attr("class") },
      -> { node.content = "text" },
      -> { node.parse("<em>fragment</em>") },
      -> { node.add_child("<em class='added'></em>") },
      -> { node.add_next_sibling("<div id='extra'></div>") },
      -> { victim.destroy },
      -> { node.remove },
      -> { @doc.title = "Indexed" },
    ]
    writers.each do |write|
      builds = @doc.instance_variable_get(:@selector_index_rebuilds)
      write.call
      compare_with_original(@doc, ".target, #P42, [data-ix]")
      _(@doc.instance_variable_get(:@selector_index_rebuilds)).must_equal builds + 1
    end
  ensure
    previous_stats ? ENV["NOKOLEXBOR_SELECTOR_INDEX_STATS"] = previous_stats : ENV.delete("NOKOLEXBOR_SELECTOR_INDEX_STATS")
  end

  it "matches case-sensitive multibyte and whitespace-separated classes" do
    @doc.at_css("#P42")["class"] = "  A\tcaf\u00e9  B "
    selector = ".caf\u00e9, .B, #P42, [data-ix='42']"

    compare_with_original(@doc, selector)
    _(@doc.css(selector).length).must_equal 1
    compare_with_original(@doc, ".CAF\u00c9, .a, #p42, [data-ix='missing']")
  end
end
