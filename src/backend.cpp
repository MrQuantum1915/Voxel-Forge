// Copyright Darshan Patel [Mr.Quantum_1915]:)
// Backend implementation for video extraction and photogrammetry pipeline

#include "backend.h"
#include "openmvg_wrappers.hpp"

#include <QDir>
#include <QFileInfo>
#include <opencv2/opencv.hpp>

// OpenMVS
#include <openmvs/MVS.h>

#undef D2R
#undef R2D

// OpenMVG headers
#include "openMVG/sfm/sfm_data.hpp"
#include "openMVG/sfm/sfm_data_io.hpp"
#include "openMVG/features/feature.hpp"
#include "openMVG/sfm/pipelines/sfm_engine.hpp"
#include "openMVG/sfm/pipelines/sfm_regions_provider.hpp"
#include "openMVG/matching/indMatch.hpp"
#include "openMVG/matching/indMatch_utils.hpp"
#include "openMVG/matching_image_collection/Pair_Builder.hpp"
#include "openMVG/matching_image_collection/Matcher_Regions.hpp"
#include "openMVG/matching_image_collection/Cascade_Hashing_Matcher_Regions.hpp"
#include "openMVG/matching_image_collection/GeometricFilter.hpp"
#include "openMVG/matching_image_collection/F_ACRobust.hpp"
#include "openMVG/sfm/pipelines/global/sfm_global_engine_relative_motions.hpp"
#include "openMVG/sfm/sfm_data_triangulation.hpp"
#include "openMVG/sfm/sfm_data_colorization.hpp"
#include "openMVG/cameras/Camera_Pinhole.hpp"
#include "openMVG/cameras/Camera_undistort_image.hpp"
#include "openMVG/image/image_io.hpp"
#include "openMVG/system/timer.hpp"
#include "openMVG/system/loggerprogress.hpp"
#include "openMVG/third_party/stlplus3/filesystemSimplified/file_system.hpp"

void VideoFrameExtractor::extractFrames(const QString &videoFilePath, const QString &projectFullPath)
{
    QString imageDir = projectFullPath + "/images";
    QDir().mkpath(imageDir);

    emit logMessage("Extracting frames from video: " + videoFilePath);
    emit logMessage("Frames will be saved to: " + imageDir);

    // Extract frames using OpenCV
    cv::VideoCapture cap(videoFilePath.toStdString());
    if (!cap.isOpened())
    {
        emit logMessage("ERROR: Could not open video file!");
        emit extractionFinished(false);
        return;
    }

    int frameIndex = 0;
    cv::Mat frame;
    while (cap.read(frame) && !cancelRequest)
    {
        QString frameName = QString("frame_%1.jpg").arg(frameIndex, 6, 10, QChar('0'));
        QString framePath = imageDir + "/" + frameName;

        if (cv::imwrite(framePath.toStdString(), frame))
        {
            emit logMessage(QString("Saved: %1").arg(frameName));
            emit frameExtracted(frameName, framePath);
        }
        else
        {
            emit logMessage(QString("Failed to save: %1").arg(frameName));
        }

        frameIndex++;
    }

    if (cancelRequest)
    {
        emit logMessage("Frame extraction cancelled by user.");
        emit extractionFinished(false);
    }
    else
    {
        emit logMessage(QString("Frame extraction complete! Extracted %1 frames.").arg(frameIndex));
        emit extractionFinished(true);
    }

    cancelRequest = false;
}

void VideoFrameExtractor::cancelExtraction()
{
    cancelRequest = true;
}

// PhotogrammetryController implementation
void PhotogrammetryController::startPipeline(const QString &projectPath)
{
    if (currentStage != Idle)
    {
        emit logMessage("Pipeline is already running!");
        return;
    }

    cancelRequest = false;
    currentStage = InitImageListing;
    this->projectPath = projectPath;
    this->imagePath = projectPath + "/images";
    this->outputPath = projectPath + "/output";

    QDir().mkpath(outputPath);

    std::string sImagePath = imagePath.toStdString();
    std::string sMatchesDir = (outputPath + "/matches").toStdString();
    std::string sReconDir = (outputPath + "/reconstruction").toStdString();
    std::string sMVSSceneFile = (outputPath + "/scene.mvs").toStdString();

    // Hardcoded sensor database path - better to make this a setting!
    std::string sSensorDb = "/mnt/WinDisk/1_Darshan/CS/Programing/Projects/3D_Reconstruction_Software/Voxel-Forge/build/vcpkg_installed/x64-linux/share/openmvg/sensor_width_camera_database.txt";

    // Create directories
    QDir().mkpath(QString::fromStdString(sMatchesDir));
    QDir().mkpath(QString::fromStdString(sReconDir));

    emit logMessage("=== Starting OpenMVG/OpenMVS Pipeline ===");
    emit logMessage("Project path: " + projectPath);
    emit logMessage("Image path: " + imagePath);
    emit logMessage("Output path: " + outputPath);

    // Stage 1: Image Listing
    if (!cancelRequest)
    {
        currentStage = InitImageListing;
        emit logMessage("\n[Stage 1/5] Initializing image listing...");

        auto logCb = [this](const std::string &msg)
        {
            emit logMessage(QString::fromStdString(msg));
        };

        bool success = OpenMVG_Wrappers::RunImageListing(
            sImagePath,
            sMatchesDir,
            sSensorDb,
            logCb);

        if (!success)
        {
            emit logMessage("ERROR: Image listing failed!");
            currentStage = Error;
            emit pipelineFinished(false);
            return;
        }
        emit logMessage("[Stage 1/5] Image listing completed successfully!");
    }

    // Stage 2: Compute Features
    if (!cancelRequest)
    {
        currentStage = ComputeFeatures;
        emit logMessage("\n[Stage 2/5] Computing features...");

        auto logCb = [this](const std::string &msg)
        {
            emit logMessage(QString::fromStdString(msg));
        };

        std::string sSfmDataFilename = sMatchesDir + "/sfm_data.json";
        bool success = OpenMVG_Wrappers::RunComputeFeatures(
            sSfmDataFilename,
            sMatchesDir,
            logCb);

        if (!success)
        {
            emit logMessage("ERROR: Feature computation failed!");
            currentStage = Error;
            emit pipelineFinished(false);
            return;
        }
        emit logMessage("[Stage 2/5] Feature computation completed successfully!");
    }

    // Additional stages would go here (ComputeMatches, GeometricFilter, GlobalSfM, etc.)
    // For now, just finish the pipeline
    if (!cancelRequest)
    {
        currentStage = Finished;
        emit logMessage("\n=== Pipeline completed successfully! ===");
        emit pipelineFinished(true);
    }
    else
    {
        emit logMessage("\nPipeline cancelled by user.");
        emit pipelineFinished(false);
    }

    currentStage = Idle;
}

void PhotogrammetryController::cancelPipeline()
{
    cancelRequest = true;
    emit logMessage("Cancelling pipeline...");
}
