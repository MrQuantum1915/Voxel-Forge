class PhotogrammetryController : public QObject
{
    Q_OBJECT

public:
    // The pipeline now includes all OpenMVS stages
    enum PipelineStage
    {
        Idle,
        InitImageListing,
        ComputeFeatures,
        ComputeMatches,
        GeometricFilter,
        GlobalSfM,
        ExportToMVS,      // The last OpenMVG step
        Densify,          // First OpenMVS step
        ReconstructMesh,
        RefineMesh,
        TextureMesh,
        Finished,
        Error
    };

    explicit PhotogrammetryController(QObject *parent = nullptr)
        : QObject(parent), process(new QProcess(this)), currentStage(Idle)
    {
        // ... (constructor is unchanged)
        connect(process, &QProcess::readyReadStandardOutput, this, &PhotogrammetryController::handleProcessOutput);
        connect(process, &QProcess::readyReadStandardError, this, &PhotogrammetryController::handleProcessError);
        connect(process, &QProcess::errorOccurred, this, &PhotogrammetryController::handleProcessStartError);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &PhotogrammetryController::handleProcessFinished);
    }

    void startPipeline(const QString &projectPath)
    {
        // ... (this function is unchanged)
        if (currentStage != Idle)
        {
            emit logMessage("Pipeline is already running.");
            return;
        }
        this->projectPath = projectPath;
        this->imagePath = projectPath + "/images";
        this->outputPath = projectPath + "/output";
        
        QDir().mkpath(outputPath);

        emit logMessage("<b>Starting Full Reconstruction Pipeline...</b>");
        currentStage = InitImageListing;
        executeNextStage();
    }

signals:
    void logMessage(const QString &message);
    void pipelineFinished(bool success);

private slots:
    // ... (slots are unchanged)
    void handleProcessOutput() { emit logMessage(process->readAllStandardOutput()); }
    void handleProcessError() { emit logMessage("<font color='red'>" + process->readAllStandardError() + "</font>"); }
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) { /* ... unchanged ... */ }
    void handleProcessStartError(QProcess::ProcessError error) { /* ... unchanged ... */ }

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
        // --- OpenMVG Stages (mostly unchanged) ---
        case InitImageListing:
            emit logMessage("--- Stage 1: Image Listing ---");
            QDir().mkpath(matchesDir);
            program = "openMVG_main_SfMInit_ImageListing";
            arguments << "-i" << imagePath << "-o" << matchesDir << "-d" << sensorDb;
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
            arguments << "-i" << matchesDir + "/sfm_data.json" << "-m" << matchesDir + "/matches.putative.bin" << "-o" << matchesDir + "/matches.f.bin";
            break;
        case GlobalSfM:
            emit logMessage("--- Stage 5: Global Reconstruction ---");
            QDir().mkpath(outputPath + "/reconstruction/global");
            program = "openMVG_main_SfM";
            arguments << "--sfm_engine" << "GLOBAL" << "-i" << matchesDir + "/sfm_data.json" << "--match_file" << matchesDir + "/matches.f.bin" << "-o" << outputPath + "/reconstruction/global";
            break;
        case ExportToMVS:
            emit logMessage("--- Stage 6: Exporting to OpenMVS ---");
            program = "openMVG_main_openMVS_Exporter";
            arguments << "-i" << outputPath + "/reconstruction/global/sfm_data.bin" << "-o" << mvsScene;
            break;

        // --- NEW OpenMVS Stages ---
        case Densify:
            emit logMessage("--- Stage 7: Densifying Point Cloud ---");
            program = "DensifyPointCloud";
            arguments << mvsScene << "--working-folder" << outputPath;
            break;
        case ReconstructMesh:
            emit logMessage("--- Stage 8: Reconstructing Mesh ---");
            program = "ReconstructMesh";
            arguments << mvsDense << "--working-folder" << outputPath;
            break;
        case RefineMesh:
            emit logMessage("--- Stage 9: Refining Mesh ---");
            program = "RefineMesh";
            arguments << mvsMesh << "--working-folder" << outputPath;
            break;
        case TextureMesh:
            emit logMessage("--- Stage 10: Texturing Mesh ---");
            program = "TextureMesh";
            arguments << mvsRefinedMesh << "--working-folder" << outputPath;
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
