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
#include <iostream>
#include <QTextEdit> // added for the log viewer
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

// note: ptrname->fxn() is same as (*ptrname).fxn()
QString userFilesPath = QDir::homePath() + "/Voxel-Forge/";

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
        for (int i = 0; i < folders.size(); i++)
        {
            std::cout << folders[i].toStdString() << std::endl;
        }
        return folders;
    }
};

class Image
{
private:
    QString imageFilePath;
    QString name;
    QSize size;

public:
    Image(const QString &imageFilePath, const QSize &size) : imageFilePath(imageFilePath), size(size) {}
    QString getimageFilePath() const { return imageFilePath; }
    QSize getSize() const { return size; }
};

class ImageManager : private Project
{
private:
    int imageCount;
    QVector<Image> images;

public:
    ImageManager() : imageCount(0) {}
    void addImage(const Image &image)
    {
        images.append(image);
        imageCount++;
    }
    int getImageCount() const { return imageCount; }
    QVector<Image> getImages() const { return images; }
};

class PhotogrammetryController : public QObject
{
    Q_OBJECT

public:
    enum PipelineStage
    {
        Idle,
        InitImageListing,
        ComputeFeatures,
        ComputeMatches,
        GeometricFilter,
        GlobalSfM,
        ConvertToPLYModel,
        ExportToMVS,
        // First OpenMVS step
        Densify,
        ReconstructMesh,
        RefineMesh,
        TextureMesh,
        Finished,
        Error
    };

    explicit PhotogrammetryController(QObject *parent = nullptr) : QObject(parent), process(new QProcess(this)), currentStage(Idle)
    {
        // connect all necessary signals from the QProcess object
        connect(process, &QProcess::readyReadStandardOutput, this, &PhotogrammetryController::handleProcessOutput);
        connect(process, &QProcess::readyReadStandardError, this, &PhotogrammetryController::handleProcessError);
        connect(process, &QProcess::errorOccurred, this, &PhotogrammetryController::handleProcessStartError);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &PhotogrammetryController::handleProcessFinished);
    }
    // cancel the running pipeline
    void cancelPipeline()
    {
        if (process && process->state() != QProcess::NotRunning)
        {
            process->kill();                // forcefully terminate
            process->waitForFinished(3000); // wait up to 3s for it to die
            emit logMessage("<font color='orange'><b>Pipeline was cancelled by user.</b></font>");
            currentStage = Idle;
            emit pipelineFinished(false);
        }
    }

    void startPipeline(const QString &projectPath)
    {
        if (currentStage != Idle)
        {
            emit logMessage("Pipeline is already running.");
            return;
        }
        this->projectPath = projectPath;
        this->imagePath = projectPath + "/images";
        this->outputPath = projectPath + "/output";

        QDir().mkpath(outputPath);

        emit logMessage("<b>Starting OpenMVG Pipeline...</b>");
        currentStage = InitImageListing;
        executeNextStage();
    }

signals:
    void logMessage(const QString &message);
    void pipelineFinished(bool success);

private slots:
    void handleProcessOutput() { emit logMessage(process->readAllStandardOutput()); }
    void handleProcessError() { emit logMessage("<font color='red'>" + process->readAllStandardError() + "</font>"); }

    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        if (exitStatus == QProcess::CrashExit || exitCode != 0)
        {
            emit logMessage("<font color='red'><b>Pipeline failed.</b></font>");
            currentStage = Error;
            emit pipelineFinished(false);
        }
        else
        {
            currentStage = static_cast<PipelineStage>(static_cast<int>(currentStage) + 1);
            if (currentStage == Finished)
            {
                emit logMessage("<font color='green'><b>OpenMVG pipeline completed successfully.</b></font>");
                emit pipelineFinished(true);
                currentStage = Idle;
            }
            else
            {
                executeNextStage();
            }
        }
    }

    void handleProcessStartError(QProcess::ProcessError error)
    {
        QString errorMessage;
        switch (error)
        {
        case QProcess::FailedToStart:
            errorMessage = "The process failed to start. Check if the executable exists and you have permissions.";
            break;
        case QProcess::Crashed:
            errorMessage = "The process crashed after starting.";
            break;
        case QProcess::Timedout:
            errorMessage = "The process timed out.";
            break;
        default:
            errorMessage = "An unknown error occurred with the process.";
            break;
        }
        emit logMessage("<font color='red'><b>PROCESS ERROR: " + errorMessage + "</b></font>");
        currentStage = Error;
        emit pipelineFinished(false);
    }

private:
    void executeNextStage()
    {
        QString program;
        QStringList arguments;
        QString sensorDb = "/home/mr-quantum/openMVG/src/openMVG/exif/sensor_width_database/sensor_width_camera_database.txt";
        QString matchesDir = outputPath + "/matches";

        // Define paths for OpenMVS files for clarity
        QString mvsScene = outputPath + "/scene.mvs";
        QString mvsDense = outputPath + "/scene_dense.mvs";
        QString mvsMesh = outputPath + "/scene_mesh.mvs";
        QString mvsRefinedMesh = outputPath + "/scene_mesh_refined.mvs";
        QString mvsTexturedMesh = outputPath + "/scene_mesh_refined_textured.mvs";

        switch (currentStage)
        {
        case InitImageListing:
            emit logMessage("--- Stage 1: Image Listing ---");
            QDir().mkpath(matchesDir);
            program = "openMVG_main_SfMInit_ImageListing";
            arguments << "-i" << imagePath << "-o" << matchesDir << "-d" << sensorDb;
            //  add the focal length here if  images lack EXIF data
            // arguments << "-f" << "2304";
            break;
        case ComputeFeatures:
            emit logMessage("--- Stage 2: Feature Computation ---");
            program = "openMVG_main_ComputeFeatures";
            arguments << "-i" << matchesDir + "/sfm_data.json" << "-o" << matchesDir << "-p" << "NORMAL";
            break;
        case ComputeMatches:
            emit logMessage("--- Stage 3: Match Computation (Putative) ---");
            program = "openMVG_main_ComputeMatches";
            arguments << "-i" << matchesDir + "/sfm_data.json" << "-o" << matchesDir + "/matches.putative.bin";
            break;
        case GeometricFilter:
            emit logMessage("--- Stage 4: Geometric Filtering ---");
            program = "openMVG_main_GeometricFilter";
            arguments << "-i" << matchesDir + "/sfm_data.json"
                      << "-m" << matchesDir + "/matches.putative.bin"
                      << "-o" << matchesDir + "/matches.f.bin";
            break;
        case GlobalSfM:
            emit logMessage("--- Stage 5: Global Reconstruction ---");
            QDir().mkpath(outputPath + "/reconstruction/global");
            program = "openMVG_main_SfM";
            arguments << "--sfm_engine" << "GLOBAL"
                      << "-i" << matchesDir + "/sfm_data.json"
                      << "--match_file" << matchesDir + "/matches.f.bin"
                      << "-o" << outputPath + "/reconstruction/global";
            break;
        case ConvertToPLYModel:
            emit logMessage("--- Stage 6: Converting to PLY Model ---");
            program = "openMVG_main_ComputeSfM_DataColor";
            QDir().mkpath(outputPath + "/reconstruction/global/sparse_3D_model");
            arguments << "-i" << outputPath + "/reconstruction/global/sfm_data.bin"
                      << "-o" << outputPath + "/reconstruction/global/sparse_3D_model/model.ply";
            break;

        case ExportToMVS:
            emit logMessage("--- Stage 7: Exporting to OpenMVS ---");
            // This is an OpenMVG tool, so we use its path
            program = "openMVG_main_openMVG2openMVS";
            arguments << "-i" << outputPath + "/reconstruction/global/sfm_data.bin"
                      << "-o" << mvsScene
                      << "-d" << matchesDir;
            break;

        // --- OpenMVS Stages with Full Paths ---
        case Densify:
            emit logMessage("--- Stage 7: Densifying Point Cloud ---");
            program = "/usr/local/bin/OpenMVS/DensifyPointCloud";
            arguments << mvsScene << "--working-folder" << outputPath;
            break;
        case ReconstructMesh:
            emit logMessage("--- Stage 8: Reconstructing Mesh ---");
            program = "/usr/local/bin/OpenMVS/ReconstructMesh";
            arguments << mvsDense << "--working-folder" << outputPath;
            break;
        case RefineMesh:
            emit logMessage("--- Stage 9: Refining Mesh ---");
            program = "/usr/local/bin/OpenMVS/RefineMesh";
            arguments << mvsMesh << "--working-folder" << outputPath;
            break;
        case TextureMesh:
            emit logMessage("--- Stage 10: Texturing Mesh ---");
            program = "/usr/local/bin/OpenMVS/TextureMesh";
            arguments << mvsRefinedMesh << "--working-folder" << outputPath;
            break;

        default:
            return;
        }

        QString resolvedProgram = program;
        if (!QFileInfo(program).isAbsolute())
        {
            QString found = QStandardPaths::findExecutable(program);
            if (!found.isEmpty())
            {
                resolvedProgram = found;
            }
        }

        QFileInfo exeInfo(resolvedProgram);
        if (!exeInfo.exists())
        {
            emit logMessage("<font color='red'><b>PROCESS ERROR: Executable not found: " + resolvedProgram + "</b></font>");
            currentStage = Error;
            emit pipelineFinished(false);
            return;
        }
        if (!exeInfo.isExecutable())
        {
            emit logMessage("<font color='red'><b>PROCESS ERROR: Executable is not marked executable: " + resolvedProgram + "</b></font>");
            currentStage = Error;
            emit pipelineFinished(false);
            return;
        }

        emit logMessage("Starting: " + exeInfo.absoluteFilePath() + " " + arguments.join(' '));
        process->start(resolvedProgram, arguments);
    }

    QProcess *process;
    PipelineStage currentStage;
    QString projectPath, imagePath, outputPath;
};

class ExportModel{};
class 

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

    // controller and pipeline connections
    PhotogrammetryController *photoController = new PhotogrammetryController(&window);

    QObject::connect(runPipelineButton, &QPushButton::clicked, [&]()
                     {
        if (projectFullPath.isEmpty()) {
            logViewer->setHtml("<font color='red'>Error: No project selected.</font>");
            return;
        }
        logViewer->clear();
        runPipelineButton->setEnabled(false); // prevent multiple clicks
        cancelPipelineButton->setEnabled(true);
        photoController->startPipeline(projectFullPath); });
    QObject::connect(photoController, &PhotogrammetryController::logMessage, logViewer, &QTextEdit::append);
    // when pipeline finishes, re-enable Run and disable Cancel
    QObject::connect(photoController, &PhotogrammetryController::pipelineFinished, [&](bool success)
                     {
        runPipelineButton->setEnabled(true);
        cancelPipelineButton->setEnabled(false); });
    // Cancel button wiring
    QObject::connect(cancelPipelineButton, &QPushButton::clicked, [&]()
                     {
        cancelPipelineButton->setEnabled(false);
        photoController->cancelPipeline(); });

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
