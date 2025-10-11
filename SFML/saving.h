#pragma once

#include "polygon.h"
#include <string>

#define NULL_SAVE_PATH "NULL"

#ifdef _WIN32
#define FILE_SEPARATOR '\\'
#else
#define FILE_SEPARATOR '/'
#endif

struct ViewState {
    float zoomLevel;
    float centerX;
    float centerY;
};

struct ImageState {
    bool hasImage = false;
    std::string imagePath = "";
    float positionX = 0.0f;
    float positionY = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float opacity = 0.7f;
    bool enabled = false;
};

std::string getExecutablePath();

bool saveToFile(std::vector<Polygon> polygons, const ImageState& imageState, std::string fileLocation, const ViewState& viewState);

std::pair<std::vector<Polygon>, std::pair<ImageState, ViewState>> openFromFile(std::string fileLocation);

void quickSave(std::vector<Polygon> polygons, const ImageState& imageState, std::string fileName, const ViewState& viewState);
