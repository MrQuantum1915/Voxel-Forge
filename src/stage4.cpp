#include "openmvg_wrappers.hpp"

// code implementation taken from openMVG/src/software/SfM/main_GeometricFilter.cpp
// repo

#include "openMVG/features/akaze/image_describer_akaze.hpp"
#include "openMVG/features/descriptor.hpp"
#include "openMVG/features/feature.hpp"
#include "openMVG/graph/graph.hpp"
#include "openMVG/graph/graph_stats.hpp"
#include "openMVG/matching/indMatch.hpp"
#include "openMVG/matching/indMatch_utils.hpp"
// Forward declaration to avoid duplicate symbol with stage3.cpp
namespace openMVG { namespace matching { 
    void PairWiseMatchingToAdjacencyMatrixSVG(const size_t NbImages,
                                              const PairWiseMatches & matches,
                                              const std::string & sOutName);
}}
#include "openMVG/matching_image_collection/Cascade_Hashing_Matcher_Regions.hpp"
#include "openMVG/matching_image_collection/E_ACRobust.hpp"
#include "openMVG/matching_image_collection/E_ACRobust_Angular.hpp"
#include "openMVG/matching_image_collection/Eo_Robust.hpp"
#include "openMVG/matching_image_collection/F_ACRobust.hpp"
#include "openMVG/matching_image_collection/GeometricFilter.hpp"
#include "openMVG/matching_image_collection/H_ACRobust.hpp"
#include "openMVG/matching_image_collection/Matcher_Regions.hpp"
#include "openMVG/matching_image_collection/Pair_Builder.hpp"
#include "openMVG/sfm/pipelines/sfm_features_provider.hpp"
#include "openMVG/sfm/pipelines/sfm_regions_provider.hpp"
#include "openMVG/sfm/pipelines/sfm_regions_provider_cache.hpp"
#include "openMVG/sfm/sfm_data.hpp"
#include "openMVG/sfm/sfm_data_io.hpp"
#include "openMVG/stl/stl.hpp"
#include "openMVG/system/timer.hpp"

#include "openMVG/third_party/stlplus3/filesystemSimplified/file_system.hpp"

#include <cstdlib>
#include <iostream>
#include <locale>
#include <memory>
#include <string>

using namespace openMVG;
using namespace openMVG::matching;
using namespace openMVG::robust;
using namespace openMVG::sfm;
using namespace openMVG::matching_image_collection;

namespace OpenMVG_Wrappers
{
    enum EGeometricModel
    {
        FUNDAMENTAL_MATRIX = 0,
        ESSENTIAL_MATRIX = 1,
        HOMOGRAPHY_MATRIX = 2,
        ESSENTIAL_MATRIX_ANGULAR = 3,
        ESSENTIAL_MATRIX_ORTHO = 4,
        ESSENTIAL_MATRIX_UPRIGHT = 5
    };

    bool RunGeometricFilter(
        std::string sSfM_Data_Filename,
        std::string sPutativeMatchesFilename,
        std::string sFilteredMatchesFilename,
        LogCallback logCallback,
        // optional
        std::string sInputPairsFilename,
        std::string sOutputPairsFilename,
        std::string sGeometricModel,
        bool bForce,
        bool bGuided_matching,
        int imax_iteration,
        unsigned int ui_max_cache_size)
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

        if (sFilteredMatchesFilename.empty())
        {
            LOG_ERROR("It is an invalid output file");
            return false;
        }
        if (sSfM_Data_Filename.empty())
        {
            LOG_ERROR("It is an invalid SfM file");
            return false;
        }
        if (sPutativeMatchesFilename.empty())
        {
            LOG_ERROR("It is an invalid putative match file");
            return false;
        }

        const std::string sMatchesDirectory = stlplus::folder_part(sPutativeMatchesFilename);

        EGeometricModel eGeometricModelToCompute = FUNDAMENTAL_MATRIX;
        switch (std::tolower(sGeometricModel[0], std::locale()))
        {
        case 'f':
            eGeometricModelToCompute = FUNDAMENTAL_MATRIX;
            break;
        case 'e':
            eGeometricModelToCompute = ESSENTIAL_MATRIX;
            break;
        case 'h':
            eGeometricModelToCompute = HOMOGRAPHY_MATRIX;
            break;
        case 'a':
            eGeometricModelToCompute = ESSENTIAL_MATRIX_ANGULAR;
            break;
        case 'u':
            eGeometricModelToCompute = ESSENTIAL_MATRIX_UPRIGHT;
            break;
        case 'o':
            eGeometricModelToCompute = ESSENTIAL_MATRIX_ORTHO;
            break;
        default:
            LOG_ERROR("Unknown geometric model");
            return false;
        }

        // -----------------------------
        // - Load SfM_Data Views & intrinsics data
        // a. Load putative descriptor matches
        // [a.1] Filter matches with input pairs
        // b. Geometric filtering of putative matches
        // + Export some statistics
        // -----------------------------

        //---------------------------------------
        // Read SfM Scene (image view & intrinsics data)
        //---------------------------------------
        SfM_Data sfm_data;
        if (!Load(sfm_data, sSfM_Data_Filename, ESfM_Data(VIEWS | INTRINSICS)))
        {
            LOG_ERROR("The input SfM_Data file \"" + sSfM_Data_Filename + "\" cannot be read.");
            return false;
        }

        //---------------------------------------
        // Load SfM Scene regions
        //---------------------------------------
        // Init the regions_type from the image describer file (used for image regions extraction)
        using namespace openMVG::features;
        // Consider that the image_describer.json is inside the matches directory (which is bellow the sfm_data.bin)
        const std::string sImage_describer = stlplus::create_filespec(sMatchesDirectory, "image_describer.json");
        std::unique_ptr<Regions> regions_type = Init_region_type_from_file(sImage_describer);
        if (!regions_type)
        {
            LOG_ERROR("Invalid: " + sImage_describer + " regions type file.");
            return false;
        }

        //---------------------------------------
        // a. Compute putative descriptor matches
        //    - Descriptor matching (according user method choice)
        //    - Keep correspondences only if NearestNeighbor ratio is ok
        //---------------------------------------

        // Load the corresponding view regions
        std::shared_ptr<Regions_Provider> regions_provider;
        if (ui_max_cache_size == 0)
        {
            // Default regions provider (load & store all regions in memory)
            regions_provider = std::make_shared<Regions_Provider>();
        }
        else
        {
            // Cached regions provider (load & store regions on demand)
            regions_provider = std::make_shared<Regions_Provider_Cache>(ui_max_cache_size);
        }

        // Show the progress on the command line:
        system::LoggerProgress progress;

        if (!regions_provider->load(sfm_data, sMatchesDirectory, regions_type, &progress))
        {
            LOG_ERROR("Invalid regions.");
            return false;
        }

        PairWiseMatches map_PutativeMatches;
        //---------------------------------------
        // A. Load initial matches
        //---------------------------------------
        if (!Load(map_PutativeMatches, sPutativeMatchesFilename))
        {
            LOG_ERROR("Failed to load the initial matches file.");
            return false;
        }

        if (!sInputPairsFilename.empty())
        {
            // Load input pairs
            LOG("Loading input pairs ...");
            Pair_Set input_pairs;
            loadPairs(sfm_data.GetViews().size(), sInputPairsFilename, input_pairs);

            // Filter matches with the given pairs
            LOG("Filtering matches with the given pairs.");
            map_PutativeMatches = getPairs(map_PutativeMatches, input_pairs);
        }

        //---------------------------------------
        // b. Geometric filtering of putative matches
        //    - AContrario Estimation of the desired geometric model
        //    - Use an upper bound for the a contrario estimated threshold
        //---------------------------------------

        // Wrap in a scope to ensure filter_ptr is destroyed before regions_provider
        {
            std::unique_ptr<ImageCollectionGeometricFilter> filter_ptr(
                new ImageCollectionGeometricFilter(&sfm_data, regions_provider));

            if (!filter_ptr)
            {
                LOG_ERROR("Failed to create geometric filter");
                return false;
            }

            system::Timer timer;
            const double d_distance_ratio = 0.6;

            PairWiseMatches map_GeometricMatches;
            switch (eGeometricModelToCompute)
            {
            case HOMOGRAPHY_MATRIX:
            {
                const bool bGeometric_only_guided_matching = true;
                filter_ptr->Robust_model_estimation(
                    GeometricFilter_HMatrix_AC(4.0, imax_iteration),
                    map_PutativeMatches,
                    bGuided_matching,
                    bGeometric_only_guided_matching ? -1.0 : d_distance_ratio,
                    &progress);
                map_GeometricMatches = filter_ptr->Get_geometric_matches();
            }
            break;
            case FUNDAMENTAL_MATRIX:
            {
                filter_ptr->Robust_model_estimation(
                    GeometricFilter_FMatrix_AC(4.0, imax_iteration),
                    map_PutativeMatches,
                    bGuided_matching,
                    d_distance_ratio,
                    &progress);
                map_GeometricMatches = filter_ptr->Get_geometric_matches();
            }
            break;
            case ESSENTIAL_MATRIX:
            {
                filter_ptr->Robust_model_estimation(
                    GeometricFilter_EMatrix_AC(4.0, imax_iteration),
                    map_PutativeMatches,
                    bGuided_matching,
                    d_distance_ratio,
                    &progress);
                map_GeometricMatches = filter_ptr->Get_geometric_matches();

                //-- Perform an additional check to remove pairs with poor overlap
                std::vector<PairWiseMatches::key_type> vec_toRemove;
                for (const auto &pairwisematches_it : map_GeometricMatches)
                {
                    const size_t putativePhotometricCount = map_PutativeMatches.find(pairwisematches_it.first)->second.size();
                    const size_t putativeGeometricCount = pairwisematches_it.second.size();
                    const float ratio = putativeGeometricCount / static_cast<float>(putativePhotometricCount);
                    if (putativeGeometricCount < 50 || ratio < .3f)
                    {
                        // the pair will be removed
                        vec_toRemove.push_back(pairwisematches_it.first);
                    }
                }
                //-- remove discarded pairs
                for (const auto &pair_to_remove_it : vec_toRemove)
                {
                    map_GeometricMatches.erase(pair_to_remove_it);
                }
            }
            break;
            case ESSENTIAL_MATRIX_ANGULAR:
            {
                filter_ptr->Robust_model_estimation(
                    GeometricFilter_ESphericalMatrix_AC_Angular<false>(4.0, imax_iteration),
                    map_PutativeMatches, bGuided_matching, d_distance_ratio, &progress);
                map_GeometricMatches = filter_ptr->Get_geometric_matches();
            }
            break;
            case ESSENTIAL_MATRIX_UPRIGHT:
            {
                filter_ptr->Robust_model_estimation(
                    GeometricFilter_ESphericalMatrix_AC_Angular<true>(4.0, imax_iteration),
                    map_PutativeMatches, bGuided_matching, d_distance_ratio, &progress);
                map_GeometricMatches = filter_ptr->Get_geometric_matches();
            }
            break;
            case ESSENTIAL_MATRIX_ORTHO:
            {
                filter_ptr->Robust_model_estimation(
                    GeometricFilter_EOMatrix_RA(2.0, imax_iteration),
                    map_PutativeMatches,
                    bGuided_matching,
                    d_distance_ratio,
                    &progress);
                map_GeometricMatches = filter_ptr->Get_geometric_matches();
            }
            break;
            }

            //---------------------------------------
            //-- Export geometric filtered matches
            //---------------------------------------
            if (!Save(map_GeometricMatches, sFilteredMatchesFilename))
            {
                LOG_ERROR("Cannot save filtered matches in: " + sFilteredMatchesFilename);
                return false;
            }

            // -- export Geometric View Graph statistics
            graph::getGraphStatistics(sfm_data.GetViews().size(), getPairs(map_GeometricMatches));

            LOG("Task done in (s): " + std::to_string(timer.elapsed()));

            //-- export Adjacency matrix
            LOG("\n Export Adjacency Matrix of the pairwise's geometric matches");

            PairWiseMatchingToAdjacencyMatrixSVG(sfm_data.GetViews().size(),
                                                 map_GeometricMatches,
                                                 stlplus::create_filespec(sMatchesDirectory, "GeometricAdjacencyMatrix", "svg"));

            const Pair_Set outputPairs = getPairs(map_GeometricMatches);

            //-- export view pair graph once geometric filter have been done
            {
                std::set<IndexT> set_ViewIds;
                std::transform(sfm_data.GetViews().begin(), sfm_data.GetViews().end(), std::inserter(set_ViewIds, set_ViewIds.begin()), stl::RetrieveKey());
                graph::indexedGraph putativeGraph(set_ViewIds, outputPairs);
                graph::exportToGraphvizData(
                    stlplus::create_filespec(sMatchesDirectory, "geometric_matches"),
                    putativeGraph);
            }

            // Write pairs
            if (!sOutputPairsFilename.empty())
            {
                LOG("Saving pairs to: " + sOutputPairsFilename);
                if (!savePairs(sOutputPairsFilename, outputPairs))
                {
                    LOG_ERROR("Failed to write pairs file");
                    return false;
                }
            }
        } // End of filter_ptr scope - filter destroyed here before regions_provider
        
        return true;
    }
}
