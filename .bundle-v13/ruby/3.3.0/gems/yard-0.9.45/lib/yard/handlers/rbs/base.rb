# frozen_string_literal: true
module YARD
  module Handlers
    # Handlers for RBS (Ruby type signature) files.
    module RBS
      # Base class for all RBS handlers.
      # Handlers match on the {Parser::RBS::Statement#type} symbol of the
      # current statement and process it to create or annotate code objects.
      class Base < Handlers::Base
        # @return [Boolean] whether this handler matches the given statement
        def self.handles?(statement, _processor)
          handlers.any? do |matcher|
            case matcher
            when Symbol
              statement.type == matcher
            when String
              statement.type.to_s == matcher
            when Regexp
              (statement.source || '') =~ matcher
            else
              false
            end
          end
        end

        # Recurse into the body of a namespace statement.
        # @param opts [Hash] state overrides
        # @see #push_state
        def parse_block(opts = {})
          return if statement.block.nil? || statement.block.empty?
          push_state(opts) do
            parser.process(statement.block)
          end
        end
      end
    end
  end
end
