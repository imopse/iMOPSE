---
layout: default
title: Compile Project
permalink: /docs/instructions/compile_project/
parent: Instructions
nav_order: 1
---

# Compile project

To start working with iMOPSE, clone the repository. It contains two C++ projects: `optimizer` and `paretoAnalyzer`. Both projects contain their own `CMakeLists.txt` files and should be compiled separately.

For working with iMOPSE, we recommend using an IDE such as [CLion](https://www.jetbrains.com/clion/) or [Visual Studio](https://visualstudio.microsoft.com/vs/). The project can also be built manually with CMake.

## using CLion IDE

CLion can be used on Linux, Windows, and macOS. For Windows, CLion includes a built-in C++ compiler.

For Linux and Mac CLion requires user to configure toolchain, for more information, see the official [CLion toolchain configuration guide](https://www.jetbrains.com/help/clion/how-to-create-toolchain-in-clion.html).

On Linux if a compiler or build tools are missing, install them first. For example, on Ubuntu or Debian-based systems:

```bash
sudo apt update
sudo apt install build-essential cmake
```
The `build-essential` package includes `g++`, `gcc`, `make`, and other common build tools.


To build the project in CLion:

- Open the repository in CLion.
- In one of the project directories, find the `CMakeLists.txt` file.
- Right-click `CMakeLists.txt` and select the `Load CMake Project` option.
- Make sure CLion has a valid toolchain configured in `Settings | Build, Execution, Deployment | Toolchains`.
- CLion will configure the project using CMake.
- After the project is built, the run button will be available in the top-right corner.

Expected output should be:

```text
Usage: <pathToExecutable> <MethodConfigPath> <ProblemName> <ProblemDefinitionPath> <OutputDirectory> [ExecutionsCount] [Seed]
```

## using Visual Studio

Visual Studio is available for Windows, to build C++ CMake projects in Visual Studio, make sure that Visual Studio is installed with C++ development tools. During installation, select the `Desktop development with C++` workload. This provides the required C++ compiler and CMake support.

For more information, see the official [Visual Studio C++ installation guide](https://learn.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170) and [CMake projects in Visual Studio](https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-170).

To build the project in Visual Studio:

- Open Visual Studio.
- Select `Open a local folder`.
- Open one of the project directories, such as `optimizer`, that contains a `CMakeLists.txt` file.
- Visual Studio should automatically detect the CMake project.
- Wait until CMake configuration is finished.
- Select the executable target, for example `imopse`.
- Choose the build configuration, such as `Debug` or `Release`.
- Build and run the project from Visual Studio.

Expected output should be:

```text
Usage: <pathToExecutable> <MethodConfigPath> <ProblemName> <ProblemDefinitionPath> <OutputDirectory> [ExecutionsCount] [Seed]
```

## using CMake manually

The project can also be built manually with CMake from the command line.

Before building, make sure that CMake and a C++17-compatible compiler are installed.

Check CMake and Make:

```bash
cmake --version
make --version
```

Check that a C++ compiler is available. For example, on Linux:

```bash
g++ --version
```

`g++` is one common C++ compiler, but it is not the only option. Other C++17-compatible compilers, such as `clang++` on Linux or MSVC on Windows, may also work.

### Linux

On Ubuntu or Debian-based systems, if the required tools are missing, install them with:

```bash
sudo apt update
sudo apt install build-essential cmake
```

Enter the `optimizer` directory:

```bash
cd optimizer
```

Configure the project in `Debug` mode:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```
or in `Release` mode:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Then build project:

```bash
cmake --build build
```

Run the executable:

```bash
./build/imopse
```

### Windows

On Windows, make sure that CMake and a C++ toolchain are installed. For example, you can use Visual Studio with C++ development tools, MinGW, or another C++17-compatible compiler.

Enter the `optimizer` directory:

```bash
cd optimizer
```

Configure the project:

```bash
cmake -S . -B build
```

Build the project in `Debug` mode:

```bash
cmake --build build --config Debug
```

Or build it in `Release` mode:

```bash
cmake --build build --config Release
```

The executable location depends on the selected CMake generator. For example, with Visual Studio generators, it may be created inside a configuration directory such as:

```bash
build/Release/imopse.exe
```

or:

```bash
build/Debug/imopse.exe
```

Expected output should be:

```text
Usage: <pathToExecutable> <MethodConfigPath> <ProblemName> <ProblemDefinitionPath> <OutputDirectory> [ExecutionsCount] [Seed]
```

To run imopse for specific method and problem see: [Example of use]({{ 'docs/instructions/example_of_use' | relative_url }}) and visit available [Configurations]({{ 'docs/configurations' | relative_url }}) page.