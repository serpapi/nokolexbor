# frozen_string_literal: true
# Handles RBS include, extend, and prepend declarations.
class YARD::Handlers::RBS::MixinHandler < YARD::Handlers::RBS::Base
  handles :include, :extend, :prepend

  process do
    mixin = P(namespace, statement.mixin_name)
    case statement.type
    when :include
      mixins = namespace.mixins(:instance)
      mixins << mixin unless mixins.include?(mixin)
    when :extend
      mixins = namespace.mixins(:class)
      mixins << mixin unless mixins.include?(mixin)
    when :prepend
      mixins = namespace.mixins(:instance)
      mixins.unshift(mixin) unless mixins.include?(mixin)
    end
  end
end
