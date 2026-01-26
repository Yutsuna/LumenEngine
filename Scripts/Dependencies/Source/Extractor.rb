#frozen_string_literal: true

require 'fileutils'
require 'rubygems/package'
require 'zlib'


module DependencyInstaller


  class Extractor


    FORMATS = {
      '.zip' => :unpack_zip,
      '.gz'  => :unpack_tgz,
      '.tgz' => :unpack_tgz,
      '.tar' => :unpack_tar
    }.freeze


    def extract(archive_path, destination)
      method_name = FORMATS[File.extname(archive_path).downcase]

      if method_name
        in_temp_dir(destination) { |temp_dir|
          send(method_name, archive_path, temp_dir)
        }
      else
        Logger.warning("Unknown archive format: #{archive_path}")
        false
      end
    end


    private


    def in_temp_dir(destination)
      temp_dir = "#{destination}_temp_#{Time.now.to_i}"
      FileUtils.mkdir_p(temp_dir)

      if yield(temp_dir)
        finalize_extraction(temp_dir, destination)
      else
        FileUtils.rm_rf(temp_dir)
        false
      end
    end


    def unpack_zip(archive_path, temp_dir)
      cmd = zip_command(archive_path, temp_dir)

      if system(*cmd)
        true
      else
        Logger.error("Failed to extract ZIP archive")
        false
      end
    end


    def zip_command(path, dest)
      if Gem.win_platform?
        ['tar', '-xf', path, '-C', dest]
      else
        ['unzip', '-q', '-o', path, '-d', dest]
      end
    end


    def unpack_tgz(archive_path, temp_dir)
      File.open(archive_path, 'rb') { |file|
        Zlib::GzipReader.wrap(file) { |gz|
          unpack_tar_stream(gz, temp_dir)
        }
      }
    rescue => e
      log_error("TAR.GZ", e)
    end


    def unpack_tar(archive_path, temp_dir)
      File.open(archive_path, 'rb') { |file|
        unpack_tar_stream(file, temp_dir)
      }
    rescue => e
      log_error("TAR", e)
    end


    def unpack_tar_stream(io, destination)
      Gem::Package::TarReader.new(io) { |tar|
        tar.each { |entry|
          extract_entry(entry, destination)
        }
      }
      true
    end


    def extract_entry(entry, destination)
      dest_path = File.join(destination, entry.full_name)

      if entry.directory?
        FileUtils.mkdir_p(dest_path)
      elsif entry.file?
        FileUtils.mkdir_p(File.dirname(dest_path))
        File.open(dest_path, 'wb') { |f| f.write(entry.read) }
      end
    end


    def finalize_extraction(temp_dir, destination)
      contents = Dir.children(temp_dir)
      FileUtils.mkdir_p(destination)

      source = if contents.size == 1 && File.directory?(File.join(temp_dir, contents.first))
                 File.join(temp_dir, contents.first)
               else
                 temp_dir
               end

      FileUtils.cp_r(Dir.glob("#{source}/*"), destination)
      FileUtils.rm_rf(temp_dir)
      true
    end


    def log_error(type, error)
      Logger.error("Failed to extract #{type} archive: #{error.message}")
      false
    end


  end


end