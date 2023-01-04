# frozen_string_literal: true

module Nokolexbor
  class Document < Nokolexbor::Node
    def create_element(name, *contents_or_attrs, &block)
      elm = Nokolexbor::Element.new(name, self, &block)
      contents_or_attrs.each do |arg|
        case arg
        when Hash
          arg.each do |k, v|
            elm[k.to_s] = v.to_s
          end
        else
          elm.content = arg
        end
      end
      elm
    end

    # Create a Text Node with +string+
    def create_text_node(string, &block)
      Nokolexbor::Text.new(string.to_s, self, &block)
    end

    # Create a CDATA Node containing +string+
    def create_cdata(string, &block)
      Nokolexbor::CDATA.new(string.to_s, self, &block)
    end

    # Create a Comment Node containing +string+
    def create_comment(string, &block)
      Nokolexbor::Comment.new(string.to_s, self, &block)
    end

    # A reference to +self+
    def document
      self
    end

    def meta_encoding
      if (meta = at_css("meta[charset]"))
        meta[:charset]
      elsif (meta = meta_content_type)
        meta["content"][/charset\s*=\s*([\w-]+)/i, 1]
      end
    end

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

    private

    IMPLIED_XPATH_CONTEXTS = ["//"].freeze # :nodoc:
  end
end