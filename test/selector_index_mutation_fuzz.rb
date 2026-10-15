require "json"
require "nokolexbor"

abort "build Nokolexbor with NOKOLEXBOR_SELECTOR_CHECK=1 (see README); it checks every search against the plain traversal" unless defined?(Nokolexbor::SELECTOR_CHECK)

selector_path = ENV.fetch("SELECTOR_CORPUS")
selectors = JSON.parse(File.read(selector_path))
abort "selector corpus must contain CSS strings" unless selectors.is_a?(Array) && !selectors.empty? && selectors.all? { |s| s.is_a?(String) }

target = Integer(ARGV.fetch(0, "100000"))
seed = Integer(ARGV.fetch(1, "20261004"))
abort "operation count must be positive" unless target > 0
random = Random.new(seed)
classes = selectors.flat_map { |selector| selector.scan(/\.([A-Za-z_][A-Za-z0-9_-]*)/).flatten }.uniq
classes -= %w[stable fuzz-item child]
abort "selector corpus contains no class tokens" if classes.empty?

def fragment(random, classes, serial)
  name = %w[div article aside section][random.rand(4)]
  token = classes[random.rand(classes.length)]
  "<#{name} id=\"fuzz-#{serial}\" class=\"fuzz-item #{token}\" data-fuzz=\"#{serial % 7}\"><span class=\"child\">#{serial}</span></#{name}>"
end

def new_document(random, classes)
  stable = 120.times.map do |i|
    first = classes[i % classes.length]
    second = classes[(i * 7 + 1) % classes.length]
    "<section id=\"stable-#{i}\" class=\"stable #{first}\" data-ved=\"#{i}\"><div class=\"#{second}\"><span>#{i}</span></div></section>"
  end.join
  changing = 6.times.map { |i| fragment(random, classes, i) }.join
  Nokolexbor::HTML("<main id=\"fuzz-root\">#{stable}<div id=\"fuzz-mutable\">#{changing}</div></main>")
end

def mutate(document, random, classes, serial)
  root = document.at_css("#fuzz-mutable")
  raise "lost the mutable root" unless root

  items = root.element_children.to_a
  if items.empty?
    root.add_child(fragment(random, classes, serial))
    return :add_child
  end

  node = items[random.rand(items.length)]
  markup = fragment(random, classes, serial)
  case random.rand(11)
  when 0
    root.inner_html = markup
    :inner_html
  when 1
    node.remove
    :remove
  when 2
    node.unlink
    :unlink
  when 3
    items.length >= 20 ? node.remove : root.add_child(markup)
    :add_child
  when 4
    items.length >= 20 ? node.remove : node.add_next_sibling(markup)
    :add_next_sibling
  when 5
    node.replace(markup)
    :replace
  when 6
    node["class"] = "fuzz-item #{classes[random.rand(classes.length)]}"
    node["data-attrid"] = serial.to_s
    :set_attribute
  when 7
    node.remove_attr(%w[class data-fuzz id data-attrid][random.rand(4)])
    :remove_attribute
  when 8
    node["data-fuzz"] = serial.to_s unless node.attribute("data-fuzz")
    attr = node.attribute("data-fuzz")
    random.rand(2).zero? ? attr.name = "data-alt-#{serial % 5}" : attr.value = serial.to_s
    :attribute_name_or_value
  when 9
    node.content = serial.to_s
    :content
  when 10
    node.destroy
    :destroy
  end
end

def result_for(scope, selector, method)
  scope.public_send(method, selector)
  nil
rescue Nokolexbor::Lexbor::UnexpectedDataError => error
  error.class
end

def query_scope(document, random)
  case random.rand(100)
  when 0..54
    document
  when 55..84
    document.at_css("#fuzz-mutable")
  when 85..97
    document.at_css("#stable-#{random.rand(120)}")
  when 98
    document.css("section.stable, .stable")
  else
    outer = document.at_css("#stable-#{random.rand(120)}")
    Nokolexbor::NodeSet.new(document, [outer, outer.at_css("div")])
  end
end

operations = 0
comparisons = 0
nodeset_comparisons = 0
overlap_comparisons = 0
sampled_selectors = Array.new(selectors.length, false)
mutations = Hash.new(0)
invalid = 0
stressed = 0
resets = 0
reported = 0
document = new_document(random, classes)

begin
  while operations < target
    GC.stress = operations % 2500 < 2
    stressed += 1 if GC.stress

    if random.rand(100) < 35
      kind = mutate(document, random, classes, operations)
      mutations[kind] += 1
      operations += 1
    else
      scope = query_scope(document, random)
      selector_index = random.rand(selectors.length)
      sampled_selectors[selector_index] = true
      nodeset_comparisons += 1 if scope.is_a?(Nokolexbor::NodeSet)
      overlap_comparisons += 1 if scope.is_a?(Nokolexbor::NodeSet) && scope.length == 2
      method = random.rand(2).zero? ? :css : :at_css
      # The check build aborts inside the call if the index and the plain traversal disagree.
      invalid += 1 if result_for(scope, selectors[selector_index], method)
      comparisons += 1
      operations += 2
    end

    if operations >= (resets + 1) * 5000
      document = nil
      scope = nil
      GC.stress = false
      GC.start
      document = new_document(random, classes)
      resets += 1
    end
    if operations / 10000 > reported
      reported = operations / 10000
      warn "operations=#{operations} comparisons=#{comparisons} mutations=#{mutations.values.sum} gc_stress=#{stressed}"
    end
  end
ensure
  GC.stress = false
end

puts JSON.generate(seed: seed, operations: operations, comparisons: comparisons,
                   nodeset_comparisons: nodeset_comparisons,
                   overlap_comparisons: overlap_comparisons,
                   mutations: mutations, invalid_selectors: invalid,
                   gc_stress_operations: stressed, document_resets: resets,
                   selector_count: selectors.length, sampled_selectors: sampled_selectors.count(true))
