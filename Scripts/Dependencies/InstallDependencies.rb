#!/usr/bin/env ruby
# frozen_string_literal: true

require_relative 'Source/Manager'


if __FILE__ == $PROGRAM_NAME
  dependencies = [
    {
      name: 'SDL3',
      version: '3.4.0',
      sources: {
        'windows-x64': 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0-win32-x64.zip',
        'windows-arm64': 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0-win32-arm64.zip',
        'linux-x64': 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0.tar.gz',
        'macos-x64': 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0.tar.gz',
        'macos-arm64': 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0.tar.gz'
      }
    }
  ]

  manager = DependencyInstaller::DependencyManager.new(
    dependencies: dependencies,
    install_path: 'Vendor',
    download_path: 'TempDownloads'
  )

  manager.execute

end
