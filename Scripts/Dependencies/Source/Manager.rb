require 'fileutils'
require 'etc'

require_relative 'Logger'
require_relative 'Downloader'
require_relative 'Dependency'
require_relative 'SystemDetector'
require_relative 'Installer'


module DependencyInstaller


  class DependencyManager
    attr_reader :dependencies

    def initialize(dependencies:, install_path: 'Vendor', download_path: 'TempDownloads')
      @dependencies = dependencies.map { |dep| Dependency.new(**dep) }
      @install_path = install_path
      @download_path = download_path

      max_threads = [Etc.nprocessors, 4].min

      @downloader = Downloader.new(max_concurrent: max_threads)
      @installer = Installer.new(install_path: @install_path)
    end

    def execute
      Logger.info("Starting dependency installation...")
      Logger.info("Platform: #{SystemDetector.detect_platform}")
      Logger.info("Dependencies: #{@dependencies.size}")

      FileUtils.mkdir_p(@download_path)

      Logger.progress("Downloading dependencies...")
      @downloader.download_all(@dependencies, @download_path)

      Logger.progress("Installing dependencies...")
      @installer.install_all(@dependencies, @download_path)

      cleanup

      Logger.success("All dependencies processed!")
    end

    private

    def cleanup
      Logger.info("Cleaning up temporary files...")
      FileUtils.rm_rf(@download_path)
    end

  end


end
