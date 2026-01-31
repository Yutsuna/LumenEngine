require 'rbconfig'

module DependencyInstaller


  class SystemDetector

    def self.detect_os
      case RbConfig::CONFIG['host_os']
      when /mswin|msys|mingw|cygwin|bccwin|wince|emc/
        :windows
      when /darwin|mac os/
        :macos
      when /linux/
        :linux
      else
        :unknown
      end
    end

    def self.detect_architecture
      arch = RbConfig::CONFIG['host_cpu']
      case arch
      when /x86_64|x64|amd64/
        :x64
      when /arm64|aarch64/
        :arm64
      when /i386|i686/
        :x86
      else
        arch.to_sym
      end
    end

    def self.detect_platform
      "#{detect_os}-#{detect_architecture}"
    end

  end


end
