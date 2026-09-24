# frozen_string_literal: true
require 'fileutils'

module YARD
  module Server
    # Implements static caching for requests.
    #
    # @see Router Router documentation for "Caching"
    module StaticCaching
      # Called by a router to return the cached object. By default, this
      # method performs disk-based caching. To perform other forms of caching,
      # implement your own +#check_static_cache+ method and mix the module into
      # the Router class.
      #
      # This method checks for the existence of cached data. To actually cache
      # a response, see {#cache}.
      #
      # @example Implementing In-Memory Cache Checking
      #   module MemoryCaching
      #     def check_static_cache
      #       # $memory_cache is filled by {Commands::Base#cache}
      #       cached_data = $memory_cache[request.path]
      #       if cached_data
      #         [200, {'Content-Type' => 'text/html'}, [cached_data]]
      #       else
      #         nil
      #       end
      #     end
      #   end
      #
      #   class YARD::Server::Router; include MemoryCaching; end
      # @return [Array(Numeric,Hash,Array)] the Rack-style response
      # @return [nil] if no cache is available and routing should continue
      # @see Commands::Base#cache
      def check_static_cache
        return nil unless adapter.document_root
        cache_path = cache_path(request.path)
        return nil unless cache_path

        if File.file?(cache_path)
          log.debug "Loading cache from disk: #{cache_path}"
          return [200, {'Content-Type' => 'text/html'}, [File.read_binary(cache_path)]]
        end
        nil
      end

      # Caches rendered HTML response data to disk.
      #
      # @param [String] data the data to cache
      # @return [void]
      # @since 0.9.44
      def cache(data)
        return unless adapter.document_root

        path = cache_path(request.path_info)
        return unless path

        FileUtils.mkdir_p(File.dirname(path))
        log.debug "Caching data to #{path}"
        File.open(path, 'wb') {|f| f.write(data) }
      end

      private

      def cache_path(request_path)
        return nil if request_path.split(/[\/\\]/).include?('..')

        path = request_path.sub(/\.html$/, '') + '.html'
        path = path.sub(%r{\A/+}, '')
        return nil if path =~ /\A[A-Za-z]:/

        path = File.cleanpath(path)
        File.join(adapter.document_root, path)
      end
    end
  end
end
