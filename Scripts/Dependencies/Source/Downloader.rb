require_relative 'Logger'
require_relative 'SystemDetector'
require_relative 'Dependency'

require 'net/http'
require 'uri'
require 'digest'


module DependencyInstaller


  class Downloader

    MAX_RETRIES = 3
    RETRY_DELAY = 2
    MAX_REDIRECTS = 5
    USER_AGENT = 'LumenEngine-DependencyInstaller/1.0'


    def initialize(max_concurrent: 4)
      @max_concurrent = max_concurrent
    end


    def download(dependency, destination_path)
      platform = SystemDetector.detect_platform
      url = dependency.get_url_for_platform(platform)

      unless url
        Logger.error("No URL found for #{dependency.name} on platform #{platform}")
        return false
      end

      filename = dependency.filename
      file_path = File.join(destination_path, filename)

      if File.exist?(file_path)
        Logger.info("File already exists: #{filename}")
        return validate_checksum(file_path, dependency.checksum)
      end

      MAX_RETRIES.times { |attempt|
        begin
          download_file(url, file_path, filename)
          
          if validate_checksum(file_path, dependency.checksum)
            Logger.clear_line
            Logger.success("Downloaded #{filename}")
            return true
          else
            File.delete(file_path) if File.exist?(file_path)
            Logger.error("Checksum validation failed for #{filename}")
            return false
          end
        rescue StandardError => e
          Logger.clear_line
          Logger.warning("Download attempt #{attempt + 1} failed for #{filename}: #{e.message}")
          sleep(RETRY_DELAY) if attempt < MAX_RETRIES - 1
        end
      }

      Logger.error("Failed to download #{dependency.name} after #{MAX_RETRIES} attempts")
      false
    end


    def download_all(dependencies, destination_path)
      queue = Queue.new
      dependencies.each { |dep| queue << dep }

      threads = Array.new(@max_concurrent) {
        Thread.new {
          until queue.empty?
            begin
              dependency = queue.pop(true)
              download(dependency, destination_path)
            rescue ThreadError
              break
            end
          end
        }
      }

      threads.each(&:join)
    end


    private


    def download_file(url, destination, filename)
      uri = URI.parse(url)
      redirect_count = 0

      loop {
        Net::HTTP.start(uri.host, uri.port, use_ssl: uri.scheme == 'https') { |http|
          request = Net::HTTP::Get.new(uri.request_uri)
          request['User-Agent'] = USER_AGENT
          
          http.request(request) { |response|
            case response
            when Net::HTTPSuccess
              request_success(response, destination, filename)
              return
            when Net::HTTPRedirection
              uri = request_redirect(response, redirect_count)
              redirect_count += 1
            else
              request_failure(response)
            end
          }
        }
      }
    end


    def request_success(response, destination, filename)
      total_size = response['content-length'].to_i
      downloaded = 0

      File.open(destination, 'wb') { |file|
        response.read_body { |chunk|
          file.write(chunk)
          downloaded += chunk.size
          Logger.render_progress_bar(filename, downloaded, total_size) if total_size > 0
        }
      }
    end


    def request_redirect(response, redirect_count)
      raise "Too many redirects" if redirect_count >= MAX_REDIRECTS
      
      new_uri = URI.parse(response['location'])
      new_uri.host ? new_uri : URI.parse("#{uri.scheme}://#{uri.host}#{new_uri}")
    end


    def request_failure(response)
      raise "HTTP Error: #{response.code}"
    end


    def validate_checksum(file_path, expected_checksum)
      return true unless expected_checksum

      actual_checksum = Digest::SHA256.file(file_path).hexdigest
      
      if actual_checksum == expected_checksum
        true
      else
        Logger.error("Checksum mismatch: expected #{expected_checksum}, got #{actual_checksum}")
        false
      end
    end

  end


end
