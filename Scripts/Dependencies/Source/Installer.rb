require 'fileutils'
require_relative 'Logger'
require_relative 'SystemDetector'


module DependencyInstaller


  class Installer


    def initialize(install_path: 'Vendor')
      @install_path = install_path
      FileUtils.mkdir_p(@install_path)
    end


    def needs_update?(dependency)
      dest_path = File.join(@install_path, dependency.name)
      version_file = File.join(dest_path, '.version')

      return true unless Dir.exist?(dest_path)
      return true unless File.exist?(version_file)

      installed_version = File.read(version_file).strip
      
      return true unless installed_version == dependency.version

      false
    end


    def install(dependency, download_path)
      filename = dependency.filename
      file_path = File.join(download_path, filename)

      unless File.exist?(file_path)
        Logger.error("File not found: #{file_path}")
        return false
      end

      Logger.progress("Installing #{dependency.name}...")

      destination = File.join(@install_path, dependency.name)
      
      FileUtils.rm_rf(destination) if Dir.exist?(destination)
      FileUtils.mkdir_p(destination)

      success = false

      if extract_archive(file_path, destination)
        if run_build_commands(dependency, destination)
          success = true
        else
          Logger.error("Failed to build #{dependency.name}")
        end
      else
        Logger.error("Failed to install #{dependency.name}")
      end

      if success
        File.write(File.join(destination, '.version'), dependency.version)
        Logger.success("Installed #{dependency.name} (#{dependency.version}) to #{destination}")
        return true
      else
        FileUtils.rm_rf(destination)
        return false
      end
    end


    def install_all(dependencies, download_path)
      dependencies.each { |dependency|
        install(dependency, download_path)
      }
    end


    private


    def run_build_commands(dependency, directory)
      commands = dependency.get_build_commands_for_platform(SystemDetector.detect_platform)
      return true unless commands

      Logger.progress("Building #{dependency.name}...")

      Dir.chdir(directory) {
        commands.each { |cmd|
          Logger.info("Executing: #{cmd}")
          unless system(cmd)
            Logger.error("Command failed: #{cmd}")
            return false
          end
        }
      }
      true
    end


    def extract_archive(archive_path, destination)
      case File.extname(archive_path).downcase
      when '.zip'
        extract_zip(archive_path, destination)
      when '.gz', '.tgz'
        extract_tar_gz(archive_path, destination)
      when '.tar'
        extract_tar(archive_path, destination)
      else
        Logger.warning("Unknown archive format: #{archive_path}")
        false
      end
    end


    def extract_zip(archive_path, destination)
      temp_dir = "#{destination}_temp"
      FileUtils.mkdir_p(temp_dir)

      if system('unzip', '-q', '-o', archive_path, '-d', temp_dir)
        finalize_extraction(temp_dir, destination)
      else
        Logger.error("Failed to extract ZIP archive")
        FileUtils.rm_rf(temp_dir)
        false
      end
    end


    def extract_tar_gz(archive_path, destination)
      temp_dir = "#{destination}_temp"
      FileUtils.mkdir_p(temp_dir)

      if system('tar', '-xzf', archive_path, '-C', temp_dir)
        finalize_extraction(temp_dir, destination)
      else
        Logger.error("Failed to extract TAR.GZ archive")
        FileUtils.rm_rf(temp_dir)
        false
      end
    end


    def extract_tar(archive_path, destination)
      if system('tar', '-xf', archive_path, '-C', destination)
        true
      else
        Logger.error("Failed to extract TAR archive")
        false
      end
    end


    def finalize_extraction(temp_dir, destination)
      contents = Dir.entries(temp_dir).reject { |e| e == '.' || e == '..' }

      if contents.length == 1 && File.directory?(File.join(temp_dir, contents[0]))
        root_dir = File.join(temp_dir, contents[0])
        Dir.entries(root_dir).reject { |e| e == '.' || e == '..' }.each { |item|
          FileUtils.mv(File.join(root_dir, item), destination)
        }
      else
        contents.each { |item|
          FileUtils.mv(File.join(temp_dir, item), destination)
        }
      end

      FileUtils.rm_rf(temp_dir)
      true
    end


  end


end
