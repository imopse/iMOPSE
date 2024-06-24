---
layout: default
title: Compile Project
permalink: /docs/compile_project/
nav_order: 2
---

# Compile project
To start working with iMOPSE, clone the repository, it consists of two C++ projects `optimizer` and `paretoAnalyzer`, both of them contain CMakeLists.txt files.
In this section we present a few example methods to compile C++ project.
## using CMake
- Verify CMake and Make
```bash
cmake --version
make --version
```
- Verify if CMake and Make are installed on your system. If not, you will need to install them. Visit the [CMake Website](https://cmake.org) and the [GNU Make Manual](https://www.gnu.org/software/make/manual/make.html#Installing-Make) for installation instructions.
- Enter `optimizer` directory, create new `build` directory and enter it
```bash
cd optimizer
mkdir build
cd build
```
- Run CMake
```bash
cmake ..
```
- Compile the project
```bash
make
```
- Run executable
```bash
./imopse
```
Expected output should be: `Usage: <pathToExecutable> <MethodConfigPath> <ProblemName> <ProblemDefinitionPath> <OutputDirectory> [ExecutionsCount] [Seed]`

## using Clion IDE
- Open project in CLion.
- In one of project directories find `CMakeLists.txt`, right-click it and select `Load CMake Project` option.
- CLion will automatically build the project.
- After building the run button will be available in the top-right corner. Expected output should be: `Usage: <pathToExecutable> <MethodConfigPath> <ProblemName> <ProblemDefinitionPath> [ExecutionsCount] [Seed]`
