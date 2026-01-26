#!/usr/bin/env ruby
# frozen_string_literal: true

require_relative 'Source/Manager'


DEPENDENCIES = [
  {
    name: 'SDL3',
    version: '3.4.0',
    sources: {
      'windows-x64': 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0.tar.gz',
      'windows-arm64': 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0.tar.gz',
      'linux-x64': 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0.tar.gz',
      'macos-x64': 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0.tar.gz',
      'macos-arm64': 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0.tar.gz'
    },
    build_commands: {
      'windows-x64': [
        'cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=.',
        'cmake --build build --config Release --parallel',
        'cmake --install build'
      ],
      'windows-arm64': [
        'cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=.',
        'cmake --build build --config Release --parallel',
        'cmake --install build'
      ],
      'linux-x64': [
        'cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=. -DCMAKE_INSTALL_LIBDIR=lib',
        'cmake --build build --config Release --parallel',
        'cmake --install build'
      ],
      'macos-x64': [
        'cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=. -DCMAKE_INSTALL_LIBDIR=lib',
        'cmake --build build --config Release --parallel',
        'cmake --install build'
      ],
      'macos-arm64': [
        'cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=. -DCMAKE_INSTALL_LIBDIR=lib',
        'cmake --build build --config Release --parallel',
        'cmake --install build'
      ]
    }
  }
].freeze


if __FILE__ == $PROGRAM_NAME

  manager = DependencyInstaller::DependencyManager.new(
    dependencies: DEPENDENCIES,
    install_path: 'Vendor',
    download_path: 'TempDownloads'
  )

  manager.execute

end