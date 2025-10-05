#pragma once

#include "polygon.h"
#include <string>

#define NULL_SAVE_PATH "NULL"

#ifdef _WIN32
#define FILE_SEPARATOR '\\'
#else
#define FILE_SEPARATOR '/'
#endif

// Structure to hold image state information
struct ImageState {
    bool hasImage = false;
    std::string imagePath = "";
    float positionX = 0.0f;
    float positionY = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float opacity = 0.7f;
    bool enabled = true;
};

std::string getExecutablePath();

bool saveToFile(std::vector<Polygon> polygons, const ImageState& imageState, std::string fileLocation);

std::pair<std::vector<Polygon>, ImageState> openFile(std::string fileLocation);

void quickSave(std::vector<Polygon> polygons, const ImageState& imageState, std::string fileName);