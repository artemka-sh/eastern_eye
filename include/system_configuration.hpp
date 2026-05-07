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
    float INPUT_SIZE = 640.0;
    float SCORE_THRESHOLD = 0.5;
    float NMS_THRESHOLD = 0.45;
    float CONFIDENCE_THRESHOLD = 0.45;

    std::array<int, 3> chromakeyMin_H_S_V = {35, 40, 40};   // H, S, V
    std::array<int, 3> chromakeyMax_H_S_V = {85, 255, 255};

    //text
    float FONT_SCALE = 0.7;
    int THICKNESS = 1;

    float gradeA_minLightness_ = 70.0f;
    float gradeA_maxDefectRatio_ = 0.05f;
    float gradeB_minLightness_ = 60.0f;
    float gradeB_maxDefectRatio_ = 0.15f;

    struct DefectClass
    {
        std::string        name;
        std::string        nameRu;
        std::array<int, 3> colorBGR;
    };
    std::vector<DefectClass> defectClasses = {
        {"live_knot",       "живой сучок",      {  0, 255,   0}}, // зелёный
        {"dead_knot",       "мёртвый сучок",    {  0,   0, 255}}, // красный
        {"knot_with_crack", "сучок с трещиной", {  0, 175, 255}}, // тёмно-жёлтый
        {"crack",           "трещина",          {100,   0, 255}}, // розовый
        {"resin",           "смола",            {255,   0, 255}}, // маджента
        {"marrow",          "сердцевина",       {255,   0,   0}}, // синий
        {"quartzity",       "кварцитность",     {100,   0, 100}}, // фиолетовый
        {"knot_missing",    "выпавший сучок",   {  0, 100, 255}}, // оранжевый
        {"blue_stain",      "синева",           {255, 255,  16}}, // циан
        {"overgrown",       "заросший",         {  0,  64,   0}}, // тёмно-зелёный
    };
};


struct AppConfig
{
    InspectionSystemConfig inspectionSystemConfig;
    BoardDetectorConfig boardDetectorConfig;
    BoardTrackerConfig boardTrackerConfig;
    BoardAnalyzerConfig boardAnalyzerConfig;
};

