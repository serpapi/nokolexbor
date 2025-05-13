# frozen_string_literal: true

module Nokolexbor
  class Document < Nokolexbor::Node
    # Create an {Element} with +name+ belonging to this document, optionally setting contents or
    # attributes.
    #
    # @param name [String]
    # @param contents_or_attrs [#to_s, Hash]
    #
    # @return [Element]
    #
    # @example An empty element without attributes
    #   doc.create_element("div")
    #   # => <div></div>
    #
    # @example An element with contents
    #   doc.create_element("div", "contents")
    #   # => <div>contents</div>
    #
    # @example An element with attributes
    #   doc.create_element("div", {"class" => "container"})
    #   # => <div class='container'></div>
    #
    # @example An element with contents and attributes
    #   doc.create_element("div", "contents", {"class" => "container"})
    #   # => <div class='container'>contents</div>
    #
    # @example Passing a block to mutate the element
    #   doc.create_element("div") { |node| node["class"] = "blue" }
    #   # => <div class='blue'></div>
    def create_element(name, *contents_or_attrs, &block)
      elm = Nokolexbor::Element.new(name, self, &block)
      contents_or_attrs.each do |arg|
        case arg
        when Hash
          arg.each do |k, v|
            elm[k.to_s] = v.to_s
          end
        else
          elm.content = arg.to_s
        end
      end
      elm
    end

    # Create a {Text} with +string+.
    #
    # @return [Text]
    def create_text_node(string, &block)
      Nokolexbor::Text.new(string.to_s, self, &block)
    end

    # Create a {CDATA} containing +string+.
    #
    # @return [CDATA]
    def create_cdata(string, &block)
      Nokolexbor::CDATA.new(string.to_s, self, &block)
    end

    # Create a {Comment} containing +string+.
    #
    # @return [Comment]
    def create_comment(string, &block)
      Nokolexbor::Comment.new(string.to_s, self, &block)
    end

    # A reference to +self+.
    #
    # @return [Document]
    def document
      self
    end

    # Get the meta tag encoding for this document. If there is no meta tag, nil is returned.
    #
    # @return [String]
    def meta_encoding
      if (meta = at_css("meta[charset]"))
        meta[:charset]
      elsif (meta = meta_content_type)
        meta["content"][/charset\s*=\s*([\w-]+)/i, 1]
      end
    end

    # Set the meta tag encoding for this document.
    #
    # If an meta encoding tag is already present, its content is
    # replaced with the given text.
    #
    # Otherwise, this method tries to create one at an appropriate
    # place supplying head and/or html elements as necessary, which
    # is inside a head element if any, and before any text node or
    # content element (typically <body>) if any.
    def meta_encoding=(encoding)
      if (meta = meta_content_type)
        meta["content"] = format("text/html; charset=%s", encoding)
        encoding
      elsif (meta = at_css("meta[charset]"))
        meta["charset"] = encoding
      else
        meta = Nokolexbor::Node.new("meta", self)
        meta["charset"] = encoding

        if (head = at_css("head"))
          head.prepend_child(meta)
        else
          set_metadata_element(meta)
        end
        encoding
      end
    end

    def meta_content_type
      xpath("//meta[@http-equiv and boolean(@content)]").find do |node|
        node["http-equiv"] =~ /\AContent-Type\z/i
      end
    end
    private :meta_content_type

    def set_metadata_element(element)
      if (head = at_css("head"))
        head << element
      elsif (html = at_css("html"))
        head = html.prepend_child(Nokolexbor::Node.new("head", self))
        head.prepend_child(element)
      elsif (first = children.find do |node|
               case node
               when Nokolexbor::Node
                 true
               end
             end)
        # We reach here only if the underlying document model
        # allows <html>/<head> elements to be omitted and does not
        # automatically supply them.
        first.add_previous_sibling(element)
      else
        html = add_child(Nokolexbor::Node.new("html", self))
        head = html.add_child(Nokolexbor::Node.new("head", self))
        head.prepend_child(element)
      end
    end

    # Parse HTML into a {Document}.
    #
    # @param string_or_io [String, #read]
    #   The HTML to be parsed. It may be a String, or any object that
    #   responds to #read such as an IO, or StringIO.
    #
    # @return [Document]
    def self.parse(string_or_io)
      html = string_or_io
      if string_or_io.respond_to?(:read)
        html = string_or_io.read
      end

      if html.respond_to?(:encoding) && html.encoding != Encoding::UTF_8
        html = html.encode(Encoding::UTF_8, invalid: :replace, undef: :replace)
      end

      parse_native(html)
    end

    private

    IMPLIED_XPATH_CONTEXTS = ["//"].freeze # :nodoc:
  end
end