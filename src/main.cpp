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

QString projectPath = QDir::homePath() + "/Voxel-Forge/";

class Projects
{
public:
    QVector<QString> getProjects()
    {
        QVector<QString> softwareStoragePath;

        QDir directory(projectPath);
        if (directory.exists())
        {
            QStringList dirs = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

            for (const QString &projectName : dirs)
            {
                softwareStoragePath.append(projectName);
            }
        }

        return softwareStoragePath;
    }
};

class Image
{
private:
    QString filePath;
    QSize size;

public:
    Image(const QString &filePath, const QSize &size) : filePath(filePath), size(size) {}
    QString getFilePath() const { return filePath; }
    QSize getSize() const { return size; }
};

class ImageManager
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
    sidebar->addItem("Projects");
    sidebar->addItem("Image Manager");
    sidebar->addItem("Settings");
    sidebar->setFixedWidth(150);
    mainLayout->addWidget(sidebar);

    QStackedWidget *stackedContent = new QStackedWidget;

    QWidget *homePage = new QWidget;
    QVBoxLayout *homeLayout = new QVBoxLayout(homePage);
    QPushButton *button = new QPushButton("Launch COLMAP GUI");
    homeLayout->addWidget(button);
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

    Projects p;
    const QVector<QString> projects = p.getProjects();

    QWidget *projectsPage = new QWidget;
    QVBoxLayout *projectsLayout = new QVBoxLayout(projectsPage);

    QLabel *projectsLabel = new QLabel("Projects");
    projectsLabel->setAlignment(Qt::AlignCenter);
    projectsLayout->addWidget(projectsLabel);

    QListWidget *projectList = new QListWidget;
    projectsLayout->addWidget(projectList);
    projectsLayout->addStretch();

    QListWidgetItem *currentItem = projectList->currentItem();
    for (const QString &projectName : projects)
    {
        projectList->addItem(projectName);
    }
    QPushButton *selectProjectButton = new QPushButton("Select Project");
    projectsLayout->addWidget(selectProjectButton, 0, Qt::AlignCenter);
    projectsLayout->addStretch();
    QObject::connect(selectProjectButton, &QPushButton::clicked, [&]()
                     {
                         if (currentItem)
                         {
                             currentProject = currentItem->text();
                             qDebug() << "Selected project:" << currentProject;
                         } });




    QWidget *imageManagerPage = new QWidget;
    QVBoxLayout *imageManagerLayout = new QVBoxLayout(imageManagerPage);
    QLabel *imageManagerLabel = new QLabel("Image");
    imageManagerLayout->addWidget(imageManagerLabel);
    imageManagerLayout->addStretch();
    QPushButton *addImageButton = new QPushButton("Add Images");
    imageManagerLayout->addWidget(addImageButton, 0, Qt::AlignCenter);
    imageManagerLayout->addStretch();

    QObject::connect(addImageButton, &QPushButton::clicked, [&]()
                     {
                         QStringList fileNames = QFileDialog::getOpenFileNames(&window, "Select Images", "", "Images (*.png *.jpg *.jpeg *.bmp)");
                         if (!fileNames.isEmpty())
                         {

                             for (const QString &filePath : fileNames)
                             {
                                 qDebug() << "Selected image:" << filePath;
                             }
                         } });

    stackedContent->addWidget(homePage);
    stackedContent->addWidget(projectsPage);
    stackedContent->addWidget(imageManagerPage);
    stackedContent->addWidget(settingsPage);

    mainLayout->addWidget(stackedContent);

    window.setCentralWidget(centralWidget);

    QMenuBar *menuBar = window.menuBar();
    QMenu *fileMenu = menuBar->addMenu("File");
    QAction *openProjectAction = fileMenu->addAction("Open Project");
    QAction *exitAction = fileMenu->addAction("Exit");
    QObject::connect(exitAction, &QAction::triggered, &app, &QApplication::quit);
    QMenu *helpMenu = menuBar->addMenu("Help");
    helpMenu->addAction("About");

    QObject::connect(button, &QPushButton::clicked, [&]()
                     { QProcess::startDetached("colmap", QStringList() << "gui"); });

    QObject::connect(sidebar, &QListWidget::currentRowChanged, stackedContent, &QStackedWidget::setCurrentIndex);
    sidebar->setCurrentRow(0);

    auto setTheme = [&](int index)
    {
        if (index == 0)
        {
            app.setStyle(QStyleFactory::create("Fusion"));
        }
        else
        {
            app.setStyleSheet("QWidget { background: #1b1b1bff; color: #ffffffff; } QPushButton { background:  #1b1b1bff; color: #ffffffff; } QListWidget { background:  #1b1b1bff; color: #ffffffff; } QComboBox { background:  #1b1b1bff; color: #ffffffff; } QMenuBar { background:  #1b1b1bff; color: #ffffffff; }");
        }
    };
    QObject::connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), setTheme);
    setTheme(0);

    window.resize(1000, 600);
    window.show();

    return app.exec();
}