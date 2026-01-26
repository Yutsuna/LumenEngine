require 'fileutils'

require_relative 'Logger'
require_relative 'SystemDetector'
require_relative 'Extractor'
require_relative 'BuildRunner'


module DependencyInstaller


  class Installer


    def initialize(install_path: 'Vendor')
      @install_path = install_path
      FileUtils.mkdir_p(@install_path)
      @extractor = Extractor.new
      @build_runner = BuildRunner.new
    end


    def needs_update?(dependency)
      dest_path = dest_for(dependency)
      version_file = version_file_for(dest_path)

      return true unless Dir.exist?(dest_path)
      return true unless File.exist?(version_file)

      installed_version = File.read(version_file).strip
      installed_version != dependency.version
    end


    def install(dependency, download_path)
      filename = dependency.filename
      file_path = File.join(download_path, filename)

      unless validate_downloaded_file(file_path)
        Logger.error("File not found: #{file_path}")
        return false
      end

      Logger.progress("Installing #{dependency.name}...")

      destination = dest_for(dependency)
      prepare_destination(destination)

      success = perform_installation(dependency, file_path, destination)

      if success
        write_version_file(destination, dependency.version)
        Logger.success("Installed #{dependency.name} (#{dependency.version}) to #{destination}")
        true
      else
        cleanup_on_failure(destination)
        false
      end
    end


    def install_all(dependencies, download_path)
      dependencies.each { |dependency|
        success = install(dependency, download_path)
        return false unless success
      }
      true
    end


    private


    def dest_for(dependency)
      File.join(@install_path, dependency.name)
    end


    def version_file_for(dest_path)
      File.join(dest_path, '.version')
    end


    def validate_downloaded_file(file_path)
      File.exist?(file_path)
    end


    def prepare_destination(destination)
      FileUtils.rm_rf(destination) if Dir.exist?(destination)
      FileUtils.mkdir_p(destination)
    end


    def perform_installation(dependency, archive_path, destination)
      if @extractor.extract(archive_path, destination)
        if @build_runner.run(dependency, destination)
          true
        else
          Logger.error("Failed to build #{dependency.name}")
          false
        end
      else
        Logger.error("Failed to install #{dependency.name}")
        false
      end
    end


    def write_version_file(destination, version)
      File.write(File.join(destination, '.version'), version)
    end


    def cleanup_on_failure(destination)
      FileUtils.rm_rf(destination)
    end


  end


end
