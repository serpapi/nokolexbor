require "spec_helper"

# Expected results come from how the documents are built, not from the library.
# A NOKOLEXBOR_SELECTOR_CHECK build also checks every search against the plain
# traversal (see README, "Selector safety checks").
describe "indexed CSS selectors" do
  before do
    html = 160.times.map do |i|
      "<article><div id=\"P#{i}\" class=\"target\" data-ix=\"#{i}\"><span class=\"child\">#{i}</span></div></article>"
    end.join
    @doc = Nokolexbor::HTML("<main>#{html}</main>")
    @selector = "article .child, div.target, #P42, [data-ix]"
  end

  def labels(nodes)
    Array(nodes).map { |node| node["id"] || "#{node.name}:#{node.text}" }
  end

  def div_and_span(i)
    ["P#{i}", "span:#{i}"]
  end

  it "preserves document order, duplicates, classes, IDs and attributes" do
    expected = 160.times.flat_map { |i| div_and_span(i) }
    _(labels(@doc.css(@selector))).must_equal expected
    _(labels(@doc.css(@selector))).must_equal expected
    _(@doc.at_css(@selector)["id"]).must_equal "P0"

    _(@doc.css("#p42, .absent, [not-present]").to_a).must_equal []
    _(@doc.at_css("#p42, .absent, [not-present]")).must_be_nil

    expected = 160.times.flat_map { |i| i.zero? ? ["P0"] : div_and_span(i) }
    _(labels(@doc.css("article + article .child, article > .target, #P42, [data-ix]"))).must_equal expected

    expected = 160.times.flat_map { |i| div_and_span(i) + ["#text:#{i}"] }
    _(labels(@doc.css(".target ::text, .child, [data-ix], #P42"))).must_equal expected
  end

  it "searches subtrees and NodeSets of an indexed document after DOM mutations" do
    @doc.css(@selector)
    @doc.at_css("#P42")["class"] = "renamed"
    @doc.at_css("#P43").remove
    @doc.at_css("main").add_child("<div id=\"extra\" class=\"target\" data-ix=\"new\"><span class=\"child\">new</span></div>")

    kept = (0...160).to_a - [43]
    expected = kept.flat_map { |i| div_and_span(i) } + ["extra"]
    _(labels(@doc.at_css("main").css(@selector))).must_equal expected
    _(labels(@doc.css(@selector))).must_equal expected
    _(labels(@doc.css("article").css(@selector))).must_equal kept.flat_map { |i| div_and_span(i) }
    _(labels(@doc.css("article")[10].css(@selector))).must_equal ["P10"]
    _(labels(@doc.css("#P5, #P7").css("span"))).must_equal ["span:5", "span:7"]

    expected = kept.flat_map { |i| div_and_span(i) } + ["extra", "span:new"]
    _(labels(@doc.css(".target:has(.child), .child, [data-ix], #extra"))).must_equal expected
  end

  it "sees every native DOM writer" do
    node = @doc.at_css("#P42")
    attribute = node.attribute("data-ix")
    victim = @doc.at_css("#P43")
    @doc.css(".target, #P42, [data-ix]")

    attribute.value = "new"
    _(labels(@doc.css("[data-ix='new']"))).must_equal ["P42"]

    attribute.name = "data-ref"
    _(labels(@doc.css("[data-ref]"))).must_equal ["P42"]
    _(@doc.css("[data-ix]").length).must_equal 159

    node["class"] = "changed"
    _(labels(@doc.css(".changed"))).must_equal ["P42"]
    _(@doc.css(".target").length).must_equal 159

    node.remove_attr("class")
    _(@doc.css(".changed").to_a).must_equal []

    node.content = "text"
    _(@doc.css("#P42 span").to_a).must_equal []
    _(@doc.css("span").length).must_equal 159

    node.parse("<em>fragment</em>")
    _(@doc.css("em").to_a).must_equal []

    node.add_child("<em class='added'></em>")
    _(labels(@doc.css(".added, #P42 > em"))).must_equal ["em:"]

    node.add_next_sibling("<div id='extra' class='target'></div>")
    _(labels(@doc.css("#P42, #extra"))).must_equal ["P42", "extra"]
    _(labels(@doc.css("article")[42].css(".target, [data-ref]"))).must_equal ["P42", "extra"]

    victim.destroy
    _(@doc.at_css("#P43")).must_be_nil
    _(@doc.css(".target").length).must_equal 159

    node.remove
    _(@doc.at_css("#P42")).must_be_nil
    _(@doc.css("[data-ref], .added").to_a).must_equal []

    @doc.title = "Indexed"
    _(@doc.at_css("title").content).must_equal "Indexed"
  end

  it "sees attribute, content, fragment and structural edits" do
    node = @doc.at_css("#P42")
    @doc.css(".target")

    node.inner_html = "<span class='fresh'>text</span>"
    node.add_next_sibling("<div id='extra' class='fresh'></div>")
    _(labels(@doc.css(".fresh, #extra, #P42"))).must_equal ["P42", "span:text", "extra"]

    node.replace("<section id='replacement' class='target'></section>")
    _(@doc.at_css("#P42")).must_be_nil
    _(labels(@doc.css("article")[42].css(".target, .fresh"))).must_equal ["replacement", "extra"]
    _(@doc.css(".target").length).must_equal 160
  end

  it "sees nodes that move between two indexed documents" do
    other = Nokolexbor::HTML(@doc.to_html)
    selector = ".target, #P42, [data-ix]"
    @doc.css(selector)
    other.css(selector)

    @doc.at_css("main").add_child(other.at_css("#P42"))
    _(@doc.css(selector).length).must_equal 161
    _(labels(@doc.css(".target")).last(2)).must_equal ["P159", "P42"]
    _(other.css(selector).length).must_equal 159
    _(other.at_css("#P42")).must_be_nil
  end

  it "matches case-sensitive multibyte and whitespace-separated classes" do
    @doc.css(".target")
    @doc.at_css("#P42")["class"] = "  A\tcaf\u00e9  B "

    _(labels(@doc.css(".caf\u00e9, .B, #P42, [data-ix='42']"))).must_equal ["P42"]
    _(@doc.css(".CAF\u00c9, .a, #p42, [data-ix='missing']").to_a).must_equal []
  end

  # Only NOKOLEXBOR_SELECTOR_CHECK builds expose the index build counter.
  if defined?(Nokolexbor::SELECTOR_CHECK)
    it "reuses the document index until a mutation or NodeSet reparenting" do
      @doc.css(@selector)
      _(@doc.selector_index_builds).must_equal 1
      @doc.css(".target, #P42, [data-ix]")
      @doc.at_css("article").css(".child, .target, [data-ix]")
      _(@doc.selector_index_builds).must_equal 1

      @doc.at_css("#P42")["class"] = "changed"
      @doc.css(".target, #P42, [data-ix]")
      _(@doc.selector_index_builds).must_equal 2

      @doc.css("article").css(".child, .target, [data-ix]")
      @doc.css(@selector)
      _(@doc.selector_index_builds).must_equal 3
    end
  end
end
