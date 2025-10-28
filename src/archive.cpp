
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
