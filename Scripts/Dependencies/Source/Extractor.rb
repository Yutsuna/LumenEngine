module DependencyInstaller


  class Extractor


    def extract(archive_path, destination)
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


    private


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
        Dir.entries(root_dir).reject { |e| e == '.' || e == '..' }.each do |item|
          FileUtils.mv(File.join(root_dir, item), destination)
        end
      else
        contents.each do |item|
          FileUtils.mv(File.join(temp_dir, item), destination)
        end
      end

      FileUtils.rm_rf(temp_dir)
      true
    end

  end


end
