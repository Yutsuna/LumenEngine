# frozen_string_literal: true

require 'thread'


module DependencyInstaller

  class Logger
    @ui_mutex = Mutex.new

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

    def self.render_progress_bar(filename, current, total)
      return if total.to_i <= 0
      
      @ui_mutex.synchronize {
        width = 30
        percentage = (current.to_f / total * 100).to_i
        filled = (width * current.to_f / total).to_i
        bar = "=" * filled + " " * (width - filled)
        
        print "\r#{COLORS[:cyan]}[DOWNLOADING]#{COLORS[:reset]} #{filename}: [#{bar}] #{percentage}%"
        $stdout.flush
      }
    end

    def self.clear_line
      print "\r\e[K"
    end

  end


end
