#include "saving.h"

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <sstream>

#include "logging.h"

// I cannot pull in all of windows.h without breaking program
#ifdef _WIN32
extern "C" __declspec(dllimport)
unsigned long __stdcall GetModuleFileNameA(
	void* hModule,
	char* lpFilename,
	unsigned long nSize
);
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#endif

// At the moment only supporting windows for getting file path
std::string getExecutablePath()
{
#ifdef _WIN32
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::filesystem::path exePath(buffer);
	return exePath.parent_path().string();
#else
	return NULL_SAVE_PATH;
#endif
}


/// <summary>
/// Save to file based on fileLocation
/// </summary>
/// <param name="polygons">The list of polygons to save to file</param>
/// <param name="fileLocation">The location that the save file will be written to</param>
/// <returns>A boolean output determining whether file was successfully saved or not</returns>
bool saveToFile(std::vector<Polygon> polygons, std::string fileLocation)
{
	std::ofstream saveFile(fileLocation);

	// File failed to open
	if (!saveFile.is_open())
		return false;

	for (Polygon polygon : polygons)
		saveFile << polygon;
	
	// Close file
	saveFile.close();

	return true;
}

/// <summary>
/// Opens file and returns polygons
/// Re-wrote large portions of this function using AI
/// </summary>
/// <param name="fileLocation"></param>
/// <returns></returns>
std::vector<Polygon> openFile(std::string fileLocation)
{
	std::vector<Polygon> polygons;

	std::ifstream saveFile(fileLocation);

	// File failed to open
	if (!saveFile.is_open())
		return polygons;


	std::string line;
	Polygon currentPolygon;
	std::vector<ImVec2> vertices;
	bool readingPolygon = false;
	int verticesToRead = 0;

	while (std::getline(saveFile, line)) {
		std::istringstream iss(line);
		std::string word;
		iss >> word;

		if (word == "POLYGON")
		{
			readingPolygon = true;
			vertices.clear();
			verticesToRead = 0;
		}
		else if (readingPolygon && word == "VERTICES")
		{
			iss >> verticesToRead;
		}
		else if (readingPolygon && verticesToRead > 0)
		{
			// Read vertices lines
			float x, y;
			iss.clear();
			iss.str(line);
			iss >> x >> y;
			vertices.push_back(ImVec2{ x, y });
			verticesToRead--;
		}
		else if (readingPolygon && word == "COLOUR")
		{
			float r, g, b;
			iss >> r >> g >> b;
			currentPolygon.setVertices(vertices);
			float colour[3] = { r,g,b };
			currentPolygon.setColour(colour);
		}
		else if (readingPolygon && word == "END")
		{
			polygons.push_back(currentPolygon);
			readingPolygon = false;
		}
	}

	// Close file
	saveFile.close();

	return polygons;
}

/// <summary>
/// Used for auto saving and normal saving (but not save as)
/// </summary>
/// <param name="polygons"></param>
/// <param name="fileName"></param>
void quickSave(std::vector<Polygon> polygons, std::string fileName)
{
	std::string saveLocation = getExecutablePath();
	if (saveLocation != NULL_SAVE_PATH) {
		saveLocation += fileName;
		if (saveToFile(polygons, saveLocation))
			std::cout << "Saved file successfully to " << saveLocation << std::endl;
		else
			std::cout << "Saved file un-successfully to" << saveLocation << std::endl;
	}
}