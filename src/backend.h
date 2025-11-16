// Copyright Darshan Patel [Mr.Quantum_1915]:)
// Backend classes for video extraction and photogrammetry pipeline

#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QString>
#include <atomic>

// Video frame extraction worker class
class VideoFrameExtractor : public QObject
{
    Q_OBJECT

public:
    explicit VideoFrameExtractor(QObject *parent = nullptr)
        : QObject(parent), cancelRequest(false) {}

public slots:
    void extractFrames(const QString &videoFilePath, const QString &projectFullPath);
    void cancelExtraction();

signals:
    void logMessage(const QString &message);
    void frameExtracted(const QString &fileName, const QString &fullPath);
    void extractionFinished(bool success);

private:
    std::atomic<bool> cancelRequest;
};

// Photogrammetry pipeline controller class
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
        ExportToMVS,
        Densify,
        ReconstructMesh,
        RefineMesh,
        TextureMesh,
        Finished,
        Error
    };

    explicit PhotogrammetryController(QObject *parent = nullptr)
        : QObject(parent), currentStage(Idle), cancelRequest(false) {}

public slots:
    void startPipeline(const QString &projectPath);
    void cancelPipeline();

signals:
    void logMessage(const QString &message);
    void pipelineFinished(bool success);

private:
    std::atomic<bool> cancelRequest;
    PipelineStage currentStage;
    QString projectPath, imagePath, outputPath;
};

#endif // BACKEND_H
