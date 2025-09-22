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
        Finished,
        Error
    };


    explicit PhotogrammetryController(QObject *parent = nullptr) : QObject(parent), process(new QProcess(this)), currentStage(Idle)
    {
        // Connect all necessary signals from the QProcess object
        connect(process, &QProcess::readyReadStandardOutput, this, &PhotogrammetryController::handleProcessOutput);
        connect(process, &QProcess::readyReadStandardError, this, &PhotogrammetryController::handleProcessError);
        connect(process, &QProcess::errorOccurred, this, &PhotogrammetryController::handleProcessStartError);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &PhotogrammetryController::handleProcessFinished);
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

        switch (currentStage)
        {
        case InitImageListing:
            emit logMessage("--- Stage 1: Image Listing ---");
            QDir().mkpath(matchesDir);
            program = "openMVG_main_SfMInit_ImageListing";
            arguments << "-i" << imagePath << "-o" << matchesDir << "-d" << sensorDb;
            // You may need to add the focal length here if your images lack EXIF data
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
            program = "openMVG_main_openMVS_Exporter";
            arguments << "-i" << outputPath + "/reconstruction/global/sfm_data.bin" << "-o" << outputPath + "/scene.mvs";
            break;
        default:
            return;
        }
        process->start(program, arguments);
    }


    QProcess *process;
    PipelineStage currentStage;
    QString projectPath, imagePath, outputPath;
};



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString currentProject = nullptr;


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
    QObject::connect(selectProjectButton, &QPushButton::clicked, [&]()
                     {
        QListWidgetItem *currentItem = projectList->currentItem();
        if (currentItem)
        {
            currentProject = currentItem->text();
            selectedProjectLabel->setText("Selected Project: " + currentProject);
            selectedProjectLabel->show();
        } });


    QWidget *imageManagerPage = new QWidget;
    QVBoxLayout *imageManagerLayout = new QVBoxLayout(imageManagerPage);


    QHBoxLayout *topControlsLayout = new QHBoxLayout();
    QPushButton *addImageButton = new QPushButton("Add Images");
    QPushButton *runPipelineButton = new QPushButton("Run OpenMVG Pipeline");
    runPipelineButton->setStyleSheet("background-color: #2a82da; color: white; font-weight: bold;");
    topControlsLayout->addWidget(addImageButton);
    topControlsLayout->addWidget(runPipelineButton);
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
                         QString imageFilePath = item->data(Qt::UserRole).toString(); // Use UserRole for full path
                         QPixmap pixmap(imageFilePath);
                         if (!pixmap.isNull())
                         {
                             imagePreviewLabel->setPixmap(pixmap.scaled(imagePreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                         }
                         else
                         {
                             imagePreviewLabel->setText("Failed to load image.");
                         } });
    QObject::connect(addImageButton, &QPushButton::clicked, [&]()
                     {
                         if (currentProject.isNull()) {
                             logViewer->append("<font color='red'>Please select a project first.</font>");
                             return;
                         }
                         QStringList fileNames = QFileDialog::getOpenFileNames(&window, "Select Images", "", "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
                         if (!fileNames.isEmpty())
                         {
                             QString imageDir = userFilesPath + currentProject + "/images";
                             QDir().mkpath(imageDir); // Ensure image directory exists
                             for (const QString &sourcePath : fileNames)
                             {
                                 QFileInfo fileInfo(sourcePath);
                                 QString destPath = imageDir + "/" + fileInfo.fileName();
                                 QFile::copy(sourcePath, destPath);


                                 QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
                                 item->setData(Qt::UserRole, destPath); // Store full path
                                 imagesListWidget->addItem(item);
                             }
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


    // controller and pipeline connections
    PhotogrammetryController *photoController = new PhotogrammetryController(&window);


    QObject::connect(runPipelineButton, &QPushButton::clicked, [&]()
                     {
        if (currentProject.isNull()) {
            logViewer->setHtml("<font color='red'>Error: No project selected.</font>");
            return;
        }
        logViewer->clear();
        runPipelineButton->setEnabled(false); // prevent multiple clicks
        photoController->startPipeline(userFilesPath + currentProject); });
    QObject::connect(photoController, &PhotogrammetryController::logMessage, logViewer, &QTextEdit::append);
    QObject::connect(photoController, &PhotogrammetryController::pipelineFinished, runPipelineButton, &QPushButton::setEnabled);


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
// #include <iostream>
// #include <QObject>
// // note: ptrname->fxn() is same as (*ptrname).fxn()

// QString userFilesPath = QDir::homePath() + "/Voxel-Forge/";

// class AllProjects
// {
// public:
//     QVector<QString> getAllProjects()
//     {
//         QVector<QString> projects;

//         QDir directory(userFilesPath);
//         if (directory.exists())
//         {
//             QStringList dirs = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

//             for (const QString &projectName : dirs)
//             {
//                 projects.push_back(projectName);
//             }
//         }
//         return projects;
//     }
// };
// class Project
// {
// private:
//     QString name;

// public:
//     QVector<QString> getAllFolders()
//     {
//         QDir directory(userFilesPath + name);
//         QVector<QString> folders;
//         if (directory.exists())
//         {
//             QStringList dirs = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
//             for (const QString folder : dirs)
//             {
//                 folders.push_back(folder);
//             }
//         }
//         for (int i = 0; i < folders.size(); i++)
//         {
//             std::cout << folders[i].toStdString() << std::endl;
//         }

//         return folders;
//     }
// };

// class Image
// {
// private:
//     QString imageFilePath;
//     QString name;
//     QSize size;

// public:
//     Image(const QString &imageFilePath, const QSize &size) : imageFilePath(imageFilePath), size(size) {}
//     QString getimageFilePath() const { return imageFilePath; }
//     QSize getSize() const { return size; }
// };

// class ImageManager : private Project
// {
// private:
//     int imageCount;
//     QVector<Image> images;

// public:
//     ImageManager() : imageCount(0) {}
//     void addImage(const Image &image)
//     {
//         images.append(image);
//         imageCount++;
//     }
//     int getImageCount() const { return imageCount; }
//     QVector<Image> getImages() const { return images; }
// };

// int main(int argc, char *argv[])
// {
//     QApplication app(argc, argv);
//     QString currentProject = nullptr;

//     QMainWindow window;
//     window.setWindowTitle("Voxel Forge");

//     QWidget *centralWidget = new QWidget;
//     QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

//     QListWidget *sidebar = new QListWidget;
//     sidebar->addItem("Home");
//     sidebar->addItem("AllProjects");
//     sidebar->addItem("Image Manager");
//     sidebar->addItem("Settings");
//     sidebar->setFixedWidth(150);
//     mainLayout->addWidget(sidebar);

//     QStackedWidget *stackedContent = new QStackedWidget;

//     QWidget *homePage = new QWidget;
//     QVBoxLayout *homeLayout = new QVBoxLayout(homePage);
//     QPushButton *button = new QPushButton("Launch COLMAP GUI");
//     homeLayout->addWidget(button);
//     homeLayout->addStretch();

//     QWidget *settingsPage = new QWidget;
//     QVBoxLayout *settingsLayout = new QVBoxLayout(settingsPage);
//     QLabel *themeLabel = new QLabel("Select Theme:");
//     QComboBox *themeCombo = new QComboBox;
//     themeCombo->addItem("Black");
//     themeCombo->addItem("White");
//     settingsLayout->addWidget(themeLabel);
//     settingsLayout->addWidget(themeCombo);
//     settingsLayout->addStretch();

//     // projects page
//     AllProjects p;
//     const QVector<QString> Allprojects = p.getAllProjects();

//     QWidget *AllprojectsPage = new QWidget;
//     QVBoxLayout *AllprojectsLayout = new QVBoxLayout(AllprojectsPage);

//     QLabel *AllprojectsLabel = new QLabel("AllProjects");
//     AllprojectsLabel->setAlignment(Qt::AlignCenter);
//     AllprojectsLayout->addWidget(AllprojectsLabel);

//     QListWidget *projectList = new QListWidget;
//     AllprojectsLayout->addWidget(projectList);
//     AllprojectsLayout->addStretch();

//     for (const QString &projectName : Allprojects)
//     {
//         projectList->addItem(projectName);
//     }
//     QPushButton *selectProjectButton = new QPushButton("Select Project");
//     AllprojectsLayout->addWidget(selectProjectButton, 0, Qt::AlignCenter);
//     AllprojectsLayout->addStretch();

//     // persistent label for selected project's info
//     QLabel *selectedProjectLabel = new QLabel;
//     selectedProjectLabel->setAlignment(Qt::AlignCenter);
//     AllprojectsLayout->addWidget(selectedProjectLabel);
//     selectedProjectLabel->hide();

//     QObject::connect(selectProjectButton, &QPushButton::clicked, [&]()
//                      {
//         QListWidgetItem *currentItem = projectList->currentItem();
//         if (currentItem)
//         {
//             currentProject = currentItem->text();
//             selectedProjectLabel->setText("Selected Project: " + currentProject);
//             selectedProjectLabel->show();
//         } });

//     // image manager page
//     QWidget *imageManagerPage = new QWidget;
//     QVBoxLayout *imageManagerLayout = new QVBoxLayout(imageManagerPage);
//     QLabel *imageManagerLabel = new QLabel("Image Manager");
//     imageManagerLayout->addWidget(imageManagerLabel);
//     imageManagerLayout->addStretch();
//     QPushButton *addImageButton = new QPushButton("Add Images");
//     imageManagerLayout->addWidget(addImageButton, 0, Qt::AlignTop);
//     imageManagerLayout->addStretch();

//     QWidget *temp = new QWidget;
//     QHBoxLayout *tempLayout = new QHBoxLayout(temp);
//     imageManagerLayout->addWidget(temp);
//     QListWidget *imagesListWidget = new QListWidget;
//     tempLayout->addWidget(imagesListWidget);
//     imageManagerLayout->addStretch();

//     QWidget *imagePreview = new QWidget;
//     imagePreview->setMinimumSize(400, 300);
//     imageManagerLayout->addWidget(imagePreview);
//     QLabel *imagePreviewLabel = new QLabel("Image Preview");
//     imagePreviewLabel->setMinimumSize(380, 280);
//     imagePreviewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//     imagePreviewLabel->setAlignment(Qt::AlignCenter);
//     QVBoxLayout *imagePreviewLayout = new QVBoxLayout(imagePreview);
//     imagePreviewLayout->addWidget(imagePreviewLabel);
//     imageManagerLayout->addStretch();

//     QObject::connect(imagesListWidget, &QListWidget::itemClicked, [&](QListWidgetItem *item)
//                      {
//                          QString imageFilePath = item->text();
//                          QPixmap pixmap(imageFilePath);
//                          if (!pixmap.isNull())
//                          {
//                              imagePreviewLabel->setPixmap(pixmap.scaled(imagePreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
//                          }
//                          else
//                          {
//                              imagePreviewLabel->setText("Failed to load image.");
//                          } });

//     QObject::connect(addImageButton, &QPushButton::clicked, [&]()
//                      {
//                          QStringList fileNames = QFileDialog::getOpenFileNames(&window, "Select Images", "", "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
//                          if (!fileNames.isEmpty())
//                          {
//                              for (const QString &imageFilePath : fileNames)
//                              {
//                                 QListWidgetItem *item = new QListWidgetItem(imageFilePath, imagesListWidget);
//                                 imagesListWidget->addItem(item);
//                              }
//                          } });

//     stackedContent->addWidget(homePage);
//     stackedContent->addWidget(AllprojectsPage);
//     stackedContent->addWidget(imageManagerPage);
//     stackedContent->addWidget(settingsPage);

//     mainLayout->addWidget(stackedContent);

//     window.setCentralWidget(centralWidget);

//     QMenuBar *menuBar = window.menuBar();
//     QMenu *fileMenu = menuBar->addMenu("File");
//     QAction *openProjectAction = fileMenu->addAction("Open Project");
//     QAction *exitAction = fileMenu->addAction("Exit");
//     QObject::connect(exitAction, &QAction::triggered, &app, &QApplication::quit);
//     QMenu *helpMenu = menuBar->addMenu("Help");
//     helpMenu->addAction("About");

//     // QObject::connect(button, &QPushButton::clicked, [&]()
//     //  { QProcess::startDetached("colmap", QStringList() << "gui"); });

//     QObject::connect(sidebar, &QListWidget::currentRowChanged, stackedContent, &QStackedWidget::setCurrentIndex);
//     sidebar->setCurrentRow(0);

//     auto setTheme = [&](int index)
//     {
//         app.setStyle(QStyleFactory::create("Fusion"));
//         if (index == 0) // black
//         {
//             QPalette darkPalette;
//             darkPalette.setColor(QPalette::Window, QColor(50, 50, 50));
//             darkPalette.setColor(QPalette::WindowText, Qt::white);
//             darkPalette.setColor(QPalette::Base, QColor(60, 60, 60));
//             darkPalette.setColor(QPalette::AlternateBase, QColor(80, 80, 80));
//             darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
//             darkPalette.setColor(QPalette::ToolTipText, Qt::white);
//             darkPalette.setColor(QPalette::Text, Qt::white);
//             darkPalette.setColor(QPalette::Button, QColor(70, 70, 70));
//             darkPalette.setColor(QPalette::ButtonText, Qt::white);
//             darkPalette.setColor(QPalette::BrightText, Qt::red);
//             darkPalette.setColor(QPalette::Highlight, QColor(60, 120, 200));
//             darkPalette.setColor(QPalette::HighlightedText, Qt::white);
//             app.setPalette(darkPalette);
//         }
//         else // white
//         {
//             app.setPalette(QApplication::style()->standardPalette());
//         }
//     };
//     QObject::connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), setTheme);
//     setTheme(0);

//     window.resize(1000, 600);
//     window.show();

//     return app.exec();
// }
