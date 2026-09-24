# frozen_string_literal: true
# Handles RBS class, module, and interface declarations by registering
# the corresponding namespace code objects and recursing into their bodies.
class YARD::Handlers::RBS::NamespaceHandler < YARD::Handlers::RBS::Base
  handles :class, :module, :interface
  namespace_only

  process do
    name = statement.name
    type = statement.type

    obj = case type
          when :class
            klass = register ClassObject.new(namespace, name)
            if (sc = statement.superclass) && !sc.strip.empty?
              klass.superclass = P(namespace, sc)
              klass.superclass.type = :class if klass.superclass.is_a?(Proxy)
            end
            klass
          when :module, :interface
            register ModuleObject.new(namespace, name)
          end

    parse_block(:namespace => obj)
  end
end
