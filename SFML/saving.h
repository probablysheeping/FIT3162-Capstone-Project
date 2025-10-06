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

std::string getExecutablePath();

bool saveToFile(std::vector<Polygon> polygons, std::string fileLocation, const ViewState& viewState);

std::pair<std::vector<Polygon>, ViewState> openFromFile(std::string fileLocation);

void quickSave(std::vector<Polygon> polygons, std::string fileName, const ViewState& viewState);
