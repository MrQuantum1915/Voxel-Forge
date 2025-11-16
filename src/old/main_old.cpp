// Copyright Darshan Patel [Mr.Quantum_1915]:)

// // not using "using namespace std;" to follow best practices.
// #include <string>
// #include <filesystem>

// // Qt
// #include <QApplication>
// #include <QPushButton>
// #include <QWidget>
// #include <QVBoxLayout>
// #include <QProcess>
// #include <QMenuBar>
// #include <QMainWindow>
// #include <QListWidget>
// #include <QHBoxLayout>
// #include <QComboBox>
// #include <QStackedWidget>
// #include <QLabel>
// #include <QStyleFactory>
// #include <QFileDialog>
// #include <QDir>
// #include <QVector>
// #include <QDebug>
// #include <QTextEdit> // added for the log viewer
// #include <QFile>
// #include <QFileInfo>
// #include <QStandardPaths>

// // threading to keep UI responsive while reconstruction is running
// #include <QThread>
// #include <atomic> // for cancel button logic

// // openmvs
// #include <openmvs/MVS.h> // main openmvs scene header

// #include <opencv2/core.hpp>

// // openmvg
// #include "openMVG/sfm/sfm_data.hpp"
// #include "openMVG/sfm/sfm_data_io.hpp"
// #include "openMVG/sfm/pipelines/global/sfm_global_engine_relative_motions.hpp"
// #include "openMVG/features/feature.hpp"

// // for stage 3 of openMVG
// #include "openMVG/matching/indMatch.hpp"
// #include "openMVG/matching/indMatch_utils.hpp"
// #include "openMVG/matching_image_collection/Pair_Builder.hpp"
// #include "openMVG/matching_image_collection/Matcher_Regions.hpp"
// #include "openMVG/matching_image_collection/Cascade_Hashing_Matcher_Regions.hpp"

// // for stage 4 of openMVG
// #include "openMVG/matching_image_collection/GeometricFilter.hpp"
// #include "openMVG/matching_image_collection/F_ACRobust.hpp"

// // for stage 5 of openMVG
// #include "openMVG/sfm/pipelines/global/sfm_global_engine_relative_motions.hpp"
// #include "openMVG/sfm/sfm_data_triangulation.hpp" // for creating the PLY file

// // stage 7 of openMVG
// #include "openmvs/MVS/Interface.h"
// #include "openMVG/cameras/Camera_Pinhole.hpp"
// #include "openMVG/cameras/Camera_undistort_image.hpp"
// #include "openMVG/image/image_io.hpp"

// // For multi-threading the undistort step
// #ifdef OPENMVG_USE_OPENMP
// #include <omp.h>
// #endif

// #include "openMVG/sfm/sfm_data_triangulation.hpp"
// #include "openMVG/sfm/pipelines/sfm_engine.hpp"
// #include "openMVG/sfm/pipelines/sfm_regions_provider.hpp"
// #include "openMVG/matching_image_collection/Matcher.hpp"
// #include "openMVG/matching_image_collection/GeometricFilter.hpp"
// #include "openMVG/system/timer.hpp"

/////////////////////////////////------2////////////////////////////////
// --- START NON-QT BLOCK ---
// C++ Standard Libraries
// --- START NON-QT BLOCK ---
// C++ Standard Libraries
#include <string>
#include <atomic> // for cancel button logic

// --- FIX FOR MACRO CONFLICT ---
// Qt or a system header defines D2R/R2D as macros.
// We must undefine them *before* MVS.h includes OpenMVG's numeric.h.

// --- END FIX ---

// OpenMVS (MUST BE FIRST to prevent OpenCV redefinition errors)
#include <openmvs/MVS.h> // Main OpenMVS scene header

#undef D2R // because it was causing macro conflict as there are different definitions of this macro in std and ohter lib
#undef R2D

// OpenCV (Must be AFTER OpenMVS)
#include <opencv2/core.hpp>

#include "openmvg_wrappers.hpp"

// OpenMVG
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
#include "openMVG/system/loggerprogress.hpp" // For progress bars
#include "openMVG/third_party/stlplus3/filesystemSimplified/file_system.hpp"

// OpenMP
#ifdef OPENMVG_USE_OPENMP
#include <omp.h>
#endif
// --- END NON-QT BLOCK ---

// --- QT INCLUDES ---
// (All Qt headers come AFTER the libraries above)
#include <QApplication>
#include <QPushButton>
#include <QWidget>
#include <QVBoxLayout>
#include <QProcess>
#include <QMenuBar>
#include <QMainWindow>
#include <QListWidget>
#include <QHBoxLayout>
#include <QComboBox>
#include <QStackedWidget>
#include <QLabel>
#include <QStyleFactory>
#include <QFileDialog>
#include <QDir>
#include <QVector>
#include <QDebug>
#include <QTextEdit>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QThread>  // Must be with Qt includes
#include <QPointer> // For safe pointer handling in async callbacks

// video frame extraction
#include <opencv2/opencv.hpp> // for video to frame extraction
// note: ptrname->fxn() is same as (*ptrname).fxn()

const QString userFilesPath = QDir::homePath() + "/Voxel-Forge/";

class AllProjects
{
public:
    QVector<QString> getAllProjects()
    {
        QVector<QString> projects;
        QDir directory(userFilesPath);
        if (directory.exists())
        {
            QStringList dirs = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString &projectName : dirs)
            {
                projects.push_back(projectName);
            }
        }
        return projects;
    }
};

class Project
{
private:
    QString name;

public:
    QVector<QString> getAllFolders()
    {
        QDir directory(userFilesPath + name);
        QVector<QString> folders;
        if (directory.exists())
        {
            QStringList dirs = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString folder : dirs)
            {
                folders.push_back(folder);
            }
        }
        return folders;
    }
};

class ProjectImage
{
private:
    QString imageFilePath;
    QString name;
    QSize size;

public:
    ProjectImage(const QString &imageFilePath, const QSize &size)
    {
        this->imageFilePath = imageFilePath;
        name = QFileInfo(imageFilePath).fileName();
        this->size = size;
    }
    QString getimageFilePath() const { return imageFilePath; }
    QSize getSize() const { return size; }
};

class ImageManager : private Project
{
private:
    int imageCount;
    QVector<ProjectImage> images;

public:
    ImageManager() : imageCount(0) {}
    void addImage(const ProjectImage &image)
    {
        images.append(image);
        imageCount++;
    }
    int getImageCount() const { return imageCount; }
    QVector<ProjectImage> getImages() const { return images; }
};

class Video
{
private:
    QString videoFilePath;
    QString name;
    double duration;
    double frameRate;

public:
    Video(const QString &videoFilePath)
    {
        this->videoFilePath = videoFilePath;
        name = QFileInfo(videoFilePath).fileName();
        cv::VideoCapture cap(videoFilePath.toStdString());
        if (cap.isOpened())
        {
            duration = cap.get(cv::CAP_PROP_FRAME_COUNT) / cap.get(cv::CAP_PROP_FPS);
            frameRate = cap.get(cv::CAP_PROP_FPS);
        }
        else
        {
            duration = 0;
            frameRate = 0;
        }
    }

    QString getVideoFilePath() const { return videoFilePath; }
    double getDuration() const { return duration; }
    double getFrameRate() const { return frameRate; }
};

// class ExportModel
// {
// };

class VideoFrameExtractor : public QObject
{
    Q_OBJECT

public:
    explicit VideoFrameExtractor(QObject *parent = nullptr)
        : QObject(parent), cancelRequest(false) {}

public slots:
    void extractFrames(const QString &videoFilePath, const QString &projectFullPath)
    {
        QString imageDir = projectFullPath + "/images";
        QDir().mkpath(imageDir);

        emit logMessage("Extracting frames from video: " + videoFilePath);
        emit logMessage("Frames will be saved to: " + imageDir);

        // extract frames using OpenCV
        cv::VideoCapture cap(videoFilePath.toStdString());
        if (!cap.isOpened())
        {
            emit logMessage("<font color='red'>Failed to open video file: " + videoFilePath + "</font>");
            emit extractionFinished(false);
            return;
        }

        int frameIndex = 0;
        cv::Mat frame;
        while (cap.read(frame) && !cancelRequest)
        {
            QString frameFileName = QString("frame_%1.jpg").arg(frameIndex, 5, 10, QChar('0'));
            QString destPath = imageDir + "/" + frameFileName;

            if (QFile::exists(destPath))
            {
                emit logMessage("Skipping existing file: " + destPath);
                frameIndex++;
                continue;
            }

            if (!cv::imwrite(destPath.toStdString(), frame))
            {
                emit logMessage("<font color='red'>Failed to write frame to " + destPath + "</font>");
                break;
            }

            // Emit signal to add item to list widget (on main thread)
            emit frameExtracted(frameFileName, destPath);
            frameIndex++;
        }

        if (cancelRequest)
        {
            emit logMessage("<font color='orange'>Frame extraction cancelled by user.</font>");
            emit extractionFinished(false);
        }
        else
        {
            emit logMessage("Extracted " + QString::number(frameIndex) + " frames to " + imageDir);
            emit extractionFinished(true);
        }

        cancelRequest = false;
    }

    void cancelExtraction()
    {
        cancelRequest = true;
    }

signals:
    void logMessage(const QString &message);
    void frameExtracted(const QString &fileName, const QString &fullPath);
    void extractionFinished(bool success);

private:
    std::atomic<bool> cancelRequest;
};

class PhotogrammetryController : public QObject
{
    Q_OBJECT

public:
    enum PipelineStage // better than using strings :)
    {
        Idle,
        InitImageListing,
        ComputeFeatures,
        ComputeMatches,
        GeometricFilter,
        GlobalSfM,
        // ConvertToPLYModel, // Note: this stage is now just part of GlobalSfM
        ExportToMVS,
        Densify,
        ReconstructMesh,
        RefineMesh,
        TextureMesh,
        Finished,
        Error
    };

    // explicit prevents the compiler from using this constructor for implicit conversions. Like accidentally converting the data type of the argument to the class object it self.
    explicit PhotogrammetryController(QObject *parent = nullptr)
        : QObject(parent), currentStage(Idle), cancelRequest(false) {}

    // will run on worker thread (main thread for GUI)
public slots: // simply its  a list of member functions then are accesed when a specific signal is emited (we connect signal to this member functions for tasks to be done)
    void startPipeline(const QString &projectPath)
    {
        if (currentStage != Idle)
        {
            emit logMessage("Pipeline is already running.");
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

        // harcoded. better make this a setting!
        std::string sSensorDb = "/mnt/WinDisk/1_Darshan/CS/Programing/Projects/3D_Reconstruction_Software/Voxel-Forge/build/vcpkg_installed/x64-linux/share/openmvg/sensor_width_camera_database.txt";

        // create directories
        // mk folder using QDir
        QDir().mkpath(QString::fromStdString(sMatchesDir));
        QDir().mkpath(QString::fromStdString(sReconDir));

        // // hold scene data in memory - no need because one benifit of writing to disk is we can resume from any stage if something fails
        // openMVG::sfm::SfM_Data sfm_data;
        // MVS::Scene mvs_scene;

        emit logMessage("<b>Behold! Reconstruction Pipeline is about to begin...</b>");

        try
        {
            while (currentStage != Finished && currentStage != Error && !cancelRequest)
            {
                auto logCallback = [this](const std::string &msg)
                {
                    emit logMessage(QString::fromStdString(msg));
                };

                switch (currentStage)
                {
                    // stage 1 - image listing
                case InitImageListing:
                {
                    emit logMessage("--- Stage 1: Image Listing ---");

                    if (!OpenMVG_Wrappers::RunImageListing(
                            sImagePath,
                            sReconDir,
                            sSensorDb,
                            logCallback))
                    {
                        throw std::runtime_error("Image Listing failed.");
                    }

                    // // Load the generated sfm_data.json -no need because we are not using in-memory sfm_data

                    // std::string sSfM_Data_Filename = sReconDir + "/sfm_data.json";
                    // if (!Load(sfm_data, sSfM_Data_Filename, openMVG::sfm::ESfM_Data(openMVG::sfm::VIEWS | openMVG::sfm::INTRINSICS)))
                    // {
                    //     throw std::runtime_error("Failed to load sfm_data.json");
                    // }
                    emit logMessage("✓ Image Listing complete.");
                    currentStage = ComputeFeatures;
                    break;
                }

                // stage 2 - compute features
                case ComputeFeatures:
                {
                    emit logMessage("---Stage 2: Compute Features ---");

                    std::string sSfM_Data_Filename = sReconDir + "/sfm_data.json";

                    if (!OpenMVG_Wrappers::RunComputeFeatures(
                            sSfM_Data_Filename,
                            sReconDir,
                            logCallback))
                    {
                        throw std::runtime_error("Compute Features failed.");
                    }
                    emit logMessage("✓ Compute Features complete.");
                    currentStage = ComputeMatches;
                    break;
                }
                }

                if (cancelRequest)
                {
                    emit logMessage("<font color='orange'><b>Pipeline was cancelled by user.</b></font>");
                    currentStage = Idle;
                    emit pipelineFinished(false);
                }
                else if (currentStage == Finished)
                {
                    emit logMessage("<font color='green'><b>Pipeline completed successfully.</b></font>");
                    currentStage = Idle;
                    emit pipelineFinished(true);
                }
            }
        }
        catch (const std::exception &e)
        {
            emit logMessage("<font color='red'><b>PIPELINE ERROR: " + QString(e.what()) + "</b></font>");
            currentStage = Error;
            emit pipelineFinished(false);
        }
        catch (...)
        {
            emit logMessage("<font color='red'><b>An unknown critical error occurred.</b></font>");
            currentStage = Error;
            emit pipelineFinished(false);
        }
    }

    void cancelPipeline()
    {
        cancelRequest = true;
    }
signals:
    void logMessage(const QString &message);
    void pipelineFinished(bool success);

private:
    std::atomic<bool> cancelRequest;
    PipelineStage currentStage;
    QString projectPath, imagePath, outputPath;
};

class PipelineStarter : public QObject
{
    Q_OBJECT
signals:
    void requestStartPipeline(const QString &projectPath);
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString currentProject = nullptr;
    // full path to the selected project (either from AllProjects or opened folder)
    QString projectFullPath;

    QMainWindow window;
    window.setWindowTitle("Voxel Forge");
    QWidget *centralWidget = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    QListWidget *sidebar = new QListWidget;
    sidebar->addItem("Home");
    sidebar->addItem("AllProjects");
    sidebar->addItem("Image Manager");
    sidebar->addItem("Settings");
    sidebar->setFixedWidth(150);
    mainLayout->addWidget(sidebar);

    QStackedWidget *stackedContent = new QStackedWidget;

    QWidget *homePage = new QWidget;
    QVBoxLayout *homeLayout = new QVBoxLayout(homePage);
    QLabel *homeLabel = new QLabel("Welcome to Voxel Forge");
    homeLabel->setAlignment(Qt::AlignCenter);
    homeLayout->addWidget(homeLabel);
    homeLayout->addStretch();

    QWidget *settingsPage = new QWidget;
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsPage);
    QLabel *themeLabel = new QLabel("Select Theme:");
    QComboBox *themeCombo = new QComboBox;
    themeCombo->addItem("Black");
    themeCombo->addItem("White");
    settingsLayout->addWidget(themeLabel);
    settingsLayout->addWidget(themeCombo);
    settingsLayout->addStretch();

    AllProjects p;
    const QVector<QString> Allprojects = p.getAllProjects();
    // (imagesListWidget and logViewer are declared later where the Image Manager UI is built)
    QWidget *AllprojectsPage = new QWidget;
    QVBoxLayout *AllprojectsLayout = new QVBoxLayout(AllprojectsPage);
    QLabel *AllprojectsLabel = new QLabel("AllProjects");
    AllprojectsLabel->setAlignment(Qt::AlignCenter);
    AllprojectsLayout->addWidget(AllprojectsLabel);
    QListWidget *projectList = new QListWidget;
    AllprojectsLayout->addWidget(projectList);
    AllprojectsLayout->addStretch();
    for (const QString &projectName : Allprojects)
    {
        projectList->addItem(projectName);
    }
    QPushButton *selectProjectButton = new QPushButton("Select Project");
    AllprojectsLayout->addWidget(selectProjectButton, 0, Qt::AlignCenter);
    AllprojectsLayout->addStretch();
    QLabel *selectedProjectLabel = new QLabel;
    selectedProjectLabel->setAlignment(Qt::AlignCenter);
    AllprojectsLayout->addWidget(selectedProjectLabel);
    selectedProjectLabel->hide();
    // NOTE: selectProjectButton handler is attached after the Image Manager widgets are created so variables are in-scope

    // NOTE: openProjectAction connect is attached after the action is created below (so variables are in-scope)

    QWidget *imageManagerPage = new QWidget;
    QVBoxLayout *imageManagerLayout = new QVBoxLayout(imageManagerPage);

    QHBoxLayout *topControlsLayout = new QHBoxLayout();
    QPushButton *addImageButton = new QPushButton("Add Images");
    QPushButton *addVideoButton = new QPushButton("Add Video");
    QPushButton *cancelVideoButton = new QPushButton("Cancel Extraction");
    cancelVideoButton->setStyleSheet(
        "QPushButton {"
        "  border-radius: 18px;"
        "  padding: 8px 24px;"
        "  background-color: #df1a13ff;"
        "  color: white;"
        "  font-weight: bold;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #888;"
        "  color: #ccc;"
        "}");
    cancelVideoButton->setEnabled(false);
    QPushButton *runPipelineButton = new QPushButton("Run OpenMVG Pipeline");
    runPipelineButton->setStyleSheet(
        "QPushButton {"
        "  border-radius: 18px;"
        "  padding: 8px 24px;"
        "  background-color: #4400ffff;"
        "  color: white;"
        "  font-weight: bold;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #888;"
        "  color: #ccc;"
        "}");
    runPipelineButton->setMinimumHeight(36);

    QPushButton *cancelPipelineButton = new QPushButton("Cancel Pipeline");
    cancelPipelineButton->setStyleSheet(
        "QPushButton {"
        "  border-radius: 18px;"
        "  padding: 8px 24px;"
        "  background-color: #df1a13ff;"
        "  color: white;"
        "  font-weight: bold;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #888;"
        "  color: #ccc;"
        "}");
    cancelPipelineButton->setEnabled(false);
    topControlsLayout->addWidget(addImageButton);
    topControlsLayout->addWidget(addVideoButton);
    topControlsLayout->addWidget(cancelVideoButton);
    topControlsLayout->addWidget(runPipelineButton);
    topControlsLayout->addWidget(cancelPipelineButton);
    imageManagerLayout->addLayout(topControlsLayout);

    QHBoxLayout *middleDisplayLayout = new QHBoxLayout();
    QListWidget *imagesListWidget = new QListWidget;
    middleDisplayLayout->addWidget(imagesListWidget);

    QLabel *imagePreviewLabel = new QLabel("Image Preview");
    imagePreviewLabel->setMinimumSize(400, 300);
    imagePreviewLabel->setAlignment(Qt::AlignCenter);
    imagePreviewLabel->setStyleSheet("border: 1px solid gray;");
    middleDisplayLayout->addWidget(imagePreviewLabel);
    imageManagerLayout->addLayout(middleDisplayLayout);

    // bottom log viewer for process output
    QTextEdit *logViewer = new QTextEdit();
    logViewer->setReadOnly(true);
    logViewer->setFontFamily("Courier");
    imageManagerLayout->addWidget(logViewer);

    QObject::connect(imagesListWidget, &QListWidget::itemClicked, [&](QListWidgetItem *item)
                     {
                         QString imageFilePath = item->data(Qt::UserRole).toString(); // use UserRole for full path
                         QPixmap pixmap(imageFilePath);
                         if (!pixmap.isNull())
                         {
                             imagePreviewLabel->setPixmap(pixmap.scaled(imagePreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                         }
                         else
                         {
                             imagePreviewLabel->setText("Failed to load image.");
                         } });

    // as now the imagesListWidget and logViewer exist, wire the selectProjectButton behavior
    QObject::connect(selectProjectButton, &QPushButton::clicked, [&]()
                     {
            QListWidgetItem *currentItem = projectList->currentItem();
            if (currentItem)
            {
                currentProject = currentItem->text();
                projectFullPath = userFilesPath + currentProject;
                selectedProjectLabel->setText("Selected Project: " + currentProject + "\n" + projectFullPath);
                selectedProjectLabel->show();
                // load images from project
                imagesListWidget->clear();
                QString imageDir = projectFullPath + "/images";
                QDir d(imageDir);
                if (d.exists()) {
                    QStringList images = d.entryList(QStringList() << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.webp", QDir::Files, QDir::Name);
                    for (const QString &img : images) {
                        QString full = imageDir + "/" + img;
                        QListWidgetItem *item = new QListWidgetItem(img);
                        item->setData(Qt::UserRole, full);
                        imagesListWidget->addItem(item);
                    }
                    logViewer->append("Loaded " + QString::number(images.size()) + " images from " + imageDir);
                } else {
                    logViewer->append("<font color='orange'>No images directory found at " + imageDir + "</font>");
                }
            } });
    QObject::connect(addImageButton, &QPushButton::clicked, [&]()
                     {
                         if (projectFullPath.isEmpty()) {
                             logViewer->append("<font color='red'>Please select or open a project first.</font>");
                             return;
                         }
                         QStringList fileNames = QFileDialog::getOpenFileNames(&window, "Select Images", "", "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
                         if (!fileNames.isEmpty())
                         {
                             QString imageDir = projectFullPath + "/images";
                             QDir().mkpath(imageDir); // ensure image directory exists
                             for (const QString &sourcePath : fileNames)
                             {
                                 QFileInfo fileInfo(sourcePath);
                                 QString destPath = imageDir + "/" + fileInfo.fileName();
                                 if (QFile::exists(destPath)) {
                                     logViewer->append("Skipping existing file: " + destPath);
                                     continue;
                                 }
                                 if (!QFile::copy(sourcePath, destPath)) {
                                     logViewer->append("<font color='red'>Failed to copy " + sourcePath + " to " + destPath + "</font>");
                                     continue;
                                 }

                                 QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
                                 item->setData(Qt::UserRole, destPath); // store full path
                                 imagesListWidget->addItem(item);
                             }
                             logViewer->append("Added " + QString::number(fileNames.size()) + " files to " + imageDir);
                         } });

    // video extraction worker thread setup
    QThread *videoWorkerThread = new QThread;
    VideoFrameExtractor *videoExtractor = new VideoFrameExtractor;
    videoExtractor->moveToThread(videoWorkerThread);

    // Use QPointer for safe cross-thread widget access
    QPointer<QTextEdit> safeLogViewer(logViewer);
    QPointer<QListWidget> safeImagesListWidget(imagesListWidget);
    QPointer<QPushButton> safeAddVideoButton(addVideoButton);
    QPointer<QPushButton> safeCancelVideoButton(cancelVideoButton);

    // CRITICAL: Use Qt::QueuedConnection for cross-thread GUI updates - other wise software crashes with segfault
    QObject::connect(videoExtractor, &VideoFrameExtractor::logMessage, logViewer, &QTextEdit::append, Qt::QueuedConnection);
    QObject::connect(videoExtractor, &VideoFrameExtractor::frameExtracted, &window, [safeImagesListWidget](const QString &fileName, const QString &fullPath)
                     {
        if (safeImagesListWidget) {
            QListWidgetItem *item = new QListWidgetItem(fileName);
            item->setData(Qt::UserRole, fullPath);
            safeImagesListWidget->addItem(item);
        } }, Qt::QueuedConnection);
    QObject::connect(videoExtractor, &VideoFrameExtractor::extractionFinished, &window, [safeAddVideoButton, safeCancelVideoButton, safeLogViewer](bool success)
                     {
        if (safeAddVideoButton) safeAddVideoButton->setEnabled(true);
        if (safeCancelVideoButton) safeCancelVideoButton->setEnabled(false);
        if (success && safeLogViewer) {
            safeLogViewer->append("<font color='green'><b>✓ Video frame extraction complete.</b></font>");
        } }, Qt::QueuedConnection);

    // cancel button - also use queued connection for thread safety
    QObject::connect(cancelVideoButton, &QPushButton::clicked, videoExtractor, &VideoFrameExtractor::cancelExtraction, Qt::QueuedConnection);

    // cleanup video worker thread
    QObject::connect(&app, &QCoreApplication::aboutToQuit, videoWorkerThread, &QThread::quit);
    QObject::connect(videoWorkerThread, &QThread::finished, videoExtractor, &QObject::deleteLater);
    QObject::connect(videoWorkerThread, &QThread::finished, videoWorkerThread, &QObject::deleteLater);

    videoWorkerThread->start();

    QObject::connect(addVideoButton, &QPushButton::clicked, [&, videoExtractor]()
                     {
                         if (projectFullPath.isEmpty()) {
                             logViewer->append("<font color='red'>Please select or open a project first.</font>");
                             return;
                         }
                         QString videoFile = QFileDialog::getOpenFileName(&window, "Select Video", "", "Videos (*.mp4 *.avi *.mov *.mkv)");
                        
                         if (!videoFile.isEmpty())
                         {
                            QString videoDir = projectFullPath + "/videos";
                            QDir().mkpath(videoDir);

                            QString destVideoPath = videoDir + "/" + QFileInfo(videoFile).fileName();
                            if(!QFile::copy(videoFile, destVideoPath)) {
                                logViewer->append("<font color='red'>Failed to copy video to project folder.</font>");
                                return;
                            }

                            // disable button and enable cancel during extraction
                            addVideoButton->setEnabled(false);
                            cancelVideoButton->setEnabled(true);
                            logViewer->append("<font color='blue'><b>Starting frame extraction in background...</b></font>");

                            // trigger extraction on worker thread
                            QMetaObject::invokeMethod(videoExtractor, "extractFrames", Qt::QueuedConnection,
                                Q_ARG(QString, destVideoPath),
                                Q_ARG(QString, projectFullPath));
                         } });

    // stacking and final Layout
    stackedContent->addWidget(homePage);
    stackedContent->addWidget(AllprojectsPage);
    stackedContent->addWidget(imageManagerPage);
    stackedContent->addWidget(settingsPage);
    mainLayout->addWidget(stackedContent);
    window.setCentralWidget(centralWidget);

    // menu bar
    QMenuBar *menuBar = window.menuBar();
    QMenu *fileMenu = menuBar->addMenu("File");
    QAction *openProjectAction = fileMenu->addAction("Open Project");
    QAction *exitAction = fileMenu->addAction("Exit");
    QObject::connect(exitAction, &QAction::triggered, &app, &QApplication::quit);
    QMenu *helpMenu = menuBar->addMenu("Help");
    helpMenu->addAction("About");

    // open project handler (needs imagesListWidget and logViewer which are created above)
    QObject::connect(openProjectAction, &QAction::triggered, [&]()
                     {
        QString dir = QFileDialog::getExistingDirectory(&window, "Open Project Folder", QDir::homePath());
        if (!dir.isEmpty()) {
            projectFullPath = dir;
            QFileInfo fi(dir);
            currentProject = fi.fileName();
            selectedProjectLabel->setText("Selected Project: " + currentProject + "\n" + projectFullPath);
            selectedProjectLabel->show();
            // populate images
            if (imagesListWidget) imagesListWidget->clear();
            QString imageDir = projectFullPath + "/images";
            QDir d(imageDir);
            if (d.exists()) {
                QStringList images = d.entryList(QStringList() << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.webp", QDir::Files, QDir::Name);
                for (const QString &img : images) {
                    QString full = imageDir + "/" + img;
                    QListWidgetItem *item = new QListWidgetItem(img);
                    item->setData(Qt::UserRole, full);
                    if (imagesListWidget) imagesListWidget->addItem(item);
                }
                if (logViewer) logViewer->append("Loaded " + QString::number(images.size()) + " images from " + imageDir);
            } else {
                if (logViewer) logViewer->append("<font color='orange'>No images directory found at " + imageDir + "</font>");
            }

            // scan for expected project folders
            QStringList expected = {"images", "output", "reconstruction", "matches"};
            for (const QString &name : expected) {
                QString p = projectFullPath + "/" + name;
                if (QDir(p).exists()) {
                    if (logViewer) logViewer->append("Found folder: " + p);
                } else {
                    if (logViewer) logViewer->append("Missing folder: " + p);
                }
            }
        } });

    QThread *workerThread = new QThread;

    // 2. Create the controller (with no parent)
    PhotogrammetryController *photoController = new PhotogrammetryController;

    // 3. Move the controller to the new thread
    photoController->moveToThread(workerThread);

    PipelineStarter *starter = new PipelineStarter;
    // Connect the signal to the worker's slot
    QObject::connect(starter, &PipelineStarter::requestStartPipeline,
                     photoController, &PhotogrammetryController::startPipeline);

    // --- Controller and Pipeline Connections ---
    // Connect GUI (main thread) to Controller (worker thread)
    // When "Run" is clicked, it will call the 'startPipeline' slot on the worker thread.
    QObject::connect(runPipelineButton, &QPushButton::clicked, [&, starter]()
                     {
    if (projectFullPath.isEmpty()) {
        logViewer->setHtml("<font color='red'>Error: No project selected.</font>");
        return;
    }
    logViewer->clear(); 
    runPipelineButton->setEnabled(false);
    cancelPipelineButton->setEnabled(true);
    
    // Emit signal instead of direct call
    emit starter->requestStartPipeline(projectFullPath); });

    // Connect Controller (worker thread) back to GUI (main thread)
    // These signal/slot connections are thread-safe
    QObject::connect(photoController, &PhotogrammetryController::logMessage, logViewer, &QTextEdit::append);
    QObject::connect(photoController, &PhotogrammetryController::pipelineFinished, [&](bool success)
                     {
        runPipelineButton->setEnabled(true);
        cancelPipelineButton->setEnabled(false); });

    // Connect Cancel button (main thread) to Controller slot (worker thread)
    QObject::connect(cancelPipelineButton, &QPushButton::clicked, photoController, &PhotogrammetryController::cancelPipeline);

    // Clean up the thread when the application quits
    QObject::connect(&app, &QCoreApplication::aboutToQuit, workerThread, &QThread::quit);
    QObject::connect(workerThread, &QThread::finished, photoController, &QObject::deleteLater);
    QObject::connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    // 4. Start the worker thread
    workerThread->start();

    // global connections and style
    QObject::connect(sidebar, &QListWidget::currentRowChanged, stackedContent, &QStackedWidget::setCurrentIndex);
    sidebar->setCurrentRow(0);
    auto setTheme = [&](int index)
    {
        app.setStyle(QStyleFactory::create("Fusion"));
        if (index == 0) // black
        {
            QPalette darkPalette;
            darkPalette.setColor(QPalette::Window, QColor(50, 50, 50));
            darkPalette.setColor(QPalette::WindowText, Qt::white);
            darkPalette.setColor(QPalette::Base, QColor(60, 60, 60));
            darkPalette.setColor(QPalette::AlternateBase, QColor(80, 80, 80));
            darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
            darkPalette.setColor(QPalette::ToolTipText, Qt::white);
            darkPalette.setColor(QPalette::Text, Qt::white);
            darkPalette.setColor(QPalette::Button, QColor(70, 70, 70));
            darkPalette.setColor(QPalette::ButtonText, Qt::white);
            darkPalette.setColor(QPalette::BrightText, Qt::green);
            darkPalette.setColor(QPalette::Highlight, QColor(60, 120, 200));
            darkPalette.setColor(QPalette::HighlightedText, Qt::white);
            app.setPalette(darkPalette);
        }
        else // white
        {
            app.setPalette(QApplication::style()->standardPalette());
        }
    };
    QObject::connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), setTheme);
    setTheme(0);
    window.resize(1000, 700); // increased height slightly for the log viewer
    window.show();
    return app.exec();
}

// required for Q_OBJECT classes in a single .cpp file
#include "main.moc"
