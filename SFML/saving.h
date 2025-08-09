#pragma once

#include "polygon.h"

#define NULL_SAVE_PATH "NULL"

std::string getExecutablePath();

bool saveToFile(std::vector<Polygon> polygons, std::string fileLocation);

std::vector<Polygon> openFile(std::string fileLocation);

void quickSave(std::vector<Polygon> polygons, std::string fileName);