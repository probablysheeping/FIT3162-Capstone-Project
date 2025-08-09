#include "saving.h"

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

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
	{
		for (ImVec2 vertex : polygon.getVertices())
		{
			saveFile << vertex.x << ',' << vertex.y << '\t';
		}
		saveFile << ':' << polygon.getColour(0) << ',' << polygon.getColour(1) << ',' << polygon.getColour(2) << '\n';
	}
	
	// Close file
	saveFile.close();

	return true;
}


std::vector<Polygon> openFile(std::string fileLocation)
{
	std::vector<Polygon> polygons;

	std::ifstream saveFile(fileLocation);

	// File failed to open
	if (!saveFile.is_open())
		return polygons;


	std::string line;
	while (std::getline(saveFile, line)) {
		Polygon polygon;

		std::vector<ImVec2> vertices;
		
		// String index
		int i = 0;

		// Read in vertices
		while (line[i] != ':') {
			// New vertex
			ImVec2 vertex;

			// Read in x vertex
			std::string vert_x;
			while (line[i] != ',')
				vert_x += line[i++];
			
			// Failure check
			if (line[i++] != ',')
				return polygons;

			// Read in y vertex
			std::string vert_y;
			while (line[i] != '\t')
				vert_y += line[i++];

			// Failure check
			if (i >= line.length())
				return polygons;

			vertex.x = std::stof(vert_x);
			vertex.y = std::stof(vert_y);

			vertices.push_back(vertex);

			// Increment again to get past last tab
			i++;
		}

		// Failure check
		if (line[i++] != ':')
			return polygons;
		else
			polygon.setVertices(vertices);

		// Read in colours
		std::string rgb_str[3] = { "", "", "" };
		int rgb_index = 0;
		while (i < line.length()) {
			if (line[i] == ',')
				rgb_index++;
			else
				rgb_str[rgb_index] += line[i];
			i++;
		}

		float r = std::stof(rgb_str[0]);
		float g = std::stof(rgb_str[1]);
		float b = std::stof(rgb_str[2]);

		polygon.setColour(r, g, b);

		polygons.push_back(polygon);
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