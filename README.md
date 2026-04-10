<h1 align="center">
  Lumen Engine<br>
  Highly Modular C++23 Game Engine
  <br>
  <img src="https://raw.githubusercontent.com/catppuccin/catppuccin/main/assets/palette/macchiato.png" width="600px"/>
  <br>
</h1>

<div align="center">
  <p></p>
  <div align="center">
     <a href="https://github.com/Leorevoir/LumenEngine/stargazers">
        <img src="https://img.shields.io/github/stars/Leorevoir/LumenEngine?color=F5BDE6&labelColor=303446&style=for-the-badge&logo=starship&logoColor=F5BDE6">
     </a>
     <a href="https://github.com/Leorevoir/LumenEngine/">
        <img src="https://img.shields.io/github/repo-size/Leorevoir/LumenEngine?color=C6A0F6&labelColor=303446&style=for-the-badge&logo=github&logoColor=C6A0F6">
     </a>
     <a href="https://github.com/Leorevoir/LumenEngine/blob/main/LICENSE">
        <img src="https://img.shields.io/static/v1.svg?style=for-the-badge&label=License&message=GPL3&colorA=313244&colorB=F5A97F&logo=unlicense&logoColor=F5A97F&"/>
     </a>
  </div>
  <br>
</div>

<p align="center">
An Actor-Model C++23 Game Engine built with Vulkan.
</p>

## About The Project

Lumen Engine is a modern modular game engine built from the ground up using C++23.
<br>
It focuses on high-concurrency architecture through the Actor-Model and provides a high-performance rendering path via a custom Vulkan Render Hardware Interface.

## Project Normes

The Project is entierly written in PascalCase.

It adheres to strict industrial coding standards inspired by the Epic Games and Unreal Engine conventions. 

## Technical Specifications

| Category | Specification |
| :--- | :--- |
| **Language Standard** | C++23 |
| **RHI** | Vulkan 1.3+ |
| **Windoing & Input** | SDL3 Abstraction Layer |
| **Memory Management** | Custom Smart Pointers |
| **Concurrency** | Lock-Free structures, Actor-Model, Triple-Buffering |
| **Build System** | CMake 3.25+, Ninja, Nix Flakes |
| **Platform Support** | Linux (For now) |

## Project Architecture

## Getting Started

### Prerequisites

The project uses [**Nix**](https://nixos.org/) for environment reproducibility.<br>
If you are on Linux with Nix installed:

```bash
nix develop path:.
```

Alternatively, ensure you have the following installed:

- CMake 3.25+
- A C++23 compliant compiler (GCC 13+ or Clang 16+)
- Vulkan SDK
- SDL3
- Ninja (Recommanded)

### Building the project

We recommand to use the [**Lumen Building Script**](./Scripts/LumenBuild.bash).

input:

```bash
lumen help
```

output:

```console
LumenEngine Build System

Usage: lumen [Command] [Mode]
       lumen [Mode]

Commands:
  build   <Mode>  Configure and compile the project (Default)
  run     <Mode>  Execute the BaseExample (build if needed)
  format          Run clang-format on LumenEngine/Source/**/*.{hpp,inl,cpp}
  clean           Remove all build artifacts
  rebuild <Mode>  Full cleanup followed by a fresh build

Modes:
  debug, release, relwithdebinfo
  - Default mode: debug
  - Inside nix shell (IN_NIX_SHELL): default mode is release
  - Shorthand: 'lumen release' is equivalent to 'lumen build release'
```

## License

This project is licensed under the GPL-3.0 LICENSE. See [**LICENSE**](./LICENSE) for details.
