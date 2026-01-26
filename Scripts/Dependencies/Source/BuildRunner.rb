module DependencyInstaller


  class BuildRunner


    def initialize(platform = SystemDetector.detect_platform)
      @platform = platform
    end


    def run(dependency, directory)
      commands = dependency.get_build_commands_for_platform(@platform)
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

  end


end
