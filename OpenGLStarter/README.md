# OpenGL Starter

[![Build](https://github.com/momotheoxy/OpenGLStarter/actions/workflows/build.yml/badge.svg)](https://github.com/momotheoxy/OpenGLStarter/actions/workflows/build.yml)

A small **C++17 + OpenGL 3.3** starter project for students using **Windows, macOS, or Linux**.

The project uses:

- **GLFW** — window creation, OpenGL context, input and events
- **GLEW** — OpenGL function loading
- **GLM** — vectors, matrices and other graphics mathematics
- **CMake** — cross-platform project configuration and build
- **vcpkg** — automatic C++ dependency installation
- **VS Code** — recommended editor/IDE for the lab

After a successful build, the program opens a window and draws a red triangle.

> You do **not** need to download GLEW, GLFW or GLM manually. vcpkg installs them during CMake configuration.

---

## 1. Install VS Code extensions

Open this repository in VS Code and install the recommended extensions when prompted:

- **C/C++** — Microsoft
- **CMake Tools** — Microsoft

You can also open **Extensions → Recommended** and install them there.

---

# Windows

## 2A. Install the compiler and tools

### Recommended: Visual Studio Community 2026

Install **Visual Studio Community 2026** and select the workload:

**Desktop development with C++**

Download Visual Studio from:

https://visualstudio.microsoft.com/downloads/

### Important: CMake 4.2+ is required for Visual Studio 2026

CMake added the **Visual Studio 18 2026** generator in **CMake 4.2**. If your CMake is older, the VS2026 preset cannot work.

Open **PowerShell** and install/update Git and CMake:

```powershell
winget install --id Git.Git -e
winget install --id Kitware.CMake -e
```

Check the version:

```powershell
cmake --version
```

For the **Visual Studio 2026** preset, use **CMake 4.2 or newer**.

> Still using Visual Studio 2022? That is supported too. Choose the **Windows MSVC (Visual Studio 2022)** preset instead.

## 2B. Install vcpkg

In PowerShell:

```powershell
git clone https://github.com/microsoft/vcpkg $env:USERPROFILE\vcpkg
& $env:USERPROFILE\vcpkg\bootstrap-vcpkg.bat
setx VCPKG_ROOT "$env:USERPROFILE\vcpkg"
```

Then **close and reopen VS Code** so it can see the new `VCPKG_ROOT` environment variable.

Verify it in a new PowerShell window:

```powershell
$env:VCPKG_ROOT
Test-Path "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
```

The second command should print:

```text
True
```

---

# macOS

## 2A. Install command-line tools

```bash
xcode-select --install
```

## 2B. Install Homebrew tools

If Homebrew is not installed:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Then:

```bash
brew install git cmake ninja pkg-config
```

## 2C. Install vcpkg

```bash
git clone https://github.com/microsoft/vcpkg ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> ~/.zshrc
export VCPKG_ROOT="$HOME/vcpkg"
```

Restart VS Code after setting `VCPKG_ROOT`.

### Apple Silicon or Intel?

Check with:

```bash
uname -m
```

- `arm64` → choose **macOS Apple Silicon (arm64)**
- `x86_64` → choose **macOS Intel (x64)**

The project requests OpenGL **3.3 core**, which is supported by macOS. Apple has deprecated OpenGL, but macOS still provides OpenGL up to 4.1.

---

# Linux (Ubuntu / Debian)

## 2A. Install compiler, build tools and GLFW system dependencies

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  git \
  cmake \
  ninja-build \
  pkg-config \
  libxinerama-dev \
  libxcursor-dev \
  xorg-dev \
  libglu1-mesa-dev
```

The X11/Mesa packages are required because the vcpkg GLFW port still relies on several Linux system libraries.

## 2B. Install vcpkg

```bash
git clone https://github.com/microsoft/vcpkg ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> ~/.bashrc
export VCPKG_ROOT="$HOME/vcpkg"
```

Restart VS Code after setting `VCPKG_ROOT`.

---

# 3. Get the project

```bash
git clone https://github.com/momotheoxy/OpenGLStarter.git
cd OpenGLStarter
```

Open **this folder itself** in VS Code — the folder containing `CMakeLists.txt`, `CMakePresets.json` and `vcpkg.json`.

---

# 4. Configure

In VS Code:

1. Press **Ctrl+Shift+P** (Windows/Linux) or **Cmd+Shift+P** (macOS).
2. Run **CMake: Select Configure Preset**.
3. Select the preset matching your computer:

| Platform | Preset |
|---|---|
| Windows + Visual Studio 2026 | `Windows MSVC (Visual Studio 2026)` |
| Windows + Visual Studio 2022 | `Windows MSVC (Visual Studio 2022)` |
| Apple Silicon Mac | `macOS Apple Silicon (arm64)` |
| Intel Mac | `macOS Intel (x64)` |
| 64-bit Linux | `Linux x64` |

CMake will configure the project and vcpkg will automatically download/build the required C++ libraries on the first run.

The **first configuration/build can take several minutes**. Later builds are much faster.

---

# 5. Build and run

## Build

Press:

**F7**

or run:

**CMake: Build**

from the Command Palette.

## Run / debug

After the build succeeds, press:

**F5**

Choose the matching debug configuration if VS Code asks:

- Windows → `Debug (Windows - MSVC)`
- macOS → `Debug (macOS - LLDB)`
- Linux → `Debug (Linux - GDB)`

You should see a black window with a **red triangle**.

---

# Folder structure

```text
OpenGLStarter/
├── .vscode/
│   ├── extensions.json
│   ├── launch.json
│   └── settings.json
├── Shaders/
│   ├── shader.frag
│   └── shader.vert
├── src/
│   ├── Libs/
│   │   ├── Mesh.cpp
│   │   ├── Mesh.h
│   │   ├── Shader.cpp
│   │   ├── Shader.h
│   │   ├── Window.cpp
│   │   └── Window.h
│   ├── ProjectPaths.h.in
│   └── main.cpp
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
└── README.md
```

Each configure preset has its own build directory under `build/`, so changing from one preset to another does not reuse an incompatible CMake cache.

---

# Troubleshooting

## `VCPKG_ROOT` is empty

Check:

### Windows PowerShell

```powershell
$env:VCPKG_ROOT
```

### macOS / Linux

```bash
echo $VCPKG_ROOT
```

If it is empty, set it again and **restart VS Code**.

---

## `Could not find toolchain file ... vcpkg.cmake`

Confirm that this file exists:

```text
<VCPKG_ROOT>/scripts/buildsystems/vcpkg.cmake
```

Then run:

**CMake: Delete Cache and Reconfigure**

---

## Visual Studio 2026 preset is missing or unsupported

Run:

```powershell
cmake --version
```

Visual Studio 2026 support requires **CMake 4.2+**.

Also confirm that **Visual Studio Community 2026 → Desktop development with C++** is installed.

---

## CMake selected MinGW on Windows

This starter intentionally uses **MSVC**, not MinGW.

Run:

**CMake: Select Configure Preset**

and select one of the Windows MSVC presets. If necessary, use:

**CMake: Delete Cache and Reconfigure**

---

## Linux: GLFW fails while configuring

Make sure the Linux system dependencies were installed:

```bash
sudo apt install -y libxinerama-dev libxcursor-dev xorg-dev libglu1-mesa-dev pkg-config
```

Then delete the failed build directory or run **CMake: Delete Cache and Reconfigure**.

---

## Shader files cannot be found

The starter does **not** depend on the program's current working directory for shader files. During CMake configuration, `src/ProjectPaths.h.in` is used to generate a header containing the absolute path to this clone's `Shaders` folder. The program then opens `shader.vert` and `shader.frag` from that folder using `std::filesystem`.

Students should edit only the shader files in the repository's top-level `Shaders/` folder. There is no second copied shader folder beside the executable.

If the repository is moved or renamed after CMake has already been configured, run **CMake: Delete Cache and Reconfigure** so the generated path is refreshed. If a shader still cannot be opened, the program prints the exact path it attempted to read.

---

## Project is inside OneDrive or a path with unusual characters

If Windows produces unexplained build/file-lock problems, move the repository to a short local path such as:

```text
C:\Dev\OpenGLStarter
```

This is not an OpenGL requirement, but it avoids several common beginner setup problems.

---

# Dependency versions

The dependencies are declared in `vcpkg.json`, so students normally do not need to manage versions manually.

As of **August 2026**, upstream releases include:

- GLEW **2.3.1**
- GLFW **3.5.1**
- GLM **1.0.3**

The exact version installed by vcpkg depends on the vcpkg registry revision being used.

---

# Notes for the Computer Graphics lab

This repository intentionally keeps the starter code small. The first program contains three helper classes:

- `Window` — creates and owns the GLFW/OpenGL window and context
- `Shader` — reads, compiles and links GLSL shaders
- `Mesh` — creates and renders a VAO/VBO/index buffer mesh

The starter shader renders one red triangle. Later lab work can extend this code with transformations, uniforms, vertex colours, indexed drawing, textures, cameras and lighting.

---

# Credits

This project uses open-source software including **GLEW**, **GLFW**, **GLM**, **CMake** and **vcpkg**.

Created for educational use in Computer Graphics teaching.

---

# Automatic build check (GitHub Actions)

Every push to `main` and every pull request is automatically **compiled** on:

- Windows Server 2025 with **Visual Studio 2026**
- macOS 26 on **Apple Silicon (arm64)**
- Ubuntu 24.04 on **x64 Linux**

The workflow is stored in `.github/workflows/build.yml`. It builds the program but does not try to open the OpenGL window, because GitHub's build runners are used only to verify that the code and dependencies compile successfully.

If the badge at the top of this README says **passing**, the current `main` branch has passed these cross-platform build checks.
