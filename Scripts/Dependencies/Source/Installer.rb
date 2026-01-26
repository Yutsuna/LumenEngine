require 'fileutils'
require_relative 'Logger'


module DependencyInstaller

  class Installer

    def initialize(install_path: 'Vendor')
      @install_path = install_path
      FileUtils.mkdir_p(@install_path)
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

      if extract_archive(file_path, destination)
        Logger.success("Installed #{dependency.name} to #{destination}")
        true
      else
        Logger.error("Failed to install #{dependency.name}")
        false
      end
    end

    def install_all(dependencies, download_path)
      dependencies.each { |dependency|
        install(dependency, download_path)
      }
    end

    private

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
