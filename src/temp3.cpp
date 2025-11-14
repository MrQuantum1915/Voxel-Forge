// class PhotogrammetryController : public QObject
// {
//     Q_OBJECT

// public:
//     enum PipelineStage // better than using strings :)
//     {
//         Idle,
//         InitImageListing,
//         ComputeFeatures,
//         ComputeMatches,
//         GeometricFilter,
//         GlobalSfM,
//         // ConvertToPLYModel, // Note: this stage is now just part of GlobalSfM
//         ExportToMVS,
//         Densify,
//         ReconstructMesh,
//         RefineMesh,
//         TextureMesh,
//         Finished,
//         Error
//     };

//     // explicit prevents the compiler from using this constructor for implicit conversions. Like accidentally converting the data type of the argument to the class object it self.
//     explicit PhotogrammetryController(QObject *parent = nullptr)
//         : QObject(parent), currentStage(Idle), cancelRequest(false) {}

//     // will run on worker thread (main thread for GUI)
// public slots: // simply its  a list of member functions then are accesed when a specific signal is emited (we connect signal to this member functions for tasks to be done)
//     void startPipeline(const QString &projectPath)
//     {
//         if (currentStage != Idle)
//         {
//             emit logMessage("Pipeline is already running.");
//             return;
//         }
//         cancelRequest = false;
//         currentStage = InitImageListing;
//         this->projectPath = projectPath;
//         this->imagePath = projectPath + "/images";
//         this->outputPath = projectPath + "/output";

//         QDir().mkpath(outputPath);

//         std::string sImagePath = imagePath.toStdString();
//         std::string sMatchesDir = (outputPath + "/matches").toStdString();
//         std::string sReconDir = (outputPath + "/reconstruction").toStdString();
//         std::string sMVSSceneFile = (outputPath + "/scene.mvs").toStdString();

//         // harcoded. better make this a setting!
//         std::string sSensorDb = "/mnt/WinDisk/1_Darshan/CS/Programing/Projects/3D_Reconstruction_Software/Voxel-Forge/build/vcpkg_installed/x64-linux/share/openmvg/sensor_width_camera_database.txt";

//         // create directories
//         // std::filesystem::create_directories(sMatchesDir);
//         // std::filesystem::create_directories(sReconDir);
//         // make folder using QDir
//         QDir().mkpath(QString::fromStdString(sMatchesDir));
//         QDir().mkpath(QString::fromStdString(sReconDir));


//         // --- Hold scene data in memory ---
//         openMVG::sfm::SfM_Data sfm_data;
//         MVS::Scene mvs_scene;

//         // use a smart pointer for regions, as it's large
//         auto regions_provider = std::make_shared<openMVG::sfm::Regions_Provider>();

//         emit logMessage("<b>Starting Pipeline...</b>");

//         try
//         {
//             // sequentially run through the stages
//             while (currentStage != Finished && currentStage != Error && !cancelRequest)
//             {
//                 switch (currentStage)
//                 {
//                 // --- STAGE 1: IMAGE LISTING ---
//                 case InitImageListing:
//                     emit logMessage("--- Stage 1: Image Listing ---");
//                     if (!openMVG::sfm::LoadScene(sfm_data, sImagePath, sMatchesDir, sSensorDb))
//                         throw std::runtime_error("Image Listing failed.");
//                     emit logMessage("Image Listing complete.");
//                     currentStage = ComputeFeatures;
//                     break;

//                 // --- STAGE 2: COMPUTE FEATURES ---
//                 case ComputeFeatures:
//                     emit logMessage("--- Stage 2: Feature Computation ---");
//                     if (!regions_provider->load(sfm_data, sMatchesDir))
//                     {
//                         // If loading fails, compute them
//                         if (!regions_provider->compute_features(sfm_data, openMVG::features::NORMAL, false))
//                             throw std::runtime_error("Feature Computation failed.");
//                         // Save them for future runs
//                         regions_provider->save(sfm_data, sMatchesDir);
//                     }
//                     emit logMessage("Feature Computation complete.");
//                     currentStage = ComputeMatches;
//                     break;

//                 // had to refer to openMVG source code to implement this stage properly :( because there is no higher level function that does this directly
//                 case ComputeMatches:
//                 {
//                     emit logMessage("--- Stage 3: Match Computation (Putative) ---");

//                     // output file for matches
//                     std::string sMatchesFile = sMatchesDir + "/matches.putative.bin";

//                     // this object will hold the results
//                     openMVG::matching::PairWiseMatches map_PutativeMatches;

//                     // --- allocate the matcher ---
//                     // we need the region type, the regions_provider has it.
//                     const openMVG::features::Regions *regions_type = regions_provider->regions_type().get();
//                     if (!regions_type)
//                         throw std::runtime_error("Regions type is invalid in ComputeMatches.");

//                     // set a default ratio. 0.8f is std.
//                     const float fDistRatio = 0.8f;
//                     std::unique_ptr<openMVG::matching_image_collection::Matcher> collectionMatcher;

//                     // replicate the "AUTO" logic from main_ComputeMatches.cpp
//                     if (regions_type->IsScalar())
//                     {
//                         emit logMessage("Using FAST_CASCADE_HASHING_L2 matcher");
//                         collectionMatcher.reset(new openMVG::matching_image_collection::Cascade_Hashing_Matcher_Regions(fDistRatio));
//                     }
//                     else if (regions_type->IsBinary())
//                     {
//                         emit logMessage("Using HNSWHAMMING matcher");
//                         collectionMatcher.reset(new openMVG::matching_image_collection::Matcher_Regions(fDistRatio, openMVG::matching::HNSW_HAMMING));
//                     }

//                     if (!collectionMatcher)
//                         throw std::runtime_error("Invalid Nearest Neighbor method or regions type.");

//                     // --- get pairs ---
//                     // we don't have a predefined pair list, so we use exhaustive.
//                     openMVG::Pair_Set pairs = openMVG::exhaustivePairs(sfm_data.GetViews().size());
//                     emit logMessage("Matching " + QString::number(pairs.size()) + " exhaustive pairs.");

//                     // --- run matching ---
//                     openMVG::system::Timer timer;
//                     // Note: We pass nullptr for the progress callback.
//                     // can add custom progress object in future
//                     collectionMatcher->Match(regions_provider, pairs, map_PutativeMatches, nullptr);
//                     emit logMessage(QString("Task (Regions Matching) done in: %1s").arg(timer.elapsed()));

//                     // --- save matches ---
//                     // the next stage will load this file.
//                     if (!Save(map_PutativeMatches, sMatchesFile))
//                         throw std::runtime_error("Cannot save computed matches file.");

//                     emit logMessage(QString("Putative matches computed and saved: %1 pairs").arg(map_PutativeMatches.size()));

//                     currentStage = GeometricFilter;
//                     break;
//                 }

//                 // --- STAGE 4: GEOMETRIC FILTER ---
//                 case GeometricFilter:
//                 {
//                     emit logMessage("--- Stage 4: Geometric Filtering ---");

//                     // input file (from stage 3):
//                     std::string sPutativeMatchesFile = sMatchesDir + "/matches.putative.bin";
//                     // output file (for Stage 5):
//                     std::string sFilteredMatchesFile = sMatchesDir + "/matches.f.bin";

//                     // --- load putative matches ---
//                     openMVG::matching::PairWiseMatches map_PutativeMatches;
//                     if (!Load(map_PutativeMatches, sPutativeMatchesFile))
//                         throw std::runtime_error("Geometric Filter: Failed to load putative matches file.");

//                     // --- create the geometric filter ---
//                     // this filter uses our in-memory scene data (sfm_data) and features (regions_provider)
//                     std::unique_ptr<openMVG::matching_image_collection::ImageCollectionGeometricFilter> filter_ptr(
//                         new openMVG::matching_image_collection::ImageCollectionGeometricFilter(
//                             &sfm_data, regions_provider));

//                     // --- set filter parameters (from main_GeometricFilter.cpp defaults) (the source code) ---
//                     const double d_distance_ratio = 0.6; // For guided matching (if enabled)
//                     const int imax_iteration = 2048;     // RANSAC iterations
//                     const bool bGuided_matching = false; // We'll use the default (false)

//                     openMVG::matching::PairWiseMatches map_GeometricMatches;
//                     openMVG::system::Timer timer;

//                     // --- Run the Filter ---
//                     // We will replicate the default behavior: Fundamental Matrix (FMatrix_AC)
//                     // with a 4.0 pixel threshold and max 2048 iterations.
//                     emit logMessage("Running robust model estimation (AC-RANSAC)...");
//                     filter_ptr->Robust_model_estimation(
//                         openMVG::robust::GeometricFilter_FMatrix_AC(4.0, imax_iteration),
//                         map_PutativeMatches,
//                         bGuided_matching,
//                         d_distance_ratio,
//                         nullptr // No progress bar
//                     );
//                     map_GeometricMatches = filter_ptr->Get_geometric_matches();

//                     emit logMessage(QString("Task (Geometric Filtering) done in: %1s").arg(timer.elapsed()));

//                     // --- Save Filtered Matches ---
//                     if (!Save(map_GeometricMatches, sFilteredMatchesFile))
//                         throw std::runtime_error("Cannot save geometric filtered matches file.");

//                     emit logMessage(QString("Geometric filtering complete. Saved %1 geometrically-valid pairs.").arg(map_GeometricMatches.size()));

//                     currentStage = GlobalSfM;
//                     break;
//                 }

//                     // --- STAGE 5 & 6: GLOBAL RECONSTRUCTION & PLY EXPORT ---
//                 case GlobalSfM:
//                 {
//                     emit logMessage("--- Stage 5: Global Reconstruction ---");

//                     // --- Load Filtered Matches ---
//                     // Load the output from Stage 4 (GeometricFilter)
//                     std::string sFilteredMatchesFile = sMatchesDir + "/matches.f.bin";
//                     openMVG::matching::PairWiseMatches map_GeometricMatches;
//                     if (!Load(map_GeometricMatches, sFilteredMatchesFile))
//                         throw std::runtime_error("GlobalSfM: Failed to load geometric matches file.");

//                     // --- Run the Global SfM Engine ---
//                     // sfm_data is still in memory!
//                     openMVG::sfm::GlobalSfM_Engine sfm_engine(sfm_data);

//                     openMVG::system::Timer timer;
//                     if (!sfm_engine.Process(map_GeometricMatches, true)) // true == triangulate
//                         throw std::runtime_error("Global Reconstruction failed.");

//                     emit logMessage(QString("Task (Global Reconstruction) done in: %1s").arg(timer.elapsed()));

//                     // --- Save the SfM scene (sfm_data.bin) ---
//                     // The scene (sfm_data) is now updated with 3D points and camera poses
//                     std::string sBinFile = sReconDir + "/sfm_data.bin";
//                     if (!Save(sfm_data, sBinFile, openMVG::sfm::ESfM_Data(openMVG::sfm::VIEWS | openMVG::sfm::INTRINSICS | openMVG::sfm::EXTRINSICS | openMVG::sfm::STRUCTURE | openMVG::sfm::LANDMARKS)))
//                     {
//                         throw std::runtime_error("Failed to save sfm_data.bin file.");
//                     }

//                     // --- STAGE 6: CONVERT TO PLY MODEL (Combined) ---
//                     // This is the logic from main_ComputeSfM_DataColor.cpp
//                     emit logMessage("--- Stage 6: Converting to PLY Model ---");

//                     // Colorize the sparse point cloud
//                     openMVG::sfm::Colorize_Landmarks(sfm_data);

//                     // Save the sparse point cloud as a .ply file
//                     std::string sPlyFile = sReconDir + "/sparse_model.ply";
//                     if (!openMVG::sfm::sfm_data_triangulation::Triangulate_Color_And_Save_To_PLY(sfm_data, sPlyFile))
//                     {
//                         throw std::runtime_error("Failed to compute and save sparse PLY file.");
//                     }

//                     emit logMessage(QString("Sparse PLY model saved to: %1").arg(QString::fromStdString(sPlyFile)));

//                     currentStage = ExportToMVS;
//                     break;
//                 }

//                 // --- STAGE 7: EXPORT TO MVS ---
//                 // got from source code of /main_openMVG2openMVS.cpp that is in on this path : src/software/SfM/export/main_openMVG2openMVS.cpp  -- of openMVG repo
//                 case ExportToMVS:
//                 {
//                     emit logMessage("--- Stage 7: Exporting to OpenMVS ---");

//                     // --- Setup Paths ---
//                     // sMVSSceneFile is already defined: outputPath + "/scene.mvs"
//                     // We also need a directory for the undistorted images
//                     std::string sUndistortedImagesDir = (outputPath + "/undistorted_images").toStdString();

//                     if (!stlplus::is_folder(sUndistortedImagesDir))
//                     {
//                         stlplus::folder_create(sUndistortedImagesDir);
//                     }
//                     const std::string sOutSceneDir = stlplus::folder_part(sMVSSceneFile);
//                     const std::string sOutImagesDir = stlplus::folder_to_relative_path(sOutSceneDir, sUndistortedImagesDir);

//                     // --- 1. Create the OpenMVS Scene Object ---
//                     _INTERFACE_NAMESPACE::Interface mvs_scene_out; // Use the MVS::Interface
//                     size_t nPoses = 0;
//                     const uint32_t nViews = (uint32_t)sfm_data.GetViews().size();

//                     // Map OpenMVG's non-contiguous IDs to OpenMVS's 0-based IDs
//                     std::map<openMVG::IndexT, uint32_t> map_intrinsic, map_view;

//                     // --- 2. Convert Platforms (Intrinsics) ---
//                     emit logMessage("Converting cameras/platforms...");
//                     for (const auto &intrinsic : sfm_data.GetIntrinsics())
//                     {
//                         if (isPinhole(intrinsic.second->getType()))
//                         {
//                             const Pinhole_Intrinsic *cam = dynamic_cast<const Pinhole_Intrinsic *>(intrinsic.second.get());
//                             if (map_intrinsic.count(intrinsic.first) == 0)
//                                 map_intrinsic.insert(std::make_pair(intrinsic.first, mvs_scene_out.platforms.size()));

//                             _INTERFACE_NAMESPACE::Interface::Platform platform;
//                             _INTERFACE_NAMESPACE::Interface::Platform::Camera camera;
//                             camera.width = cam->w();
//                             camera.height = cam->h();
//                             camera.K = cam->K();
//                             camera.R = Mat3::Identity();
//                             camera.C = Vec3::Zero();
//                             platform.cameras.push_back(camera);
//                             mvs_scene_out.platforms.push_back(platform);
//                         }
//                     }

//                     // --- 3. Convert Images and Poses ---
//                     emit logMessage("Converting " + QString::number(nViews) + " views and poses...");
//                     mvs_scene_out.images.reserve(nViews);

//                     for (const auto &view : sfm_data.GetViews())
//                     {
//                         if (sfm_data.IsPoseAndIntrinsicDefined(view.second.get()))
//                         {
//                             map_view[view.first] = mvs_scene_out.images.size();

//                             _INTERFACE_NAMESPACE::Interface::Image image;
//                             // Set path relative to the .mvs file
//                             image.name = stlplus::create_filespec(sOutImagesDir, view.second->s_Img_path);
//                             image.platformID = map_intrinsic.at(view.second->id_intrinsic);
//                             _INTERFACE_NAMESPACE::Interface::Platform &platform = mvs_scene_out.platforms[image.platformID];
//                             image.cameraID = 0;

//                             _INTERFACE_NAMESPACE::Interface::Platform::Pose pose;
//                             image.poseID = platform.poses.size();
//                             image.ID = map_view[view.first];
//                             const openMVG::geometry::Pose3 poseMVG(sfm_data.GetPoseOrDie(view.second.get()));
//                             pose.R = poseMVG.rotation();
//                             pose.C = poseMVG.center();
//                             platform.poses.push_back(pose);
//                             ++nPoses;

//                             mvs_scene_out.images.emplace_back(image);
//                         }
//                     }
//                     emit logMessage(QString("Exported %1 calibrated poses.").arg(nPoses));

//                     // --- 4. Undistort Images (The Slow Part) ---
//                     emit logMessage("Undistorting images (this may take a while)...");
//                     system::LoggerProgress my_progress_bar_images(sfm_data.views.size(), "- Undistorting Images ");
//                     std::atomic<bool> bOk(true);

// #ifdef OPENMVG_USE_OPENMP
//                     // Use 2 threads, same as our VCPKG_MAX_CONCURRENCY setting
//                     const unsigned int nb_max_thread = 2;
// #pragma omp parallel for schedule(dynamic) num_threads(nb_max_thread)
// #endif
//                     for (int i = 0; i < static_cast<int>(sfm_data.views.size()); ++i)
//                     {
//                         ++my_progress_bar_images;
//                         if (!bOk)
//                             continue;

//                         Views::const_iterator iterViews = sfm_data.views.begin();
//                         std::advance(iterViews, i);
//                         const View *view = iterViews->second.get();

//                         const std::string srcImage = stlplus::create_filespec(sfm_data.s_root_path, view->s_Img_path);
//                         const std::string imageName = stlplus::create_filespec(sUndistortedImagesDir, view->s_Img_path);

//                         if (sfm_data.IsPoseAndIntrinsicDefined(view))
//                         {
//                             const openMVG::cameras::IntrinsicBase *cam = sfm_data.GetIntrinsics().at(view->id_intrinsic).get();
//                             if (cam->have_disto())
//                             {
//                                 // Undistort and save
//                                 Image<openMVG::image::RGBColor> imageRGB, imageRGB_ud;
//                                 if (ReadImage(srcImage.c_str(), &imageRGB))
//                                 {
//                                     UndistortImage(imageRGB, cam, imageRGB_ud, BLACK);
//                                     bOk = bOk & WriteImage(imageName.c_str(), imageRGB_ud);
//                                 }
//                                 else
//                                 {
//                                     Image<uint8_t> image_gray, image_gray_ud;
//                                     if (ReadImage(srcImage.c_str(), &image_gray))
//                                     {
//                                         UndistortImage(image_gray, cam, image_gray_ud, BLACK);
//                                         bOk = bOk & WriteImage(imageName.c_str(), image_gray_ud);
//                                     }
//                                     else
//                                     {
//                                         emit logMessage("<font color='red'>Failed to read image: " + QString::fromStdString(srcImage) + "</font>");
//                                         bOk = false;
//                                     }
//                                 }
//                             }
//                             else
//                             {
//                                 stlplus::file_copy(srcImage, imageName); // Just copy
//                             }
//                         }
//                         else
//                         {
//                             stlplus::file_copy(srcImage, imageName); // Just copy
//                         }
//                     }
//                     if (!bOk)
//                         throw std::runtime_error("Failed during image undistortion (check file permissions or disk space).");

//                     // --- 5. Convert Structure (Landmarks) ---
//                     emit logMessage("Converting 3D structure...");
//                     mvs_scene_out.vertices.reserve(sfm_data.GetLandmarks().size());
//                     for (const auto &vertex : sfm_data.GetLandmarks())
//                     {
//                         const Landmark &landmark = vertex.second;
//                         _INTERFACE_NAMESPACE::Interface::Vertex vert;
//                         _INTERFACE_NAMESPACE::Interface::Vertex::ViewArr &views = vert.views;
//                         for (const auto &observation : landmark.obs)
//                         {
//                             const auto it(map_view.find(observation.first));
//                             if (it != map_view.end())
//                             {
//                                 _INTERFACE_NAMESPACE::Interface::Vertex::View view;
//                                 view.imageID = it->second;
//                                 view.confidence = 0;
//                                 views.push_back(view);
//                             }
//                         }
//                         if (views.size() < 2)
//                             continue;

//                         vert.X = landmark.X.cast<float>();
//                         mvs_scene_out.vertices.push_back(vert);
//                     }
//                     emit logMessage(QString("Exported %1 landmarks.").arg(mvs_scene_out.vertices.size()));

//                     // --- 6. Write OpenMVS scene.mvs file ---
//                     if (!_INTERFACE_NAMESPACE::ARCHIVE::SerializeSave(mvs_scene_out, sMVSSceneFile))
//                         throw std::runtime_error("Failed to write scene.mvs file.");

//                     emit logMessage("Export to OpenMVS complete.");
//                     currentStage = Densify;
//                     break;
//                 }

//                 // --- STAGE 8: DENSIFY ---
//                 case Densify:
//                 {
//                     emit logMessage("--- Stage 8: Densifying Point Cloud ---");
//                     if (!mvs_scene.Load(sMVSSceneFile))
//                         throw std::runtime_error("Failed to load MVS scene for densify.");

//                     // Set working folder for OpenMVS
//                     mvs_scene.SetWorkingFolder(outputPath.toStdString());

//                     if (mvs_scene.DenseReconstruction() != 0) // 0 is success
//                         throw std::runtime_error("Dense Reconstruction failed.");

//                     emit logMessage("Densify complete.");
//                     currentStage = ReconstructMesh;
//                     break;
//                 }
//                 // --- STAGE 9: RECONSTRUCT MESH ---
//                 case ReconstructMesh:
//                 {
//                     emit logMessage("--- Stage 9: Reconstructing Mesh ---");
//                     if (mvs_scene.MeshReconstruction() != 0)
//                         throw std::runtime_error("Mesh Reconstruction failed.");
//                     emit logMessage("Mesh reconstruction complete.");
//                     currentStage = RefineMesh;
//                     break;
//                 }

//                 // --- STAGE 10: REFINE MESH ---
//                 case RefineMesh:
//                 {
//                     emit logMessage("--- Stage 10: Refining Mesh ---");
//                     if (mvs_scene.MeshRefinement() != 0)
//                         throw std::runtime_error("Mesh Refinement failed.");
//                     emit logMessage("Mesh refinement complete.");
//                     currentStage = TextureMesh;
//                     break;
//                 }
//                 // --- STAGE 11: TEXTURE MESH ---
//                 case TextureMesh:
//                 {
//                     emit logMessage("--- Stage 11: Texturing Mesh ---");
//                     if (mvs_scene.MeshTexturing() != 0)
//                         throw std::runtime_error("Mesh Texturing failed.");
//                     emit logMessage("Mesh texturing complete.");
//                     currentStage = Finished;
//                     break;
//                 }

//                 default:
//                     currentStage = Error;
//                     break;
//                 }
//             }

//             if (cancelRequest)
//             {
//                 emit logMessage("<font color='orange'><b>Pipeline was cancelled by user.</b></font>");
//                 currentStage = Idle;
//                 emit pipelineFinished(false);
//             }
//             else if (currentStage == Finished)
//             {
//                 emit logMessage("<font color='green'><b>Pipeline completed successfully.</b></font>");
//                 currentStage = Idle;
//                 emit pipelineFinished(true);
//             }
//         }
//         catch (const std::exception &e)
//         {
//             emit logMessage("<font color='red'><b>PIPELINE ERROR: " + QString(e.what()) + "</b></font>");
//             currentStage = Error;
//             emit pipelineFinished(false);
//         }
//         catch (...)
//         {
//             emit logMessage("<font color='red'><b>An unknown critical error occurred.</b></font>");
//             currentStage = Error;
//             emit pipelineFinished(false);
//         }
//     }

//     // This slot is connected to the "Cancel" button.
//     // It's thread-safe because std::atomic is.
//     void cancelPipeline()
//     {
//         cancelRequest = true;
//     }

// signals:
//     void logMessage(const QString &message);
//     void pipelineFinished(bool success);

// private:
//     std::atomic<bool> cancelRequest;
//     PipelineStage currentStage;
//     QString projectPath, imagePath, outputPath;
// };