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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Voxel Forge");

    QWidget *centralWidget = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Sidebar
    QListWidget *sidebar = new QListWidget;
    sidebar->addItem("Home");
    sidebar->addItem("Projects");
    sidebar->addItem("Settings");
    sidebar->setFixedWidth(150);
    mainLayout->addWidget(sidebar);

    // Stacked widget for content
    QStackedWidget *stackedContent = new QStackedWidget;

    // Home page
    QWidget *homePage = new QWidget;
    QVBoxLayout *homeLayout = new QVBoxLayout(homePage);
    QPushButton *button = new QPushButton("Launch COLMAP GUI");
    homeLayout->addWidget(button);
    homeLayout->addStretch();

    // Settings page
    QWidget *settingsPage = new QWidget;
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsPage);
    QLabel *themeLabel = new QLabel("Select Theme:");
    QComboBox *themeCombo = new QComboBox;
    themeCombo->addItem("Black");
    themeCombo->addItem("White");
    settingsLayout->addWidget(themeLabel);
    settingsLayout->addWidget(themeCombo);
    settingsLayout->addStretch();

    // Add pages to stacked widget
    stackedContent->addWidget(homePage);     // index 0
    stackedContent->addWidget(new QWidget);  // Projects placeholder, index 1
    stackedContent->addWidget(settingsPage); // index 2

    mainLayout->addWidget(stackedContent);

    window.setCentralWidget(centralWidget);

    // Menu bar
    QMenuBar *menuBar = window.menuBar();
    QMenu *fileMenu = menuBar->addMenu("File");
    QAction *exitAction = fileMenu->addAction("Exit");
    QObject::connect(exitAction, &QAction::triggered, &app, &QApplication::quit);
    QMenu *helpMenu = menuBar->addMenu("Help");
    helpMenu->addAction("About");

    QObject::connect(button, &QPushButton::clicked, [&]() {
        QProcess::startDetached("colmap", QStringList() << "gui");
    });

    // Sidebar navigation
    QObject::connect(sidebar, &QListWidget::currentRowChanged, stackedContent, &QStackedWidget::setCurrentIndex);
    sidebar->setCurrentRow(0);

    // Theme switching
    auto setTheme = [&](int index) {
        if (index == 0) { // Black
            app.setStyle(QStyleFactory::create("Fusion"));
        } else { // White
            app.setStyleSheet("QWidget { background: #fff; color: #222; } QPushButton { background: #eee; color: #222; } QListWidget { background: #fff; color: #222; } QComboBox { background: #fff; color: #222; } QMenuBar { background: #fff; color: #222; }");
        }
    };
    QObject::connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), setTheme);
    setTheme(0); // Default to black

    window.resize(1000, 600);
    window.show();

    return app.exec();
}