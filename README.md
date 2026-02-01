# myBuild

**Disclaimer: This project is in an early alpha stage and is not production-ready. Use with caution.**

myBuild is an experimental build system and package manager for C/C++ projects. It aims to simplify the development workflow by managing dependencies directly through Git and automating the compilation process via a single JSON configuration file.

**NOTE:** Recipes of the dependencies are/will be updated in [myBuild Cookbook](https://github.com/mainak55512/myBuild-cookbook)

## Project Structure

myBuild expects a specific directory layout to function correctly:

* **src/**: All local project source files (.c, .cpp).
* **include/**: Local header files (.h, .hpp).
* **deps/**: External libraries (managed by myBuild).
* **myBuild.json**: The project manifest.

## How it Works

Currently, myBuild uses a "manifest-first" approach:

1. **Git Integration**: When a library is added, myBuild clones the repository into the `deps/` directory.
2. **Strict Manifest Requirement**: For a dependency to be compatible, it **must** contain its own `myBuild.json` file. myBuild reads this file to understand which directories to include and compile.
3. **Compilation**: The tool aggregates all source files and include paths from the main project and all dependencies to trigger the local compiler.

## myBuild.json Structure

```json
{
    "project_name": "example_project",
    "project_language": "c",
    "version": "0.1.0",
    "compiler_path": "/usr/bin/gcc",
    "include_paths": ["include"],
    "src": ["src"],
    "dependencies": {
        "example_lib": {
            "version": "1.0.0",
            "include_paths": ["include"],
            "src": ["lib"],
            "remote": "https://github.com/user/example_lib"
        }
    }
}

```

## Current Limitations

As this is an early development prototype, please be aware of the following:

* **No Build Flags**: Custom compiler flags (e.g., -O3, -Wall) are not yet supported.
* **Naming Conflicts**: There is currently no resolution logic for dependencies that share the same directory or project names.
* **Strict Compatibility**: Only repositories containing a `myBuild.json` file can be added as dependencies at this time.
* **No Incremental Builds**: The system currently performs full builds.

## Build
Clone the repo
```bash
git clone https://github.com/mainak55512/myBuild
cd myBuild
```
build with gcc or clang
```bash
gcc @build.rsp
```
or
```bash
clang @build.rsp
```

## Usage

### Initialize a Project

```bash
myBuild init

```

### Add a Dependency

(The remote repository must contain a `myBuild.json` file)

```bash
myBuild add <git_remote_url>

```
or
Add the recipe in the `dependencies` section in myBuild.json and run
```bash
myBuild sync
```
**N.B.** recipes are/will be available in [myBuild Cookbook](https://github.com/mainak55512/myBuild-cookbook)

### Build

```bash
myBuild build

```
or directly run with
```bash
myBuild run
```
