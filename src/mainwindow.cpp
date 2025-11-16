#include "mainwindow.h"
#include "backend.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QProcess>
#include <QImageReader>
#include <QPixmap>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>
#include <QDir>
#include <QApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QFrame>
#include <QStyleFactory>
#include <QGraphicsDropShadowEffect>
#include <QDebug>
#include <QPainter>
#include <QLinearGradient>
#include <QBitmap>
#include <QTextEdit>
#include <QGridLayout>
#include <QDesktopServices>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Voxel Forge");

    // Set modern techno font for the entire application
    QFont appFont("Rajdhani", 10);
    appFont.setStyleHint(QFont::SansSerif);
    QApplication::setFont(appFont);

    // central widget layout: left sidebar + right stacked content
    QWidget *central = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(16);
    this->setStyleSheet(
        "QMainWindow { "
        "background-color: #030312; "
        "font-family: 'Rajdhani', 'Exo 2', 'Orbitron', 'Electrolize', 'Audiowide', sans-serif; "
        "}"
    );

    sidebar = new QListWidget;
    sidebar->setFixedWidth(200);
    sidebar->setSpacing(8);
    sidebar->setFocusPolicy(Qt::NoFocus);
    sidebar->addItem("Home");
    sidebar->addItem("Project Manager");
    sidebar->addItem("Image Manager");
    sidebar->addItem("3D Models");
    sidebar->addItem("Settings");
    sidebar->setCurrentRow(0);

    sidebar->setStyleSheet(
        "QListWidget { "
        "background: rgba(0,0,0,0.2); "
        "color: #eaeaea; "
        "font-size: 14px; "
        "font-family: 'Rajdhani', 'Exo 2', 'Orbitron', sans-serif; "
        "border-radius: 8px; "
        "border: 1px solid #8b8b8bff; "
        "outline: none; "
        "}"
        "QListWidget::item { "
        "padding: 12px 16px; "
        "margin: 4px 8px; "
        "border-radius: 6px; "
        "outline: none; "
        "border: none; "
        "}"
        "QListWidget::item:selected { "
        "background: rgba(134, 41, 255, 1); "
        "color: white; "
        "font-weight: 600; "
        "outline: none; "
        "border: none; "
        "}"
        "QListWidget::item:hover:!selected { "
        "background: rgba(194, 194, 194, 0.71); "
        "}"
        "QListWidget::item:focus { "
        "outline: none; "
        "border: none; "
        "}");
    mainLayout->addWidget(sidebar);

    // stacked content (right)
    stackedContent = new QStackedWidget;
    stackedContent->setStyleSheet(
        "QStackedWidget { "
        "border: 1px solid #8b8b8bff; "
        "border-radius: 8px; "
        "background: rgba(0,0,0,0.2); "
        "}");
    stackedContent->addWidget(createHomePage());
    stackedContent->addWidget(createProjectManagerPage());
    stackedContent->addWidget(createImageManagerPage());
    stackedContent->addWidget(create3DModelsPage());
    stackedContent->addWidget(createSettingsPage());
    mainLayout->addWidget(stackedContent, 1);

    setCentralWidget(central);

    createMenuBar();

    connect(sidebar, &QListWidget::currentRowChanged, this, &MainWindow::changePage);

    // setup backend threads
    setupVideoExtractorThread();
    setupPipelineThread();

    // default style/theme
    setTheme(0);
}

MainWindow::~MainWindow()
{
    // clean up video worker thread
    if (videoWorkerThread)
    {
        videoWorkerThread->quit();
        videoWorkerThread->wait();
    }

    // clean up pipeline worker thread
    if (pipelineWorkerThread)
    {
        pipelineWorkerThread->quit();
        pipelineWorkerThread->wait();
    }
}

void MainWindow::createMenuBar()
{
    QMenuBar *mb = menuBar();
    mb->setNativeMenuBar(false); // keep menu bar inside window on Windows/Linux

    QMenu *file = mb->addMenu("File");
    QAction *openAct = file->addAction("Open Project Folder...");
    connect(openAct, &QAction::triggered, this, &MainWindow::openProjectFolder);
    file->addSeparator();
    QAction *exitAct = file->addAction("Exit");
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);

    QMenu *help = mb->addMenu("Help");
    QAction *aboutAct = help->addAction("About");
    connect(aboutAct, &QAction::triggered, this, &MainWindow::showAbout);

    // subtle styling for menu bar
    mb->setStyleSheet(
        "QMenuBar { background: transparent; color: #eaeaea; } QMenuBar::item { spacing: 6px; padding: 4px 12px; }");
}

QWidget *MainWindow::createHomePage()
{
    QWidget *w = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout(w);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(20);

    // Logo at the top
    QWidget *imageContainer = new QWidget;
    imageContainer->setFixedSize(800, 400);

    QVBoxLayout *imageLayout = new QVBoxLayout(imageContainer);
    imageLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *imageLabel = new QLabel;
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setFixedSize(800, 400);

    QPixmap pixmap(":/icons/icons/logo.jpg");
    imageLabel->setPixmap(pixmap.scaled(380, 230, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    imageLayout->addWidget(imageLabel);

    QHBoxLayout *imageCenter = new QHBoxLayout;
    imageCenter->addStretch();
    imageCenter->addWidget(imageContainer);
    imageCenter->addStretch();
    mainLayout->addLayout(imageCenter);

    mainLayout->addSpacing(15);

    // Title section below logo
    QLabel *title = new QLabel("VOXEL FORGE");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size: 56px; "
        "color: #ffffff; "
        "font-weight: 700; "
        "font-family: 'Orbitron', 'Rajdhani', 'Exo 2', sans-serif; "
        "letter-spacing: 2px;");
    mainLayout->addWidget(title);

    QLabel *subtitle = new QLabel("Transform Images into 3D Reality");
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet(
        "font-size: 18px; "
        "color: #a8a8ff; "
        "font-weight: 400; "
        "font-family: 'Rajdhani', 'Exo 2', sans-serif; "
        "letter-spacing: 2px;");
    mainLayout->addWidget(subtitle);

    mainLayout->addSpacing(15);

    // Features - horizontal compact list
    QHBoxLayout *featuresLayout = new QHBoxLayout;
    featuresLayout->setSpacing(15);

    auto createFeatureItem = [](const QString &title)
    {
        QWidget *item = new QWidget;
        item->setStyleSheet(
            "QWidget { "
            "background: rgba(255, 255, 255, 0.03); "
            "border: 1px solid rgba(123, 97, 255, 0.2); "
            "border-radius: 10px; "
            "padding: 12px 20px; "
            "}"
            "QWidget:hover { "
            "background: rgba(255, 255, 255, 0.05); "
            "border: 1px solid rgba(123, 97, 255, 0.4); "
            "}");

        QHBoxLayout *layout = new QHBoxLayout(item);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);

        QLabel *titleLabel = new QLabel(title);
        titleLabel->setStyleSheet(
            "font-size: 13px; "
            "font-weight: 600; "
            "color: #ffffff; "
            "background: transparent; "
            "letter-spacing: 2.5px;");

        layout->addWidget(titleLabel);

        return item;
    };

    featuresLayout->addStretch();
    featuresLayout->addWidget(createFeatureItem("Point Clouds"));
    featuresLayout->addWidget(createFeatureItem("3D Models"));
    featuresLayout->addWidget(createFeatureItem("Image Manager"));
    featuresLayout->addWidget(createFeatureItem("VR Ready"));
    featuresLayout->addStretch();

    mainLayout->addLayout(featuresLayout);
    mainLayout->addStretch();

    return w;
}

QWidget *MainWindow::createProjectManagerPage()
{
    QWidget *w = new QWidget;
    QVBoxLayout *mainPageLayout = new QVBoxLayout(w);
    mainPageLayout->setContentsMargins(25, 25, 25, 25);
    mainPageLayout->setSpacing(15);

    QLabel *title = new QLabel("PROJECT MANAGER");
    title->setAlignment(Qt::AlignLeft);
    title->setStyleSheet(
        "font-size: 28px; "
        "font-weight: bold; "
        "color: #ffffff; "
        "font-family: 'Orbitron', 'Rajdhani', sans-serif; "
        "letter-spacing: 1px;");
    mainPageLayout->addWidget(title);
    mainPageLayout->addSpacing(10);

    // Project management section
    QHBoxLayout *projectControls = new QHBoxLayout();

    selectProjectButton = new QPushButton("Select Project");
    selectProjectButton->setCursor(Qt::PointingHandCursor);
    selectProjectButton->setFixedHeight(36);
    selectProjectButton->setStyleSheet(
        "QPushButton { "
        "background: rgb(103, 38, 255); "
        "color: white; "
        "border-radius: 8px; "
        "padding: 6px 20px; "
        "font-weight: 600; "
        "font-size: 13px; "
        "} "
        "QPushButton:hover { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #7f75ff, stop:1 #9f8fff); "
        "}");

    createProjectButton = new QPushButton("New Project");
    createProjectButton->setCursor(Qt::PointingHandCursor);
    createProjectButton->setFixedHeight(36);
    createProjectButton->setStyleSheet(
        "QPushButton { "
        "background: rgb(46, 204, 113); "
        "color: white; "
        "border-radius: 8px; "
        "padding: 6px 20px; "
        "font-weight: 600; "
        "font-size: 13px; "
        "} "
        "QPushButton:hover { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #27ae60, stop:1 #1e8449); "
        "}");

    projectControls->addWidget(selectProjectButton);
    projectControls->addWidget(createProjectButton);
    projectControls->addStretch();
    mainPageLayout->addLayout(projectControls);

    // Current project display
    currentProjectLabel = new QLabel("No project selected");
    currentProjectLabel->setStyleSheet(
        "QLabel { "
        "background: rgba(255, 255, 255, 0.03); "
        "border: 1px solid rgba(123, 97, 255, 0.2); "
        "border-radius: 8px; "
        "padding: 12px 16px; "
        "color: #9b7dff; "
        "font-size: 14px; "
        "font-weight: 500; "
        "}");
    mainPageLayout->addWidget(currentProjectLabel);
    mainPageLayout->addSpacing(10);

    // Create cancel pipeline button (will be added later next to Pipeline Log title)
    cancelPipelineButton = new QPushButton("Cancel Pipeline");
    cancelPipelineButton->setCursor(Qt::PointingHandCursor);
    cancelPipelineButton->setFixedHeight(36);
    cancelPipelineButton->setEnabled(false);
    cancelPipelineButton->setStyleSheet(
        "QPushButton { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #dc3545, stop:1 #c82333); "
        "color: white; "
        "border-radius: 8px; "
        "padding: 6px 20px; "
        "font-weight: 600; "
        "font-size: 13px; "
        "border: 1px solid rgba(0,0,0,0.1); "
        "} "
        "QPushButton:hover { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #e74c3c, stop:1 #d62c1a); "
        "border-color: rgba(0,0,0,0.2); "
        "} "
        "QPushButton:pressed { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #bd2130, stop:1 #a71d2a); "
        "} "
        "QPushButton:disabled { "
        "background-color: #555; "
        "color: #888; "
        "border: 1px solid #444; "
        "}");

    // Connect project buttons
    connect(selectProjectButton, &QPushButton::clicked, this, &MainWindow::selectProject);
    connect(createProjectButton, &QPushButton::clicked, this, &MainWindow::createNewProject);
    connect(cancelPipelineButton, &QPushButton::clicked, this, &MainWindow::cancelPipeline);

    // Reconstruction cards with clickable buttons
    QHBoxLayout *cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(25);

    auto makeProjectCard = [&](const QString &text, const QString &imagePath, QPushButton **buttonPtr)
    {
        QWidget *card = new QWidget();
        card->setMinimumSize(220, 250);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(15, 15, 15, 15);
        cardLayout->setSpacing(15);
        cardLayout->setAlignment(Qt::AlignCenter);

        QWidget *imageContainer = new QWidget();
        imageContainer->setFixedSize(250, 250);

        QLabel *imageLabel = new QLabel(imageContainer);
        imageLabel->setGeometry(0, 0, 250, 250);
        imageLabel->setAlignment(Qt::AlignCenter);

        QPixmap originalPixmap(imagePath);
        if (!originalPixmap.isNull())
        {
            QPixmap scaledPixmap = originalPixmap.scaled(250, 250, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPixmap roundedPixmap(scaledPixmap.size());
            roundedPixmap.fill(Qt::transparent);

            QPainter painter(&roundedPixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.setBrush(QBrush(scaledPixmap));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(roundedPixmap.rect(), 15, 15);

            imageLabel->setPixmap(roundedPixmap);
        }

        cardLayout->addWidget(imageContainer);
        cardLayout->addStretch();

        QPushButton *button = new QPushButton(text);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(
            "QPushButton {"
            "  background-color: #6c63ff; "
            "  border-radius: 8px; "
            "  padding: 10px 12px; "
            "  color: white; "
            "  font-size: 14px; "
            "  font-weight: 600; "
            "}"
            "QPushButton:hover {"
            "  background-color: #7f75ff; "
            "}"
            "QPushButton:disabled {"
            "  background-color: #555; "
            "  color: #888; "
            "}");

        cardLayout->addWidget(button);

        card->setStyleSheet(
            "QWidget { "
            "  background-color: #030317; "
            "  border: 1px solid #3c3c3c; "
            "  border-radius: 12px; "
            "}"
            "QWidget:hover { "
            "  background-color: #02021c; "
            "  border: 1px solid #7b61ff; "
            "}");

        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
        shadow->setBlurRadius(20);
        shadow->setXOffset(0);
        shadow->setYOffset(5);
        shadow->setColor(QColor(0, 0, 0, 100));
        card->setGraphicsEffect(shadow);

        if (buttonPtr)
            *buttonPtr = button;

        return card;
    };

    cardsLayout->addWidget(makeProjectCard("Generate Sparse Cloud", ":/icons/icons/sparse.jpg", &sparseReconButton));
    cardsLayout->addWidget(makeProjectCard("Generate Dense Cloud", ":/icons/icons/dense.jpg", &denseReconButton));
    cardsLayout->addWidget(makeProjectCard("View 3D Model", ":/icons/icons/model.jpg", &view3DModelButton));
    cardsLayout->addWidget(makeProjectCard("VR Connect", ":/icons/icons/vr.jpg", nullptr));

    mainPageLayout->addLayout(cardsLayout);
    mainPageLayout->addSpacing(15);

    connect(sparseReconButton, &QPushButton::clicked, this, &MainWindow::runSparseReconstruction);
    connect(denseReconButton, &QPushButton::clicked, this, &MainWindow::runDenseReconstruction);
    connect(view3DModelButton, &QPushButton::clicked, this, &MainWindow::goTo3DModelsPage);

    // Pipeline log viewer with cancel button
    QHBoxLayout *logHeaderLayout = new QHBoxLayout();
    QLabel *logLabel = new QLabel("Pipeline Log:");
    logLabel->setStyleSheet("font-size: 14px; color: #ffffff; font-weight: 600;");
    logHeaderLayout->addWidget(logLabel);
    logHeaderLayout->addStretch();
    logHeaderLayout->addWidget(cancelPipelineButton);
    mainPageLayout->addLayout(logHeaderLayout);

    pipelineLogViewer = new QTextEdit();
    pipelineLogViewer->setReadOnly(true);
    pipelineLogViewer->setFontFamily("Courier");
    pipelineLogViewer->setMaximumHeight(180);
    pipelineLogViewer->setStyleSheet(
        "QTextEdit { "
        "background: rgba(0,0,0,0.3); "
        "color: #cfcfcf; "
        "border: 2px solid rgba(153, 0, 255, 1); "
        "border-radius: 8px; "
        "padding: 8px; "
        "font-size: 20px; "
        "}");
    mainPageLayout->addWidget(pipelineLogViewer);

    return w;
}

QWidget *MainWindow::createImageManagerPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(page);
    outer->setSpacing(12);

    QHBoxLayout *header = new QHBoxLayout;
    QLabel *title = new QLabel("IMAGE MANAGER");
    title->setStyleSheet(
        "font-size: 28px; "
        "font-weight: bold; "
        "color: #ffffff; "
        "font-family: 'Orbitron', 'Rajdhani', sans-serif; "
        "letter-spacing: 1px;");
    header->addWidget(title);
    header->addStretch();

    // Add Video button
    addVideoButton = new QPushButton("+ Add Video");
    addVideoButton->setCursor(Qt::PointingHandCursor);
    addVideoButton->setFixedHeight(36);
    addVideoButton->setStyleSheet(
        "QPushButton { "
        "background: rgba(111, 0, 255, 1); "
        "color: white; "
        "border-radius: 8px; "
        "padding: 6px 16px; "
        "font-weight: 600; "
        "border: 1px solid rgba(0,0,0,0.1); "
        "} "
        "QPushButton:hover { "
        "background: rgba(124, 24, 255, 1); "
        "border-color: rgba(255, 255, 255, 0.45); "
        "box-shadow: 0 0 12px rgba(140,130,255,0.6); "
        "} "
        "QPushButton:disabled { "
        "background-color: #888; "
        "color: #ccc; "
        "}");

    // Cancel Video button
    cancelVideoButton = new QPushButton("Cancel Extraction");
    cancelVideoButton->setCursor(Qt::PointingHandCursor);
    cancelVideoButton->setFixedHeight(36);
    cancelVideoButton->setEnabled(false);
    cancelVideoButton->setStyleSheet(
        "QPushButton { "
        "background-color: #df1a13ff; "
        "color: white; "
        "border-radius: 8px; "
        "padding: 6px 16px; "
        "font-weight: 600; "
        "} "
        "QPushButton:disabled { "
        "background-color: #888; "
        "color: #ccc; "
        "}");

    addImageButton = new QPushButton("+ Add Images");
    addImageButton->setCursor(Qt::PointingHandCursor);
    addImageButton->setFixedHeight(36);
    addImageButton->setStyleSheet(
        "QPushButton { "
        "background: rgba(111, 0, 255, 1); "
        "color: white; "
        "border-radius: 8px; "
        "padding: 6px 16px; "
        "font-weight: 600; "
        "border: 1px solid rgba(0,0,0,0.1); "
        "} "
        "QPushButton:hover { "
        "background: rgba(124, 24, 255, 1); "
        "border-color: rgba(255, 255, 255, 0.45); "
        "box-shadow: 0 0 12px rgba(140,130,255,0.6); "
        "} "
        "QPushButton:pressed { "
        "background: rgba(90, 80, 224, 1); "
        "} ");

    saveImagesButton = new QPushButton("Save Selected...");
    saveImagesButton->setCursor(Qt::PointingHandCursor);
    saveImagesButton->setFixedHeight(36);
    saveImagesButton->setStyleSheet(
        "QPushButton { "
        "background-color: #2ecc71; "
        "color: white; "
        "border-radius: 8px; "
        "padding: 6px 16px; "
        "border: 1px solid #27ae60; "
        "font-weight: 500; "
        "}"
        "QPushButton:hover { "
        "background-color: #27ae60; "
        "border-color: #1e8449; "
        "}");

    deleteImagesButton = new QPushButton("Delete Selected");
    deleteImagesButton->setCursor(Qt::PointingHandCursor);
    deleteImagesButton->setFixedHeight(36);
    deleteImagesButton->setStyleSheet(
        "QPushButton { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #dc3545, stop:1 #c82333); "
        "color: white; "
        "border-radius: 8px; "
        "padding: 6px 16px; "
        "font-weight: 600; "
        "border: 1px solid rgba(0,0,0,0.1); "
        "transition: all 0.2s ease-in-out; "
        "} "
        "QPushButton:hover { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #e74c3c, stop:1 #d62c1a); "
        "border-color: rgba(0,0,0,0.2); "
        "box-shadow: 0 0 12px rgba(220,53,69,0.5); "
        "} "
        "QPushButton:pressed { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #bd2130, stop:1 #a71d2a); "
        "} ");

    refreshImagesButton = new QPushButton("Refresh");
    refreshImagesButton->setCursor(Qt::PointingHandCursor);
    refreshImagesButton->setFixedHeight(36);
    refreshImagesButton->setStyleSheet(
        "QPushButton { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #3498db, stop:1 #2980b9); "
        "color: white; "
        "border-radius: 8px; "
        "padding: 6px 16px; "
        "font-weight: 600; "
        "border: 1px solid rgba(0,0,0,0.1); "
        "} "
        "QPushButton:hover { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #5dade2, stop:1 #3498db); "
        "border-color: rgba(255, 255, 255, 0.3); "
        "} "
        "QPushButton:pressed { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #2980b9, stop:1 #21618c); "
        "}");

    // refreshbutton - left
    header->addWidget(refreshImagesButton);

    // spacer - push other buttons to right
    header->addStretch();

    // Add other buttons on the right
    header->addWidget(addImageButton);
    header->addWidget(addVideoButton);
    header->addWidget(cancelVideoButton);
    header->addWidget(deleteImagesButton);
    header->addWidget(saveImagesButton);
    outer->addLayout(header);

    // middle section with image list and preview
    QHBoxLayout *middleLayout = new QHBoxLayout();

    // image list as icon grid
    imageList = new QListWidget;
    imageList->setViewMode(QListView::IconMode);
    imageList->setIconSize(QSize(160, 120));
    imageList->setResizeMode(QListWidget::Adjust);
    imageList->setMovement(QListView::Static);
    imageList->setSpacing(12);
    imageList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    imageList->setStyleSheet(
        "QListWidget { background: rgba(0,0,0,0.15); border: 2px solid rgba(103, 38, 255, 1); "
        "border-radius: 8px; padding: 12px; }"
        "QListWidget::item { color: #eaeaea; font-size: 12px; background: rgba(255,255,255,0.03); "
        "border-radius: 6px; padding: 8px; }"
        "QListWidget::item:selected { background: rgba(123,97,255,0.2); border: 2px solid #7b61ff; }"
        "QListWidget::item:hover { background: rgba(255,255,255,0.08); }");
    middleLayout->addWidget(imageList, 2);

    // Image preview
    imagePreviewLabel = new QLabel("Image Preview");
    imagePreviewLabel->setMinimumSize(400, 300);
    imagePreviewLabel->setAlignment(Qt::AlignCenter);
    imagePreviewLabel->setStyleSheet(
        "border: 2px solid rgba(103, 38, 255, 1); "
        "border-radius: 8px; "
        "background: rgba(0,0,0,0.2); "
        "color: #ffffffff;");
    middleLayout->addWidget(imagePreviewLabel, 1);

    outer->addLayout(middleLayout, 2);

    // Image/Video Log viewer
    QLabel *imageLogLabel = new QLabel("Image/Video Log:");
    imageLogLabel->setStyleSheet("font-size: 14px; color: #ffffff; font-weight: 600;");
    outer->addWidget(imageLogLabel);

    imageLogViewer = new QTextEdit();
    imageLogViewer->setReadOnly(true);
    imageLogViewer->setFontFamily("Courier");
    imageLogViewer->setMaximumHeight(150);
    imageLogViewer->setStyleSheet(
        "QTextEdit { "
        "background: rgba(0,0,0,0.3); "
        "color: #cfcfcf; "
        "border: 2px solid rgba(103, 38, 255, 1); "
        "border-radius: 8px; "
        "padding: 8px; "
        "font-size: 20px; "
        "}");
    outer->addWidget(imageLogViewer, 1);

    // connections
    connect(addImageButton, &QPushButton::clicked, this, &MainWindow::addImages);
    connect(addVideoButton, &QPushButton::clicked, this, &MainWindow::addVideo);
    connect(cancelVideoButton, &QPushButton::clicked, this, &MainWindow::cancelVideoExtraction);
    connect(refreshImagesButton, &QPushButton::clicked, this, &MainWindow::refreshImageList);
    connect(saveImagesButton, &QPushButton::clicked, this, &MainWindow::saveSelectedImages);
    connect(deleteImagesButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedImages);
    connect(imageList, &QListWidget::itemClicked, this, &MainWindow::onImageItemClicked);

    return page;
}

QWidget *MainWindow::create3DModelsPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(page);
    outer->setSpacing(12);
    outer->setContentsMargins(25, 25, 25, 25);

    // Header with title and Open Model button
    QHBoxLayout *header = new QHBoxLayout;
    QLabel *title = new QLabel("3D MODELS");
    title->setStyleSheet(
        "font-size: 28px; "
        "font-weight: bold; "
        "color: #ffffff; "
        "font-family: 'Orbitron', 'Rajdhani', sans-serif; "
        "letter-spacing: 1px;");
    header->addWidget(title);
    header->addStretch();

    openModelButton = new QPushButton("Open Model");
    openModelButton->setCursor(Qt::PointingHandCursor);
    openModelButton->setFixedHeight(40);
    openModelButton->setEnabled(false);
    openModelButton->setStyleSheet(
        "QPushButton { "
        "background: rgba(132, 0, 255, 1); "
        "color: white; "
        "border-radius: 8px; "
        "padding: 8px 24px; "
        "font-weight: 600; "
        "font-size: 14px; "
        "border: 1px solid rgba(0,0,0,0.1); "
        "} "
        "QPushButton:hover { "
        "background: rgba(132, 0, 255, 1); "
        "} "
        "QPushButton:disabled { "
        "background-color: #555; "
        "color: #888; "
        "border: 1px solid #444; "
        "}");
    
    header->addWidget(openModelButton);
    outer->addLayout(header);

    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(15);

    // Left side - Model list
    QVBoxLayout *listLayout = new QVBoxLayout();
    
    QLabel *listLabel = new QLabel("Available Models:");
    listLabel->setStyleSheet("font-size: 14px; color: #ffffff; font-weight: 600;");
    listLayout->addWidget(listLabel);

    modelList = new QListWidget;
    modelList->setSelectionMode(QAbstractItemView::SingleSelection);
    modelList->setMaximumWidth(350);
    modelList->setStyleSheet(
        "QListWidget { "
        "background: rgba(0,0,0,0.3); "
        "border: 2px solid rgba(103, 38, 255, 1); "
        "border-radius: 8px; "
        "padding: 8px; "
        "color: #eaeaea; "
        "font-size: 13px; "
        "} "
        "QListWidget::item { "
        "padding: 10px; "
        "margin: 4px 0px; "
        "border-radius: 6px; "
        "background: rgba(255,255,255,0.03); "
        "} "
        "QListWidget::item:selected { "
        "background: rgba(123,97,255,0.3); "
        "border: 2px solid #7b61ff; "
        "color: #ffffff; "
        "} "
        "QListWidget::item:hover { "
        "background: rgba(255,255,255,0.08); "
        "}");
    
    listLayout->addWidget(modelList);
    contentLayout->addLayout(listLayout, 1);

    // Right side - 3D Model Viewer
    QVBoxLayout *viewerLayout = new QVBoxLayout();
    
    QLabel *viewerLabel = new QLabel("3D Model Viewer:");
    viewerLabel->setStyleSheet("font-size: 14px; color: #ffffff; font-weight: 600;");
    viewerLayout->addWidget(viewerLabel);

    modelViewerWidget = new QWidget();
    modelViewerWidget->setMinimumSize(600, 800);
    modelViewerWidget->setStyleSheet(
        "QWidget { "
        "background: rgba(0,0,0,0.4); "
        "border: 2px solid rgba(103, 38, 255, 1); "
        "border-radius: 8px; "
        "}");
    
    // For now, add a label that will show model info
    QVBoxLayout *viewerContentLayout = new QVBoxLayout(modelViewerWidget);
    modelViewerLabel = new QLabel("No model loaded\n\nSelect a model from the list and click 'Open Model'");
    modelViewerLabel->setAlignment(Qt::AlignCenter);
    modelViewerLabel->setStyleSheet(
        "QLabel { "
        "color: #9b7dff; "
        "font-size: 16px; "
        "background: transparent; "
        "border: none; "
        "}");
    viewerContentLayout->addWidget(modelViewerLabel);
    
    viewerLayout->addWidget(modelViewerWidget);
    contentLayout->addLayout(viewerLayout, 3);

    outer->addLayout(contentLayout);

    // Connect signals
    connect(modelList, &QListWidget::itemClicked, this, &MainWindow::onModelItemClicked);
    connect(openModelButton, &QPushButton::clicked, this, &MainWindow::openSelectedModel);

    return page;
}

QWidget *MainWindow::createSettingsPage()
{
    QWidget *w = new QWidget;
    QVBoxLayout *v = new QVBoxLayout(w);

    QLabel *title = new QLabel("SETTINGS");
    title->setStyleSheet(
        "font-size: 28px; "
        "font-weight: bold; "
        "color: #ffffff; "
        "font-family: 'Orbitron', 'Rajdhani', sans-serif; "
        "letter-spacing: 1px;");
    v->addWidget(title);

    QHBoxLayout *themeRow = new QHBoxLayout;
    QLabel *themeLabel = new QLabel("Theme:");
    themeLabel->setStyleSheet("color: #cfcfcf;");
    themeCombo = new QComboBox;
    themeCombo->addItem("Dark (Default)");
    themeCombo->addItem("Fusion (Light)");
    themeCombo->setFixedWidth(180);
    themeCombo->setStyleSheet("QComboBox { padding: 6px; }");

    themeRow->addWidget(themeLabel);
    themeRow->addWidget(themeCombo);
    themeRow->addStretch();
    v->addLayout(themeRow);
    v->addStretch();

    connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::setTheme);

    return w;
}

// Slots

void MainWindow::openProjectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Project Folder", currentProjectFolder);
    if (!dir.isEmpty())
    {
        currentProjectFolder = dir;
        QMessageBox::information(this, "Project Folder", QString("Project folder set to:\n%1").arg(currentProjectFolder));
    }
}

void MainWindow::showAbout()
{
    QString aboutText =
        "<h3>Voxel Forge</h3>"
        "<p><b>Version:</b> 1.0.0</p>"
        "<p>A 3D Model Reconstruction and Interaction System built with Qt.</p>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>Import and manage project images</li>"
        "<li>Reconstruct sparse and dense point clouds</li>"
        "<li>View and interact with 3D models</li>"
        "<li>Connect with VR systems</li>"
        "</ul>"
        "<p><b>Developed by:</b> Darshan, Tanishq, Sanskar, Prayag and Maharishi</p>"
        "<p><b>Built with:</b> Qt, OpenMVG, OpenMVS, Unreal and other open-source libraries.</p>"
        "<p>&copy; 2025 All Rights Reserved.</p>";

    QMessageBox::about(this, "About Voxel Forge", aboutText);
}

void MainWindow::createNewFolder()
{
    if (currentProjectFolder.isEmpty())
    {
        QMessageBox::warning(this, "No Project", "Please open a project folder first.");
        return;
    }

    QString folderName = QInputDialog::getText(this, "New Folder", "Enter folder name:");
    if (folderName.isEmpty())
        return;

    QDir dir(currentProjectFolder);
    if (!dir.exists())
    {
        QMessageBox::warning(this, "Error", "Project folder path is invalid.");
        return;
    }

    QString newFolderPath = dir.filePath(folderName);
    if (QDir(newFolderPath).exists())
    {
        QMessageBox::warning(this, "Error", "Folder already exists.");
        return;
    }

    if (!dir.mkdir(folderName))
    {
        QMessageBox::warning(this, "Error", "Failed to create folder.");
    }
    else
    {
        QMessageBox::information(this, "Folder Created", "Folder created successfully!");

        // refresh image manager
        changeFolder(currentProjectFolder);
    }
}

void MainWindow::changePage(int index)
{
    stackedContent->setCurrentIndex(index);
}

void MainWindow::setTheme(int index)
{
    if (index == 0)
    {
        // Dark theme
        qApp->setStyleSheet(
            "QMainWindow { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #141418, stop:1 #1e1e24); }"
            "QLabel { color: #eaeaea; }"
            "QMenuBar { color: #eaeaea; }");
        qApp->setStyle(QStyleFactory::create("Fusion"));
    }
    else
    {
        qApp->setStyleSheet("");
        qApp->setStyle(QStyleFactory::create("Fusion"));
    }
}

// Image manager: add images (open explorer and display thumbnails)
void MainWindow::addImages()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Select Images", QString(),
        "Images (*.png *.jpg *.jpeg *.bmp *.tiff)");

    if (files.isEmpty())
        return;

    QString savePath = currentProjectFolder;

    QDir dir(savePath);
    if (!dir.exists())
        dir.mkpath(".");

    for (const QString &f : files)
    {
        QFileInfo fi(f);
        QPixmap pm(f);
        if (pm.isNull())
            continue;

        QString dest = dir.filePath(fi.fileName());
        QFile::copy(f, dest);

        QPixmap scaled = pm.scaled(320, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QListWidgetItem *item = new QListWidgetItem;
        item->setIcon(QIcon(scaled));
        item->setText(fi.fileName());
        item->setToolTip(dest);            // show saved path on hover
        item->setData(Qt::UserRole, dest); // store full path
        imageList->addItem(item);          // <-- missing line before
    }
}

void MainWindow::saveSelectedImages()
{
    QList<QListWidgetItem *> selected = imageList->selectedItems();
    if (selected.isEmpty())
    {
        QMessageBox::information(this, "No selection", "Please select one or more images from the list to save.");
        return;
    }

    // default destination suggestion: currentProjectFolder (create if missing)
    QDir dir(currentProjectFolder);
    if (!dir.exists())
    {
        dir.mkpath(".");
    }

    QString dest = QFileDialog::getExistingDirectory(this, "Select Destination Folder", currentProjectFolder);
    if (dest.isEmpty())
        return;

    int copied = 0;
    QStringList failed;
    for (QListWidgetItem *it : selected)
    {
        QString src = it->data(Qt::UserRole).toString();
        if (src.isEmpty())
            continue;
        QFileInfo fi(src);
        QString dstPath = QDir(dest).filePath(fi.fileName());

        // if exists, create unique name
        int idx = 1;
        QString base = fi.completeBaseName();
        QString ext = fi.suffix();
        while (QFile::exists(dstPath))
        {
            QString newName = QString("%1_%2.%3").arg(base).arg(idx).arg(ext);
            dstPath = QDir(dest).filePath(newName);
            idx++;
        }

        if (QFile::copy(src, dstPath))
        {
            copied++;
        }
        else
        {
            failed << src;
        }
    }

    QString msg = QString("Copied %1 file(s) to:\n%2").arg(copied).arg(dest);
    if (!failed.isEmpty())
    {
        msg += QString("\n\nFailed to copy %1 file(s).").arg(failed.count());
    }
    QMessageBox::information(this, "Save Complete", msg);
}
void MainWindow::changeFolder(const QString &path)
{
    // Update the current image folder
    currentImageFolder = path;

    // Clear existing list
    imageList->clear();

    // Populate images from the new folder
    QDir dir(path);
    QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.tiff"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo &fi : files)
    {
        QPixmap pm(fi.absoluteFilePath());
        if (pm.isNull())
            continue;

        QPixmap scaled = pm.scaled(imageList->iconSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QListWidgetItem *item = new QListWidgetItem;
        item->setIcon(QIcon(scaled));
        item->setText(fi.fileName());
        item->setToolTip(fi.absoluteFilePath());
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        imageList->addItem(item);
    }
}

void MainWindow::deleteSelectedImages()
{
    QList<QListWidgetItem *> selected = imageList->selectedItems();
    if (selected.isEmpty())
    {
        QMessageBox::information(this, "No selection", "Please select one or more images to delete.");
        return;
    }

    if (QMessageBox::question(this, "Confirm Delete",
                              QString("Are you sure you want to remove %1 selected image(s) from the list?")
                                  .arg(selected.count())) != QMessageBox::Yes)
    {
        return;
    }

    for (QListWidgetItem *it : selected)
    {
        QString path = it->data(Qt::UserRole).toString();
        if (!path.isEmpty() && QFile::exists(path))
            QFile::remove(path);

        delete imageList->takeItem(imageList->row(it));
    }
}

void MainWindow::refreshImageList()
{
    if (currentProjectFolder.isEmpty())
    {
        QMessageBox::warning(this, "No Project", "Please select or create a project first.");
        return;
    }

    // Clear current image list
    imageList->clear();

    // Clear preview
    if (imagePreviewLabel)
    {
        imagePreviewLabel->clear();
        imagePreviewLabel->setText("Image Preview\n\nClick an image to preview");
        imagePreviewLabel->setAlignment(Qt::AlignCenter);
    }

    // Reload images from project folder
    loadProjectImages();

    // Show confirmation message
    int count = imageList->count();
    imageLogViewer->append(QString("<span style='color:#2ecc71;'>✓ Refreshed image list - Found %1 image(s)</span>").arg(count));
}

void MainWindow::onImageItemClicked(QListWidgetItem *item)
{
    if (!item || !imagePreviewLabel)
        return;

    QString imagePath = item->data(Qt::UserRole).toString();
    if (imagePath.isEmpty())
        return;

    QPixmap pixmap(imagePath);
    if (pixmap.isNull())
        return;

    // Scale to fit preview label
    QPixmap scaled = pixmap.scaled(imagePreviewLabel->size(),
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
    imagePreviewLabel->setPixmap(scaled);
}

void MainWindow::addVideo()
{
    if (currentProjectFolder.isEmpty())
    {
        QMessageBox::warning(this, "No Project", "Please open a project folder first.");
        return;
    }

    QString videoFile = QFileDialog::getOpenFileName(
        this, "Select Video", QString(),
        "Videos (*.mp4 *.avi *.mov *.mkv *.wmv)");

    if (videoFile.isEmpty())
        return;

    projectFullPath = currentProjectFolder;

    // Disable add video button and enable cancel button
    addVideoButton->setEnabled(false);
    cancelVideoButton->setEnabled(true);

    QString videoDir = projectFullPath + "/videos";
    QDir().mkpath(videoDir);
    QString destVideoPath = videoDir + "/" + QFileInfo(videoFile).fileName();
    if (!QFile::copy(videoFile, destVideoPath))
    {
        QMessageBox::warning(this, "Copy Failed", "<font color='red'>Failed to copy video to project folder.</font>");
        return;
    }

    // Start extraction on worker thread
    QMetaObject::invokeMethod(videoExtractor, "extractFrames",
                              Qt::QueuedConnection,
                              Q_ARG(QString, destVideoPath),
                              Q_ARG(QString, projectFullPath));
}

void MainWindow::cancelVideoExtraction()
{
    if (videoExtractor)
    {
        QMetaObject::invokeMethod(videoExtractor, "cancelExtraction", Qt::QueuedConnection);
    }
}

void MainWindow::selectProject()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Project Folder", defaultProjectPath);
    if (!dir.isEmpty() && dir.startsWith(defaultProjectPath))
    {
        currentProjectFolder = dir;
        currentProjectName = QFileInfo(dir).fileName();
        projectFullPath = dir;
        updateProjectDisplay();
    }
    else if (!dir.isEmpty())
    {
        QMessageBox::warning(this, "Invalid Project",
                             "Please select a project folder from:\n" + defaultProjectPath);
    }
}

void MainWindow::createNewProject()
{
    QString projectName = QInputDialog::getText(this, "New Project", "Enter project name:");
    if (projectName.isEmpty())
        return;

    QDir baseDir(defaultProjectPath);
    if (!baseDir.exists())
        baseDir.mkpath(".");

    QString newProjectPath = defaultProjectPath + projectName;
    if (QDir(newProjectPath).exists())
    {
        QMessageBox::warning(this, "Project Exists", "A project with this name already exists!");
        return;
    }

    QDir().mkpath(newProjectPath);
    QDir().mkpath(newProjectPath + "/images");
    QDir().mkpath(newProjectPath + "/output");

    currentProjectFolder = newProjectPath;
    currentProjectName = projectName;
    projectFullPath = newProjectPath;
    updateProjectDisplay();

    QMessageBox::information(this, "Project Created",
                             "Project '" + projectName + "' created successfully!");
}

void MainWindow::updateProjectDisplay()
{
    if (currentProjectName.isEmpty())
    {
        currentProjectLabel->setText("No project selected");
        currentProjectLabel->setStyleSheet(
            "QLabel { "
            "background: rgba(255, 255, 255, 0.03); "
            "border: 1px solid rgba(123, 97, 255, 0.2); "
            "border-radius: 8px; "
            "padding: 12px 16px; "
            "color: #888; "
            "font-size: 14px; "
            "font-weight: 500; "
            "}");

        // Clear image list when no project
        imageList->clear();
    }
    else
    {
        currentProjectLabel->setText("Current Project: " + currentProjectName);
        currentProjectLabel->setStyleSheet(
            "QLabel { "
            "background: rgba(123, 97, 255, 0.1); "
            "border: 1px solid rgba(123, 97, 255, 0.4); "
            "border-radius: 8px; "
            "padding: 12px 16px; "
            "color: #9b7dff; "
            "font-size: 14px; "
            "font-weight: 600; "
            "}");

        // Load images from project folder
        loadProjectImages();
    }
}

void MainWindow::loadProjectImages()
{
    if (currentProjectFolder.isEmpty())
        return;

    // Clear existing images
    imageList->clear();
    imageLogViewer->clear();

    QString imagePath = projectFullPath + "/images";
    QDir imageDir(imagePath);

    if (!imageDir.exists())
    {
        imageLogViewer->append("No images folder found in project.");
        return;
    }

    // Supported image formats
    QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.tiff", "*.tif"};
    QFileInfoList imageFiles = imageDir.entryInfoList(filters, QDir::Files, QDir::Name);

    if (imageFiles.isEmpty())
    {
        imageLogViewer->append("No images found in project folder.");
        return;
    }

    imageLogViewer->append(QString("Loading %1 images from project...").arg(imageFiles.count()));

    int loadedCount = 0;
    for (const QFileInfo &fileInfo : imageFiles)
    {
        QString filePath = fileInfo.absoluteFilePath();
        QPixmap pm(filePath);

        if (pm.isNull())
            continue;

        // Create thumbnail
        QPixmap scaled = pm.scaled(320, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // Add to image list
        QListWidgetItem *item = new QListWidgetItem;
        item->setIcon(QIcon(scaled));
        item->setText(fileInfo.fileName());
        item->setToolTip(filePath);
        item->setData(Qt::UserRole, filePath);
        imageList->addItem(item);

        loadedCount++;
    }

    imageLogViewer->append(QString("Successfully loaded %1 images.").arg(loadedCount));
}

void MainWindow::runSparseReconstruction()
{
    if (currentProjectFolder.isEmpty() || currentProjectName.isEmpty())
    {
        QMessageBox::warning(this, "No Project", "Please select or create a project first.");
        return;
    }

    // Check if images exist
    QString imagePath = projectFullPath + "/images";
    QDir imageDir(imagePath);
    if (!imageDir.exists() || imageDir.entryList(QDir::Files).isEmpty())
    {
        QMessageBox::warning(this, "No Images",
                             "No images found in project. Please add images first.");
        return;
    }

    pipelineLogViewer->clear();
    pipelineLogViewer->append("=== Starting Sparse Reconstruction Pipeline ===");

    sparseReconButton->setEnabled(false);
    denseReconButton->setEnabled(false);
    cancelPipelineButton->setEnabled(true);

    // Start pipeline on worker thread
    QMetaObject::invokeMethod(photoController, "startPipeline",
                              Qt::QueuedConnection,
                              Q_ARG(QString, projectFullPath));
}

void MainWindow::runDenseReconstruction()
{
    if (currentProjectFolder.isEmpty() || currentProjectName.isEmpty())
    {
        QMessageBox::warning(this, "No Project", "Please select or create a project first.");
        return;
    }

    // Check if sparse reconstruction exists
    QString reconPath = projectFullPath + "/output/reconstruction";
    QDir reconDir(reconPath);
    if (!reconDir.exists())
    {
        QMessageBox::warning(this, "No Sparse Data",
                             "Please run Sparse Reconstruction first!");
        return;
    }

    pipelineLogViewer->clear();
    pipelineLogViewer->append("=== Starting Dense Reconstruction Pipeline ===");
    pipelineLogViewer->append("Dense reconstruction will be implemented...");

    // TODO: Implement dense reconstruction
    QMessageBox::information(this, "Coming Soon",
                             "Dense reconstruction pipeline will be implemented soon!");
}

void MainWindow::cancelPipeline()
{
    if (photoController)
    {
        QMetaObject::invokeMethod(photoController, "cancelPipeline", Qt::QueuedConnection);
    }
}

void MainWindow::setupVideoExtractorThread()
{
    videoWorkerThread = new QThread(this);
    videoExtractor = new VideoFrameExtractor;
    videoExtractor->moveToThread(videoWorkerThread);

    // Use QPointer for safe cross-thread widget access
    QPointer<QTextEdit> safeImageLogViewer(imageLogViewer);
    QPointer<QListWidget> safeImageList(imageList);
    QPointer<QPushButton> safeAddVideoButton(addVideoButton);
    QPointer<QPushButton> safeCancelVideoButton(cancelVideoButton);

    // Connect signals with Qt::QueuedConnection for thread safety
    connect(videoExtractor, &VideoFrameExtractor::logMessage, this, [safeImageLogViewer](const QString &msg)
            {
                if (safeImageLogViewer) safeImageLogViewer->append(msg); }, Qt::QueuedConnection);

    connect(videoExtractor, &VideoFrameExtractor::frameExtracted, this, [safeImageList](const QString &fileName, const QString &fullPath)
            {
                if (!safeImageList) return;
                QPixmap pm(fullPath);
                if (pm.isNull()) return;
                QPixmap scaled = pm.scaled(320, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                QListWidgetItem *item = new QListWidgetItem;
                item->setIcon(QIcon(scaled));
                item->setText(fileName);
                item->setToolTip(fullPath);
                item->setData(Qt::UserRole, fullPath);
                safeImageList->addItem(item); }, Qt::QueuedConnection);

    connect(videoExtractor, &VideoFrameExtractor::extractionFinished, this, [safeAddVideoButton, safeCancelVideoButton, safeImageLogViewer](bool success)
            {
                if (safeAddVideoButton) safeAddVideoButton->setEnabled(true);
                if (safeCancelVideoButton) safeCancelVideoButton->setEnabled(false);
                if (safeImageLogViewer) {
                    safeImageLogViewer->append(success ? 
                        "Video extraction completed!" : 
                        "Video extraction cancelled or failed.");
                } }, Qt::QueuedConnection);

    videoWorkerThread->start();
}

void MainWindow::setupPipelineThread()
{
    pipelineWorkerThread = new QThread(this);
    photoController = new PhotogrammetryController;
    photoController->moveToThread(pipelineWorkerThread);

    // Connect signals
    connect(photoController, &PhotogrammetryController::logMessage,
            pipelineLogViewer, &QTextEdit::append, Qt::QueuedConnection);

    connect(photoController, &PhotogrammetryController::pipelineFinished, this, [this](bool success)
            {
                sparseReconButton->setEnabled(true);
                denseReconButton->setEnabled(true);
                cancelPipelineButton->setEnabled(false);
                pipelineLogViewer->append(success ? 
                    "\n=== Pipeline completed successfully! ===" : 
                    "\n=== Pipeline failed or was cancelled ==="); }, Qt::QueuedConnection);

    pipelineWorkerThread->start();
}

void MainWindow::goTo3DModelsPage()
{
    sidebar->setCurrentRow(3);
    stackedContent->setCurrentIndex(3);
    load3DModels();
}

void MainWindow::load3DModels()
{
    if (!modelList)
        return;
        
    modelList->clear();
    
    if (currentProjectFolder.isEmpty() || currentProjectName.isEmpty())
    {
        QListWidgetItem *item = new QListWidgetItem("No project selected");
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        modelList->addItem(item);
        return;
    }
    
    QString modelsPath = projectFullPath + "/final_3d_models";
    QDir modelsDir(modelsPath);
    
    if (!modelsDir.exists())
    {
        QListWidgetItem *item = new QListWidgetItem("No 'final_3d_models' folder found in project");
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        modelList->addItem(item);
        return;
    }
    
    // Supported 3D model formats
    QStringList filters = {"*.obj", "*.ply", "*.stl", "*.fbx", "*.dae", "*.3ds", "*.gltf", "*.glb"};
    QFileInfoList modelFiles = modelsDir.entryInfoList(filters, QDir::Files, QDir::Name);
    
    if (modelFiles.isEmpty())
    {
        QListWidgetItem *item = new QListWidgetItem("No 3D models found in 'final_3d_models' folder");
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        modelList->addItem(item);
        return;
    }
    
    for (const QFileInfo &fileInfo : modelFiles)
    {
        QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
        item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
        item->setToolTip(fileInfo.absoluteFilePath());
        modelList->addItem(item);
    }
}

void MainWindow::onModelItemClicked(QListWidgetItem *item)
{
    if (!item || !openModelButton)
        return;
    
    QString filePath = item->data(Qt::UserRole).toString();
    if (!filePath.isEmpty())
    {
        openModelButton->setEnabled(true);
    }
    else
    {
        openModelButton->setEnabled(false);
    }
}

void MainWindow::openSelectedModel()
{
    if (!modelList || !modelViewerLabel)
        return;
    
    QListWidgetItem *item = modelList->currentItem();
    if (!item)
        return;
    
    QString modelPath = item->data(Qt::UserRole).toString();
    if (modelPath.isEmpty())
        return;
    
    QFileInfo fileInfo(modelPath);
    if (!fileInfo.exists())
    {
        QMessageBox::warning(this, "File Not Found", "The selected model file does not exist.");
        return;
    }
    
    // For now, show model info in the label
    // In a full implementation, you would use Qt3D or another 3D rendering library
    QString modelInfo = QString(
        "Model Loaded:\n\n"
        "Name: %1\n"
        "Path: %2\n"
        "Size: %3 KB\n\n"
        "Note: Full 3D rendering will be implemented using Qt3D.\n"
        "For now, you can open this model in external viewers."
    ).arg(fileInfo.fileName())
     .arg(modelPath)
     .arg(fileInfo.size() / 1024);
    
    modelViewerLabel->setText(modelInfo);
    modelViewerLabel->setStyleSheet(
        "QLabel { "
        "color: #ffffff; "
        "font-size: 13px; "
        "background: transparent; "
        "border: none; "
        "}");
    
    // Optional: Launch external viewer
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Open in External Viewer?",
        "Would you like to open this model in an external 3D viewer?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes)
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(modelPath));
    }
}
