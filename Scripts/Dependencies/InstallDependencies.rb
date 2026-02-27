#!/usr/bin/env ruby
# frozen_string_literal: true

require_relative 'Source/Manager'


LINUX_CMAKE_BUILD_PARALLEL =[
    'cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=. -DCMAKE_INSTALL_LIBDIR=lib',
    'cmake --build build --parallel',
    'cmake --install build'
].freeze


DEPENDENCIES =[
  {
    name: 'SDL3',
    version: '3.4.0',
    sources: {
      'linux-x64': 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0.tar.gz',
    },
    build_commands: {
      'linux-x64':          LINUX_CMAKE_BUILD_PARALLEL,
    }
  },
  {
    name: 'vulkan',
    version: '1.4.344',
    sources: {
      'linux-x64': 'https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.4.344.tar.gz'
    },
    build_commands: {
      'linux-x64': LINUX_CMAKE_BUILD_PARALLEL
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
