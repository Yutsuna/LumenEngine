#!/usr/bin/env ruby
# frozen_string_literal: true

require_relative 'Source/Manager'


WINDOWS_CMAKE_BUILD_PARALLEL = [
  'cmake -S . -B build ' \
  '-G Ninja ' \
  '-DCMAKE_BUILD_TYPE=Release ' \
  '-DCMAKE_INSTALL_PREFIX=. ' \
  '-DCMAKE_C_COMPILER=clang-cl ' \
  '-DCMAKE_CXX_COMPILER=clang-cl',
  'cmake --build build --parallel',
  'cmake --install build'
].freeze


LINUX_CMAKE_BUILD_PARALLEL = [
    'cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=. -DCMAKE_INSTALL_LIBDIR=lib',
    'cmake --build build --parallel',
    'cmake --install build'
].freeze


MACOS_CMAKE_BUILD_PARALLEL = [
    'cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=. -DCMAKE_INSTALL_LIBDIR=lib',
    'cmake --build build --parallel',
    'cmake --install build'
].freeze


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
      'windows-x64':        WINDOWS_CMAKE_BUILD_PARALLEL,
      'windows-arm64':      WINDOWS_CMAKE_BUILD_PARALLEL,
      'linux-x64':          LINUX_CMAKE_BUILD_PARALLEL,
      'macos-x64':          MACOS_CMAKE_BUILD_PARALLEL,
      'macos-arm64':        MACOS_CMAKE_BUILD_PARALLEL
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