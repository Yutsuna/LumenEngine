require 'uri'
require_relative 'SystemDetector'


module DependencyInstaller


  class Dependency
    attr_reader :name, :version, :sources, :checksum

    def initialize(name:, version:, sources:, checksum: nil)
      @name = name
      @version = version
      @sources = sources
      @checksum = checksum
    end

    def get_url_for_platform(platform)
      @sources[platform.to_s] || @sources[platform.to_sym]
    end

    def filename
      url = get_url_for_platform(SystemDetector.detect_platform)
      return nil unless url
      File.basename(URI.parse(url).path)
    end

  end


end
