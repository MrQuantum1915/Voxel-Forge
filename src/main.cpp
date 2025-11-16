// Copyright Darshan Patel [Mr.Quantum_1915]:)
// Main entry point for Voxel Forge application

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    MainWindow window;
    window.showMaximized();
    
    return app.exec();
}
