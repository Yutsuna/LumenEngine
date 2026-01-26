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
    BUFFER_SIZE = 8192

    def initialize(max_concurrent: 4)
      @max_concurrent = max_concurrent
      @mutex = Mutex.new
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

      Logger.progress("Downloading #{dependency.name} v#{dependency.version}...")

      MAX_RETRIES.times { |attempt|
        begin
          download_file(url, file_path)
          
          if validate_checksum(file_path, dependency.checksum)
            Logger.success("Downloaded #{filename}")
            return true
          else
            File.delete(file_path) if File.exist?(file_path)
            Logger.error("Checksum validation failed for #{filename}")
            return false
          end
        rescue StandardError => e
          Logger.warning("Download attempt #{attempt + 1} failed: #{e.message}")
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

    def download_file(url, destination)
      uri = URI.parse(url)
      max_redirects = 5
      redirect_count = 0

      loop {
        Net::HTTP.start(uri.host, uri.port, use_ssl: uri.scheme == 'https') { |http|
          request = Net::HTTP::Get.new(uri.request_uri)
          request['User-Agent'] = 'LumenEngine-DependencyInstaller/1.0'

          response = http.request(request)

          case response
          when Net::HTTPSuccess
            File.open(destination, 'wb') { |file|
              http.request(request) { |res|
                res.read_body { |chunk|
                  file.write(chunk)
                }
              }
            }
            return
          when Net::HTTPRedirection
            redirect_count += 1
            raise "Too many redirects" if redirect_count > max_redirects
            uri = URI.parse(response['location'])
          else
            raise "HTTP Error: #{response.code}"
          end
        }
      }
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
