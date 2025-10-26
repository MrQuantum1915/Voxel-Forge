# Voxel-Forge  (Legacy Version - Executable Based Integration)

---

## About

Voxel-Forge is a Qt-based application for creating 3D models from images using photogrammetry.

⚠️ **Note:** This branch uses the older **executable-based integration**. It relies on calling pre-installed `openMVG_*` and `OpenMVS_*` command-line tools using Qt's `QProcess`. For the modern, library based integration using vcpkg, please see the `main` branch.

---

## Download the Software

**Note:** There are no prebuilt installable binaries available for this legacy version. To use this version, you must build the application from source by following the instructions in the "Contributing / Building from Source" section below.

Pre-built binaries for the latest version (using library-based integration) can be found on the `main` branch.

---

## Development Setup (Legacy Executable Branch)

### Frameworks Used

- `CMake` as build tool
- `QT` for GUI
- Relies on external `OpenMVG` and `OpenMVS` **command-line executables** for the reconstruction pipeline.

---

## Contributing / Building from Source

### **Step 1**. Install OpenMVG and OpenMVS (as Executables)

* You must compile and install **OpenMVG** from source. Ensure its command-line tools (e.g., `openMVG_main_SfMInit_ImageListing`) are built and accessible in your system's `PATH`.
    * [OpenMVG Build Instructions](https://github.com/openMVG/openMVG/blob/develop/BUILD.md#build-instructions)
* You must compile and install **OpenMVS** from source. Ensure its command-line tools (e.g., `DensifyPointCloud`) are built and accessible in your system's `PATH`.
    * [OpenMVS Build Instructions](https://github.com/cdcseacave/openMVS/blob/develop/BUILD.md)

### **Step 2**. Install CMake and Qt Development Libraries

* Install CMake (version 3.10 or higher).
* Install the Qt5 development libraries suitable for your system (e.g., `qtbase5-dev` on Debian/Ubuntu). Make sure `qmake` is in your PATH or CMake can find your Qt installation.

### **Step 3**. (Optional) OpenMVS - Source Adjustments for Stability

* If you encounter floating-point assertion failures when *running* the pipeline (commonly `DensifyPointCloud` on Linux/GCC), you might need to adjust the OpenMVS source code **before building its executables** in Step 1.
* Comment out the following lines in the OpenMVS source code and rebuild OpenMVS:
    ```cpp
    // openMVS/libs/MVS/DepthMap.cpp -> DepthEstimator::ProcessPixel()
    ASSERT(ISEQUAL(norm(normalMap0(nx)), 1.f));   

    // openMVS/libs/MVS/DepthMap.h -> inline void CorrectNormal(Normal& normal) const
    ASSERT(ISEQUAL(norm(normal), 1.f));           

    // openMVS/libs/MVS/DepthMap.h -> inline Normal RandomNormal(const Point3f& viewRay)
    ASSERT(ISEQUAL(norm(normal), 1.f));           
    ```
* These asserts check for perfect normal vector normalization and can sometimes fail due to minor floating-point inaccuracies. Commenting them out is a common workaround.

### **Step 4**. Build Voxel-Forge

* Clone this repository and checkout the `legacy-executable` branch.
* Run these commands from within the project's root directory:
    ```bash
    mkdir build
    cd build
    cmake ..  
    cmake --build . 
    ```

---

## Running

The executable `Voxel-Forge` will be located inside the `build` directory.

```bash
./Voxel-Forge
