# frozen_string_literal: true


module DependencyInstaller

  class Logger

    COLORS = {
      red: "\e[31m",
      green: "\e[32m",
      yellow: "\e[33m",
      blue: "\e[34m",
      magenta: "\e[35m",
      cyan: "\e[36m",
      reset: "\e[0m"
    }.freeze

    def self.info(message)
      puts "#{COLORS[:blue]}[INFO]#{COLORS[:reset]} #{message}"
    end

    def self.success(message)
      puts "#{COLORS[:green]}[SUCCESS]#{COLORS[:reset]} #{message}"
    end

    def self.warning(message)
      puts "#{COLORS[:yellow]}[WARNING]#{COLORS[:reset]} #{message}"
    end

    def self.error(message)
      puts "#{COLORS[:red]}[ERROR]#{COLORS[:reset]} #{message}"
    end

    def self.progress(message)
      puts "#{COLORS[:cyan]}[PROGRESS]#{COLORS[:reset]} #{message}"
    end

  end


end
