require 'spec_helper'

describe "Multi threading" do
  # Correctness guard: verifies the fixed code produces correct results under
  # concurrent use. Does NOT reliably reproduce the original data race on MRI
  # Ruby (the GVL serialises C extension calls, making the race window too
  # narrow). For definitive race proof see test/tsan_race_test.c.
  it 'should be thread-safe' do
    1000.times.map do
      Thread.new do
        random_str = (0...32).map { (65 + rand(58)).chr }.join
        doc = Nokolexbor::HTML('<div><a class="a">' + random_str + '</a></div>')
        _(doc.css("div ::text").text).must_equal random_str
      end
    end.each(&:join)
  end

  # Correctness guard: verifies correct results with diverse selectors under
  # high concurrency. Like the test above, does not reliably reproduce the
  # original static-singleton data race on MRI Ruby due to GVL serialisation.
  it 'returns correct results under concurrent access with diverse selectors' do
    num_threads = 100
    iterations = 50
    errors = Queue.new
    barrier = Queue.new

    workers = num_threads.times.map do |i|
      Thread.new do
        barrier.pop
        iterations.times do |j|
          html = "<div id='t#{i}'><span class='c#{i}'>text-#{i}-#{j}</span><a href='#'>link-#{i}</a></div>"
          doc = Nokolexbor::HTML(html)

          result = doc.css(".c#{i}")
          unless result.length == 1 && result.text == "text-#{i}-#{j}"
            errors << "Thread #{i} iter #{j}: css got '#{result.text}' expected 'text-#{i}-#{j}'"
          end

          result = doc.at_css("#t#{i}")
          unless result && result.name == 'div'
            errors << "Thread #{i} iter #{j}: at_css failed"
          end

          result = doc.css(".c#{i}, a")
          unless result.length == 2
            errors << "Thread #{i} iter #{j}: css comma got #{result.length} nodes, expected 2"
          end
        end
      end
    end

    num_threads.times { barrier << true }
    workers.each(&:join)

    collected = []
    collected << errors.pop until errors.empty?
    assert_equal 0, collected.size, collected.first(5).join("\n")
  end

  it 'does not retain error state from previous parse failures' do
    doc = Nokolexbor::HTML('<div><span class="valid">text</span></div>')
    invalid_selectors = [
      'div:invalid-pseudo-class-that-does-not-exist',
      '::text1',
      ':not()',
      '[attr=',
      'div >>',
    ]

    # Correctness guard: verifies error-recovery behaviour is correct.
    # Runs sequentially — does not reproduce the multi-threaded race.
    # The fix (per-call allocation) makes cross-call state leakage structurally
    # impossible, so this test remains valid as a regression guard.
    1000.times do |i|
      sel = invalid_selectors[i % invalid_selectors.length]

      _{ doc.css(sel) }.must_raise Nokolexbor::Lexbor::UnexpectedDataError
      result = doc.css('span.valid')
      _(result.length).must_equal 1, "Iteration #{i}: expected 1 node after error with '#{sel}', got #{result.length}"
      _(result.text).must_equal 'text', "Iteration #{i}: expected 'text' after error with '#{sel}', got '#{result.text}'"
    end
  end
end