#pragma once // helps prevent multiple inclusions into same src code

#include <string>
#include <functional>

namespace OpenMVG_Wrappers
{

    using LogCallback = std::function<void(const std::string&)>;

    bool RunImageListing(
        const std::string &sImageDir,
        const std::string &sOutputDir,
        const std::string &sSensorDb = "",
        LogCallback logCallback = nullptr,
        // optional (read docs bro :)) https://openmvg.readthedocs.io/en/latest/software/SfM/SfMInit_ImageListing/#:~:text=Required%20parameters%3A
        double focal_pixels = -1.0,
        const std::string &sKmatrix = "",
        int i_User_camera_model = 3, // PINHOLE_CAMERA_RADIAL3
        bool b_Group_camera_model = true,
        bool b_Use_pose_prior = false,
        const std::string &sPriorWeights = "1.0;1.0;1.0",
        int i_GPS_XYZ_method = 0

    );

    bool RunComputeFeatures(
        std::string sSfM_Data_Filename,
        std::string sOutDir = "",
        LogCallback logCallback = nullptr,

        // optional
        std::string sImage_Describer_Method = "SIFT_ANATOMY", // its free version :) SIFT is non-free and  vcpkg does not support non-free modules
        bool bUpRight = false,
        bool bForce = false,
        std::string sFeaturePreset = "NORMAL",
        int iNumThreads = 0
    );
}
