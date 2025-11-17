
#include "openmvg_wrappers.hpp"

// code implementation taken from openMVG/src/software/SfM/main_ComputeMatches.cpp
// repo

#include "openMVG/graph/graph.hpp"
#include "openMVG/graph/graph_stats.hpp"
#include "openMVG/matching/indMatch.hpp"
#include "openMVG/matching/indMatch_utils.hpp"
#include "openMVG/matching/pairwiseAdjacencyDisplay.hpp"
#include "openMVG/matching_image_collection/Cascade_Hashing_Matcher_Regions.hpp"
#include "openMVG/matching_image_collection/Matcher_Regions.hpp"
#include "openMVG/matching_image_collection/Pair_Builder.hpp"
#include "openMVG/sfm/pipelines/sfm_features_provider.hpp"
#include "openMVG/sfm/pipelines/sfm_preemptive_regions_provider.hpp"
#include "openMVG/sfm/pipelines/sfm_regions_provider.hpp"
#include "openMVG/sfm/pipelines/sfm_regions_provider_cache.hpp"
#include "openMVG/sfm/sfm_data.hpp"
#include "openMVG/sfm/sfm_data_io.hpp"
#include "openMVG/stl/stl.hpp"
#include "openMVG/system/timer.hpp"

#include "openMVG/third_party/stlplus3/filesystemSimplified/file_system.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

using namespace openMVG;
using namespace openMVG::matching;
using namespace openMVG::sfm;
using namespace openMVG::matching_image_collection;

namespace OpenMVG_Wrappers
{
    bool RunComputeMatches(
        std::string sSfM_Data_Filename,
        std::string sOutputMatchesFilename,
        LogCallback logCallback,
        // optional
        float fDistRatio,
        std::string sPredefinedPairList,
        std::string sNearestMatchingMethod,
        bool bForce,
        unsigned int ui_max_cache_size,
        unsigned int ui_preemptive_feature_count,
        double preemptive_matching_percentage_threshold)
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

        auto LOG_WARNING = [&](const std::string &msg)
        {
            OPENMVG_LOG_WARNING << msg;
            if (logCallback)
                logCallback("WARNING: " + msg);
        };

        if (sOutputMatchesFilename.empty())
        {
            LOG_ERROR("No output file set.");
            return false;
        }

        // -----------------------------
        // . Load SfM_Data Views & intrinsics data
        // . Compute putative descriptor matches
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
        const std::string sMatchesDirectory = stlplus::folder_part(sOutputMatchesFilename);

        //---------------------------------------
        // Load SfM Scene regions
        //---------------------------------------
        // Init the regions_type from the image describer file (used for image regions extraction)
        using namespace openMVG::features;
        const std::string sImage_describer = stlplus::create_filespec(sMatchesDirectory, "image_describer", "json");
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
        // If we use pre-emptive matching, we load less regions:
        if (ui_preemptive_feature_count > 0)
        {
            regions_provider = std::make_shared<Preemptive_Regions_Provider>(ui_preemptive_feature_count);
        }

        // Show the progress on the command line:
        system::LoggerProgress progress;

        if (!regions_provider->load(sfm_data, sMatchesDirectory, regions_type, &progress))
        {
            LOG_ERROR("Cannot load view regions from: " + sMatchesDirectory + ".");
            return false;
        }

        PairWiseMatches map_PutativeMatches;

        // Build some alias from SfM_Data Views data:
        // - List views as a vector of filenames & image sizes
        std::vector<std::string> vec_fileNames;
        std::vector<std::pair<size_t, size_t>> vec_imagesSize;
        {
            vec_fileNames.reserve(sfm_data.GetViews().size());
            vec_imagesSize.reserve(sfm_data.GetViews().size());
            for (const auto view_it : sfm_data.GetViews())
            {
                const View *v = view_it.second.get();
                vec_fileNames.emplace_back(stlplus::create_filespec(sfm_data.s_root_path,
                                                                     v->s_Img_path));
                vec_imagesSize.emplace_back(v->ui_width, v->ui_height);
            }
        }

        LOG(" - PUTATIVE MATCHES - ");
        // If the matches already exists, reload them
        if (!bForce && (stlplus::file_exists(sOutputMatchesFilename)))
        {
            if (!(Load(map_PutativeMatches, sOutputMatchesFilename)))
            {
                LOG_ERROR("Cannot load input matches file");
                return false;
            }
            LOG("\t PREVIOUS RESULTS LOADED; #pair: " + std::to_string(map_PutativeMatches.size()));
        }
        else // Compute the putative matches
        {
            // Allocate the right Matcher according the Matching requested method
            std::unique_ptr<Matcher> collectionMatcher;
            if (sNearestMatchingMethod == "AUTO")
            {
                if (regions_type->IsScalar())
                {
                    LOG("Using FAST_CASCADE_HASHING_L2 matcher");
                    collectionMatcher.reset(new Cascade_Hashing_Matcher_Regions(fDistRatio));
                }
                else if (regions_type->IsBinary())
                {
                    LOG("Using HNSWHAMMING matcher");
                    collectionMatcher.reset(new Matcher_Regions(fDistRatio, HNSW_HAMMING));
                }
            }
            else if (sNearestMatchingMethod == "BRUTEFORCEL2")
            {
                LOG("Using BRUTE_FORCE_L2 matcher");
                collectionMatcher.reset(new Matcher_Regions(fDistRatio, BRUTE_FORCE_L2));
            }
            else if (sNearestMatchingMethod == "BRUTEFORCEHAMMING")
            {
                LOG("Using BRUTE_FORCE_HAMMING matcher");
                collectionMatcher.reset(new Matcher_Regions(fDistRatio, BRUTE_FORCE_HAMMING));
            }
            else if (sNearestMatchingMethod == "HNSWL2")
            {
                LOG("Using HNSWL2 matcher");
                collectionMatcher.reset(new Matcher_Regions(fDistRatio, HNSW_L2));
            }
            else if (sNearestMatchingMethod == "HNSWL1")
            {
                LOG("Using HNSWL1 matcher");
                collectionMatcher.reset(new Matcher_Regions(fDistRatio, HNSW_L1));
            }
            else if (sNearestMatchingMethod == "HNSWHAMMING")
            {
                LOG("Using HNSWHAMMING matcher");
                collectionMatcher.reset(new Matcher_Regions(fDistRatio, HNSW_HAMMING));
            }
            else if (sNearestMatchingMethod == "ANNL2")
            {
                LOG("Using ANN_L2 matcher");
                collectionMatcher.reset(new Matcher_Regions(fDistRatio, ANN_L2));
            }
            else if (sNearestMatchingMethod == "CASCADEHASHINGL2")
            {
                LOG("Using CASCADE_HASHING_L2 matcher");
                collectionMatcher.reset(new Matcher_Regions(fDistRatio, CASCADE_HASHING_L2));
            }
            else if (sNearestMatchingMethod == "FASTCASCADEHASHINGL2")
            {
                LOG("Using FAST_CASCADE_HASHING_L2 matcher");
                collectionMatcher.reset(new Cascade_Hashing_Matcher_Regions(fDistRatio));
            }
            if (!collectionMatcher)
            {
                LOG_ERROR("Invalid Nearest Neighbor method: " + sNearestMatchingMethod);
                return false;
            }
            // Perform the matching
            system::Timer timer;
            {
                // From matching mode compute the pair list that have to be matched:
                Pair_Set pairs;
                if (sPredefinedPairList.empty())
                {
                    LOG("No input pair file set. Use exhaustive match by default.");
                    const size_t NImage = sfm_data.GetViews().size();
                    pairs = exhaustivePairs(NImage);
                }
                else if (!loadPairs(sfm_data.GetViews().size(), sPredefinedPairList, pairs))
                {
                    LOG_ERROR("Failed to load pairs from file: \"" + sPredefinedPairList + "\"");
                    return false;
                }
                LOG("Running matching on #pairs: " + std::to_string(pairs.size()));
                // Photometric matching of putative pairs
                collectionMatcher->Match(regions_provider, pairs, map_PutativeMatches, &progress);

                if (ui_preemptive_feature_count > 0) // Preemptive filter
                {
                    // Keep putative matches only if there is more than X matches
                    PairWiseMatches map_filtered_matches;
                    for (const auto &pairwisematches_it : map_PutativeMatches)
                    {
                        const size_t putative_match_count = pairwisematches_it.second.size();
                        const int match_count_threshold =
                            preemptive_matching_percentage_threshold * ui_preemptive_feature_count;
                        // TODO: Add an option to keeping X Best pairs
                        if (putative_match_count >= match_count_threshold)
                        {
                            // the pair will be kept
                            map_filtered_matches.insert(pairwisematches_it);
                        }
                    }
                    map_PutativeMatches.clear();
                    std::swap(map_filtered_matches, map_PutativeMatches);
                }

                //---------------------------------------
                //-- Export putative matches & pairs
                //---------------------------------------
                if (!Save(map_PutativeMatches, std::string(sOutputMatchesFilename)))
                {
                    LOG_ERROR("Cannot save computed matches in: " + sOutputMatchesFilename);
                    return false;
                }
                // Save pairs
                const std::string sOutputPairFilename =
                    stlplus::create_filespec(sMatchesDirectory, "preemptive_pairs", "txt");
                if (!savePairs(
                        sOutputPairFilename,
                        getPairs(map_PutativeMatches)))
                {
                    LOG_ERROR("Cannot save computed matches pairs in: " + sOutputPairFilename);
                    return false;
                }
            }
            LOG("Task (Regions Matching) done in (s): " + std::to_string(timer.elapsed()));
        }

        LOG("#Putative pairs: " + std::to_string(map_PutativeMatches.size()));

        // -- export Putative View Graph statistics
        graph::getGraphStatistics(sfm_data.GetViews().size(), getPairs(map_PutativeMatches));

        //-- export putative matches Adjacency matrix
        PairWiseMatchingToAdjacencyMatrixSVG(vec_fileNames.size(),
                                             map_PutativeMatches,
                                             stlplus::create_filespec(sMatchesDirectory, "PutativeAdjacencyMatrix", "svg"));
        //-- export view pair graph once putative graph matches has been computed
        {
            std::set<IndexT> set_ViewIds;
            std::transform(sfm_data.GetViews().begin(), sfm_data.GetViews().end(), std::inserter(set_ViewIds, set_ViewIds.begin()), stl::RetrieveKey());
            graph::indexedGraph putativeGraph(set_ViewIds, getPairs(map_PutativeMatches));
            graph::exportToGraphvizData(
                stlplus::create_filespec(sMatchesDirectory, "putative_matches"),
                putativeGraph);
        }

        return true;
    }
}
