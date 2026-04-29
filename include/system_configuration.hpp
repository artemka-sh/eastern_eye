#pragma once

#include <cfloat>
#include <climits>


struct BoardTrackerConfig
{
    float minIouMatch_ = 0.3f;
    int maxFramesLost_ = 15;
    int countLineX_ = 0;
    int countLineY_ = 0;
    int minFramesStable_ = 5;
};

struct BoardDetectorConfig
{
    double minRelativeArea_ = 0.05;
    double maxRelativeArea_ = DBL_MAX;
    float minAspectRatio_ = 0;
    float maxAspectRatio_ = FLT_MAX;
    int minX_ = 0;
    int maxX_ = INT_MAX;
    int minY_ = 0;
    int maxY_ = INT_MAX;
    int labAThreshold_ = 130;
};

struct InspectionSystemConfig
{
    int detectInterval_ = 1;
    int lineStopThresholdSec_ = 60;
};

struct BoardAnalyzerConfig
{
    float INPUT_WIDTH = 640.0;
    float INPUT_HEIGHT = 640.0;
    float SCORE_THRESHOLD = 0.5;
    float NMS_THRESHOLD = 0.45;
    float CONFIDENCE_THRESHOLD = 0.45;

    //text
    float FONT_SCALE = 0.7;
    int THICKNESS = 1;

    float gradeA_minLightness_ = 70.0f;
    float gradeA_maxDefectRatio_ = 0.05f;
    float gradeB_minLightness_ = 60.0f;
    float gradeB_maxDefectRatio_ = 0.15f;
};

struct AppConfig
{
    InspectionSystemConfig inspectionSystemConfig;
    BoardDetectorConfig boardDetectorConfig;
    BoardTrackerConfig boardTrackerConfig;
    BoardAnalyzerConfig boardAnalyzerConfig;
};