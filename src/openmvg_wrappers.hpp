#pragma once // helps prevent multiple inclusions into same src code

#include <string>

namespace OpenMVG_Wrappers
{

    bool RunImageListing(
        const std::string &sImageDir,
        const std::string &sOutputDir,
        const std::string &sSensorDb = "",

        // optional (read docs bro :)) https://openmvg.readthedocs.io/en/latest/software/SfM/SfMInit_ImageListing/#:~:text=Required%20parameters%3A
        double focal_pixels = -1.0,
        const std::string &sKmatrix = "",
        int i_User_camera_model = 3, // PINHOLE_CAMERA_RADIAL3
        bool b_Group_camera_model = true,
        bool b_Use_pose_prior = false,
        const std::string &sPriorWeights = "1.0;1.0;1.0",
        int i_GPS_XYZ_method = 0

    );

    // Add wrappers for other stages...
}
