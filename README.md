# Voxel-Forge

## **---Under Development---**

## **I welcome Contributors to work on this project! :)**


## About

Voxel-Forge is a Qt-based application designed to create 3D models from a series of input images using Structure-from-Motion (SfM) and Multi-View Stereo (MVS) techniques. It integrates the powerful [OpenMVG](https://github.com/openMVG/openMVG) and [OpenMVS](https://github.com/cdcseacave/openMVS) libraries to perform the photogrammetry reconstruction pipeline within a user-friendly Qt interface.

This version utilizes **library-based integration** via the [vcpkg](https://vcpkg.io/) C++ package manager for improved performance and a more streamlined build process. For executable based integration visit `legacy-executable` branch of this repository.

---

## Download the Software

* For `Linux` [https://example.com]
* For `Windows` (coming soon on demand) - ***open for your contribution to make a windows build!***

---

## Development Setup (Main Branch)

### Frameworks Used

* [`CMake`](https://cmake.org/) (v3.18+) as the build system generator.
* [`Qt5`](https://www.qt.io/) for the Graphical User Interface.
* [`vcpkg`](https://vcpkg.io/) as the C++ package manager to handle dependencies.
* [`OpenMVG`](https://github.com/openMVG/openMVG) library for Structure-from-Motion.
* [`OpenMVS`](https://github.com/cdcseacave/openMVS) library for Multi-View Stereo reconstruction.
* [`Docker`](https://www.docker.com/) - will be upgrading to docker soon!

---

## Contributing / Building from Source

### Prerequisites

1.  **C++ Compiler:** A C++17 compliant compiler (GCC, Clang, MSVC).
2.  **CMake:** Latest `stable` Version or use Version 4.1.2 (used by me while developing)
3.  **Git:** Required for cloning the repository and for vcpkg.
4.  **vcpkg:** The C++ package manager.
    * Clone and bootstrap vcpkg if you haven't already. Follow their [official getting started guide](https://github.com/microsoft/vcpkg#getting-started). Install it to a central location (e.g., `/home/user/vcpkg` or `C:\vcpkg`).
5. **vcpkg System Build Tools:** vcpkg requires certain system tools to build some libraries from source. Install the common ones for Debian/Ubuntu if not already installed:
    ```bash
    sudo apt update && sudo apt install \
    pkg-config \
    bison \
    autoconf \
    autoconf-archive \
    automake \
    libtool \
    linux-libc-dev \
    libxcb1-dev \
    libxcb-glx0-dev \
    libxcb-icccm4-dev \
    libxcb-image0-dev \
    libxcb-keysyms1-dev \
    libxcb-randr0-dev \
    libxcb-render-util0-dev \
    libxcb-render0-dev \
    libxcb-shape0-dev \
    libxcb-sync-dev \
    libxcb-util-dev \
    libxcb-xfixes0-dev \
    libxcb-xinerama0-dev \
    libxcb-xkb-dev \
    libxkbcommon-x11-dev \
    libx11-xcb-dev \
    libgl1-mesa-dev \
    libxrender-dev \
    libxi-dev \
    libxkbcommon-dev \
    xutils-dev \
    gfortran
    ```

* *Note: If vcpkg fails while building a specific library and explicitly asks for another system tool (like `flex`, `perl`, `nasm`), install it using your system's package manager (`apt`, `dnf`, `pacman`, `brew`, etc.) and ensure it's in your PATH.*

### Build Steps

1.  **Clone the Repository:**
    ```bash
    git clone <your-repo-url>
    cd Voxel-Forge
    # ensure you are on the main branch
    git checkout main
    ```

2.  **Configure with CMake:**
    * Create a build directory and navigate into it.
    * Run CMake, pointing it to your vcpkg installation's toolchain file. Replace `<path/to/vcpkg>` with the correct path.
    * vcpkg will automatically read the `vcpkg.json` file in the project root, then download and build all required dependencies (Qt, OpenMVG, OpenMVS, and their dependencies like Ceres, Eigen, Boost, etc.). **This initial step can take a significant amount of time (30-60+ minutes)!**
    ```bash
    mkdir build
    cd build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=<path/to/vcpkg>/scripts/buildsystems/vcpkg.cmake -DVCPKG_CLEAN_AFTER_BUILD=ON
    ```
    * ***Optional - Low Disk Space:*** If you encounter disk space issues during the long vcpkg build, you can try adding `-DVCPKG_CLEAN_AFTER_BUILD=ON` to the `cmake` command. This will remove temporary build files for each library after it's installed, reducing peak disk usage but potentially slowing down future partial rebuilds.

3.  **Compile the Project:**
    * Once CMake configuration is complete, build the Voxel-Forge application.
    ```bash
    cmake --build .
    ```

---

## Running

The executable `Voxel-Forge` will be located inside the `build` directory.

```bash
./Voxel-Forge
```


## Troubleshooting
### OpenMVS Floating-Point Assertions
- **Symptom**: In rare cases, particularly on certain Linux/GCC configurations, the reconstruction pipeline might crash during the dense reconstruction phase (DensifyPointCloud step internally) due to floating-point assertions within the OpenMVS library failing.

- **Cause**: These assertions check for perfect normalization of vectors (norm(vec) == 1.0f) which can sometimes fail due to minor, harmless floating-point inaccuracies (e.g., result is 0.9999999f).

- **Workaround (Advanced)**: If you encounter this, the standard workaround is to patch the OpenMVS library source code to comment out these specific assertions. This requires creating a vcpkg overlay port for OpenMVS:

    **Step 1.** Copy the existing ports/openmvs directory from your main vcpkg installation to a custom overlay directory (e.g., `<project-root>/vcpkg-overlays/openmvs`).

    **Step 2.** In the overlay directory, modify the portfile.cmake to apply a patch file that comments out the problematic ASSERT lines in DepthMap.cpp and DepthMap.h

    ```cpp
    // openMVS/libs/MVS/DepthMap.cpp -> DepthEstimator::ProcessPixel()
    ASSERT(ISEQUAL(norm(normalMap0(nx)), 1.f));   

    // openMVS/libs/MVS/DepthMap.h -> inline void CorrectNormal(Normal& normal) const
    ASSERT(ISEQUAL(norm(normal), 1.f));           

    // openMVS/libs/MVS/DepthMap.h -> inline Normal RandomNormal(const Point3f& viewRay)
    ASSERT(ISEQUAL(norm(normal), 1.f));           
    ```

    **Step 3.** Tell CMake about your overlay directory when configuring:

    ```bash
    cmake .. -DCMAKE_TOOLCHAIN_FILE=... -DVCPKG_OVERLAY_PORTS=<project-root>/vcpkg-overlays
    ```

    **Step 4.** Clean your build directory (`rm -rf build`) and re-run the CMake configuration and build steps. vcpkg will rebuild OpenMVS using your patched version.

 #### **_Note_**: Only apply this patch if you are actually experiencing these specific crashes. It generally doesn't affect the quality of the reconstruction.