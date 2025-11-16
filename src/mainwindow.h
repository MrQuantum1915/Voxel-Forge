#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QString>
#include <QDir>
#include <QThread>
#include <QPointer>

// Forward declarations for Qt classes
class QListWidget;
class QStackedWidget;
class QPushButton;
class QComboBox;
class QListWidgetItem;
class QTextEdit;
class QLabel;

// Forward declarations for backend classes
class VideoFrameExtractor;
class PhotogrammetryController;

static const QString defaultProjectPath = QDir::homePath() + "/Voxel-Forge/";

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // Menu actions
    void openProjectFolder();
    void showAbout();

    // Buttons / UI
    void changePage(int index);
    void setTheme(int index);

    // Image manager
    void addImages();
    void addVideo();
    void saveSelectedImages();
    void deleteSelectedImages();
    void refreshImageList();
    void createNewFolder();
    void changeFolder(const QString &folderName);
    void onImageItemClicked(QListWidgetItem *item);
    
    // Pipeline controls
    void runSparseReconstruction();
    void runDenseReconstruction();
    void cancelPipeline();
    void cancelVideoExtraction();
    
    // Project management
    void selectProject();
    void createNewProject();
    void updateProjectDisplay();
    void loadProjectImages();
    
    // 3D Models
    void load3DModels();
    void onModelItemClicked(QListWidgetItem *item);
    void openSelectedModel();
    void goTo3DModelsPage();

private:
    void createMenuBar();
    QWidget* createHomePage();
    QWidget* createProjectManagerPage();
    QWidget* createImageManagerPage();
    QWidget* create3DModelsPage();
    QWidget* createSettingsPage();
    void setupVideoExtractorThread();
    void setupPipelineThread();

    // UI members
    QListWidget *sidebar = nullptr;
    QStackedWidget *stackedContent = nullptr;

    // Image manager widgets
    QListWidget *imageList = nullptr;
    QPushButton *addImageButton = nullptr;
    QPushButton *addVideoButton = nullptr;
    QPushButton *cancelVideoButton = nullptr;
    QPushButton *saveImagesButton = nullptr;
    QPushButton *deleteImagesButton = nullptr;
    QPushButton *refreshImagesButton = nullptr;
    QLabel *imagePreviewLabel = nullptr;
    QTextEdit *imageLogViewer = nullptr;
    
    // Project manager widgets
    QPushButton *selectProjectButton = nullptr;
    QPushButton *createProjectButton = nullptr;
    QPushButton *sparseReconButton = nullptr;
    QPushButton *denseReconButton = nullptr;
    QPushButton *view3DModelButton = nullptr;
    QPushButton *cancelPipelineButton = nullptr;
    QLabel *currentProjectLabel = nullptr;
    QTextEdit *pipelineLogViewer = nullptr;
    
    // 3D Models page widgets
    QListWidget *modelList = nullptr;
    QPushButton *openModelButton = nullptr;
    QWidget *modelViewerWidget = nullptr;
    QLabel *modelViewerLabel = nullptr;

    // theme
    QComboBox *themeCombo = nullptr;

    // state
    QString currentProjectFolder = defaultProjectPath;
    QString currentProjectName;
    QString currentImageFolder;
    QString projectFullPath;
    
    // Backend threads and workers
    QThread *videoWorkerThread = nullptr;
    VideoFrameExtractor *videoExtractor = nullptr;
    
    QThread *pipelineWorkerThread = nullptr;
    PhotogrammetryController *photoController = nullptr;
};

#endif // MAINWINDOW_H
