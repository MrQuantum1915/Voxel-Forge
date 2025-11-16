#include "openmvg_wrappers.hpp"

#include <cereal/archives/json.hpp>

#include "openMVG/features/akaze/image_describer_akaze_io.hpp"

#include "openMVG/features/sift/SIFT_Anatomy_Image_Describer_io.hpp"
#include "openMVG/image/image_io.hpp"
#include "openMVG/features/regions_factory_io.hpp"
#include "openMVG/sfm/sfm_data.hpp"
#include "openMVG/sfm/sfm_data_io.hpp"
#include "openMVG/system/logger.hpp"
#include "openMVG/system/loggerprogress.hpp"
#include "openMVG/system/timer.hpp"

#include "openMVG/third_party/stlplus3/filesystemSimplified/file_system.hpp"

#include <cereal/details/helpers.hpp>

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <string>

#ifdef OPENMVG_USE_OPENMP
#include <omp.h>
#endif

using namespace openMVG;
using namespace openMVG::image;
using namespace openMVG::features;
using namespace openMVG::sfm;

namespace OpenMVG_Wrappers
{

    features::EDESCRIBER_PRESET stringToEnum(const std::string &sPreset)
    {
        features::EDESCRIBER_PRESET preset;
        if (sPreset == "NORMAL")
            preset = features::NORMAL_PRESET;
        else if (sPreset == "HIGH")
            preset = features::HIGH_PRESET;
        else if (sPreset == "ULTRA")
            preset = features::ULTRA_PRESET;
        else
            preset = features::EDESCRIBER_PRESET(-1);
        return preset;
    }

    bool RunComputeFeatures(
        std::string sSfM_Data_Filename,
        std::string sOutDir,
        LogCallback logCallback,
        // optional
        std::string sImage_Describer_Method,
        bool bUpRight,
        bool bForce,
        std::string sFeaturePreset,
        int iNumThreads)
    {

        // Helper for logging to both console and GUI
        auto LOG = [&](const std::string &msg)
        {
            OPENMVG_LOG_INFO << msg;
            if (logCallback)
                logCallback(msg);
        };

        auto LOG_ERROR = [&](const std::string &msg)
        {
            OPENMVG_LOG_ERROR << msg;
            if (logCallback)
                logCallback("ERROR: " + msg);
        };

        if (sOutDir.empty())
        {
            LOG_ERROR("\nIt is an invalid output directory");
            return false;
        }

        // Create output dir
        if (!stlplus::folder_exists(sOutDir))
        {
            if (!stlplus::folder_create(sOutDir))
            {
                LOG_ERROR("Cannot create output directory");
                return false;
            }
        }

        //---------------------------------------
        // a. Load input scene
        //---------------------------------------
        SfM_Data sfm_data;
        if (!Load(sfm_data, sSfM_Data_Filename, ESfM_Data(VIEWS | INTRINSICS)))
        {
            LOG_ERROR("The input file \"" + sSfM_Data_Filename + "\" cannot be read");
            return false;
        }

        // b. Init the image_describer
        // - retrieve the used one in case of pre-computed features
        // - else create the desired one

        using namespace openMVG::features;
        std::unique_ptr<Image_describer> image_describer;

        const std::string sImage_describer = stlplus::create_filespec(sOutDir, "image_describer", "json");
        if (!bForce && stlplus::is_file(sImage_describer))
        {
            // Dynamically load the image_describer from the file (will restore old used settings)
            std::ifstream stream(sImage_describer.c_str());
            if (!stream)
                return false;

            try
            {
                cereal::JSONInputArchive archive(stream);
                archive(cereal::make_nvp("image_describer", image_describer));
            }
            catch (const cereal::Exception &e)
            {
                LOG_ERROR(std::string(e.what()) + "\nCannot dynamically allocate the Image_describer interface.");
                return false;
            }
        }
        else
        {
            // Create the desired Image_describer method.
            // Don't use a factory, perform direct allocation
            if (sImage_Describer_Method == "SIFT_ANATOMY")
            {
                image_describer.reset(
                    new SIFT_Anatomy_Image_describer(SIFT_Anatomy_Image_describer::Params()));
            }
            else if (sImage_Describer_Method == "AKAZE_FLOAT")
            {
                image_describer = AKAZE_Image_describer::create(AKAZE_Image_describer::Params(AKAZE::Params(), AKAZE_MSURF), !bUpRight);
            }
            else if (sImage_Describer_Method == "AKAZE_MLDB")
            {
                image_describer = AKAZE_Image_describer::create(AKAZE_Image_describer::Params(AKAZE::Params(), AKAZE_MLDB), !bUpRight);
            }
            if (!image_describer)
            {
                LOG_ERROR("Cannot create the designed Image_describer: " + sImage_Describer_Method + ".");
                return false;
            }
            else
            {
                if (!sFeaturePreset.empty())
                    if (!image_describer->Set_configuration_preset(stringToEnum(sFeaturePreset)))
                    {
                        LOG_ERROR("Preset configuration failed.");
                        return false;
                    }
            }

            // Export the used Image_describer and region type for:
            // - dynamic future regions computation and/or loading
            {
                std::ofstream stream(sImage_describer.c_str());
                if (!stream)
                    return false;

                cereal::JSONOutputArchive archive(stream);
                archive(cereal::make_nvp("image_describer", image_describer));
                auto regionsType = image_describer->Allocate();
                archive(cereal::make_nvp("regions_type", regionsType));
            }
        }

        // Feature extraction routines
        // For each View of the SfM_Data container:
        // - if regions file exists continue,
        // - if no file, compute features
        {
            system::Timer timer;
            Image<unsigned char> imageGray;

            system::LoggerProgress my_progress_bar(sfm_data.GetViews().size(), "- EXTRACT FEATURES -");

            // Use a boolean to track if we must stop feature extraction
            std::atomic<bool> preemptive_exit(false);
#ifdef OPENMVG_USE_OPENMP
            const unsigned int nb_max_thread = omp_get_max_threads();

            if (iNumThreads > 0)
            {
                omp_set_num_threads(iNumThreads);
            }
            else
            {
                omp_set_num_threads(nb_max_thread);
            }

#pragma omp parallel for schedule(dynamic) if (iNumThreads > 0) private(imageGray)
#endif
            for (int i = 0; i < static_cast<int>(sfm_data.views.size()); ++i)
            {
                Views::const_iterator iterViews = sfm_data.views.begin();
                std::advance(iterViews, i);
                const View *view = iterViews->second.get();
                const std::string
                    sView_filename = stlplus::create_filespec(sfm_data.s_root_path, view->s_Img_path),
                    sFeat = stlplus::create_filespec(sOutDir, stlplus::basename_part(sView_filename), "feat"),
                    sDesc = stlplus::create_filespec(sOutDir, stlplus::basename_part(sView_filename), "desc");

                // If features or descriptors file are missing, compute them
                if (!preemptive_exit && (bForce || !stlplus::file_exists(sFeat) || !stlplus::file_exists(sDesc)))
                {
                    if (!ReadImage(sView_filename.c_str(), &imageGray))
                        continue;

                    //
                    // Look if there is an occlusion feature mask
                    //
                    Image<unsigned char> *mask = nullptr; // The mask is null by default

                    const std::string
                        mask_filename_local =
                            stlplus::create_filespec(sfm_data.s_root_path,
                                                     stlplus::basename_part(sView_filename) + "_mask", "png"),
                        mask_filename_global =
                            stlplus::create_filespec(sfm_data.s_root_path, "mask", "png");

                    Image<unsigned char> imageMask;
                    // Try to read the local mask
                    if (stlplus::file_exists(mask_filename_local))
                    {
                        if (!ReadImage(mask_filename_local.c_str(), &imageMask))
                        {
                            LOG_ERROR("Invalid mask: " + mask_filename_local + "; Stopping feature extraction.");
                            preemptive_exit = true;
                            continue;
                        }
                        // Use the local mask only if it fits the current image size
                        if (imageMask.Width() == imageGray.Width() && imageMask.Height() == imageGray.Height())
                            mask = &imageMask;
                    }
                    else
                    {
                        // Try to read the global mask
                        if (stlplus::file_exists(mask_filename_global))
                        {
                            if (!ReadImage(mask_filename_global.c_str(), &imageMask))
                            {
                                LOG_ERROR("Invalid mask: " + mask_filename_global + "; Stopping feature extraction.");
                                preemptive_exit = true;
                                continue;
                            }
                            // Use the global mask only if it fits the current image size
                            if (imageMask.Width() == imageGray.Width() && imageMask.Height() == imageGray.Height())
                                mask = &imageMask;
                        }
                    }

                    // Compute features and descriptors and export them to files
                    auto regions = image_describer->Describe(imageGray, mask);
                    if (regions && !image_describer->Save(regions.get(), sFeat, sDesc))
                    {
                        LOG_ERROR("Cannot save regions for image: " + sView_filename + "; Stopping feature extraction.");
                        preemptive_exit = true;
                        continue;
                    }
                }
                ++my_progress_bar;
            }
            LOG("Task done in (s): " + std::to_string(timer.elapsed()));
        }
        return true;
    }
}