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

#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#endif

// Supports Windows and Mac OS
std::string getExecutablePath()
{
#ifdef _WIN32
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::filesystem::path exePath(buffer);
	return exePath.parent_path().string();

#elif defined(__APPLE__)
	char buffer[PATH_MAX];
	uint32_t size = sizeof(buffer);
	if (_NSGetExecutablePath(buffer, &size) == 0) {
		std::filesystem::path exePath(buffer);
		return exePath.parent_path().string();
	}
	else {
		// Buffer too small, fallback to NULL_SAVE_PATH
		return NULL_SAVE_PATH;
	}

#else
	return NULL_SAVE_PATH;
#endif
}



/// <summary>
/// Save to file based on fileLocation
/// </summary>
/// <param name="polygons">The list of polygons to save to file</param>
/// <param name="imageState">The image state to save</param>
/// <param name="fileLocation">The location that the save file will be written to</param>
/// <returns>A boolean output determining whether file was successfully saved or not</returns>

bool saveToFile(std::vector<Polygon> polygons, const ImageState& imageState, std::string fileLocation, const ViewState& viewState)

{
	std::ofstream saveFile(fileLocation);

	// File failed to open
	if (!saveFile.is_open())
		return false;

	// Save image state first
	saveFile << "IMAGE_STATE\n";
	saveFile << "HAS_IMAGE " << (imageState.hasImage ? "1" : "0") << "\n";
	if (imageState.hasImage) {
		saveFile << "IMAGE_PATH " << imageState.imagePath << "\n";
		saveFile << "POSITION " << imageState.positionX << " " << imageState.positionY << "\n";
		saveFile << "SCALE " << imageState.scaleX << " " << imageState.scaleY << "\n";
		saveFile << "OPACITY " << imageState.opacity << "\n";
		saveFile << "ENABLED " << (imageState.enabled ? "1" : "0") << "\n";
		logger << currentDateTime() << " Saving image state: path=" << imageState.imagePath 
		       << ", pos=(" << imageState.positionX << "," << imageState.positionY << ")"
		       << ", scale=(" << imageState.scaleX << "," << imageState.scaleY << ")"
		       << ", opacity=" << imageState.opacity << ", enabled=" << imageState.enabled << std::endl;
	}
	saveFile << "IMAGE_END\n";

	// Save polygons
	for (Polygon polygon : polygons)
		saveFile << polygon;

	saveFile << "VIEW\n";
	saveFile << "ZOOM " << viewState.zoomLevel << "\n";	
	saveFile << "CENTER " << viewState.centerX << " " << viewState.centerY << "\n";
	saveFile << "ENDVIEW\n";
	
	// Close file
	saveFile.close();

	return true;
}

/// <summary>
/// Opens file and returns polygons and image state
/// Re-wrote large portions of this function using AI
/// </summary>
/// <param name="fileLocation"></param>
/// <returns></returns>
std::pair<std::vector<Polygon>, std::pair<ImageState, ViewState>> openFromFile(std::string fileLocation)
{
	std::vector<Polygon> polygons;
	ImageState imageState;
	ViewState viewState = { 1.0f, 0.0f, 0.0f };

	std::ifstream saveFile(fileLocation);

	// File failed to open
	if (!saveFile.is_open())
		return { polygons, { imageState, viewState } };


	std::string line;
	Polygon currentPolygon;
	std::vector<ImVec2> vertices;
	bool readingPolygon = false;
	bool readingImageState = false;
	bool readingView = false;
	int verticesToRead = 0;

	while (std::getline(saveFile, line)) {
		std::istringstream iss(line);
		std::string word;
		iss >> word;

		if (word == "IMAGE_STATE")
		{
			readingImageState = true;
		}
		else if (readingImageState && word == "HAS_IMAGE")
		{
			int hasImage;
			iss >> hasImage;
			imageState.hasImage = (hasImage == 1);
		}
		else if (readingImageState && word == "IMAGE_PATH")
		{
			std::string path;
			std::getline(iss, path);
			// Remove leading whitespace
			path.erase(0, path.find_first_not_of(" \t"));
			imageState.imagePath = path;
		}
		else if (readingImageState && word == "POSITION")
		{
			iss >> imageState.positionX >> imageState.positionY;
		}
		else if (readingImageState && word == "SCALE")
		{
			iss >> imageState.scaleX >> imageState.scaleY;
		}
		else if (readingImageState && word == "OPACITY")
		{
			iss >> imageState.opacity;
		}
		else if (readingImageState && word == "ENABLED")
		{
			int enabled;
			iss >> enabled;
			imageState.enabled = (enabled == 1);
		}
		else if (readingImageState && word == "IMAGE_END")
		{
			readingImageState = false;
			logger << currentDateTime() << " Loaded image state: hasImage=" << imageState.hasImage 
			       << ", path=" << imageState.imagePath 
			       << ", pos=(" << imageState.positionX << "," << imageState.positionY << ")"
			       << ", scale=(" << imageState.scaleX << "," << imageState.scaleY << ")"
			       << ", opacity=" << imageState.opacity << ", enabled=" << imageState.enabled << std::endl;
		}
		else if (word == "POLYGON")
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
		else if (word == "VIEW")
		{
			readingView = true;
		}
		else if (readingView && word == "ZOOM")
		{
			iss >> viewState.zoomLevel;
		}
		else if (readingView && word == "CENTER")
		{
			iss >> viewState.centerX >> viewState.centerY;
		}
		else if (readingView && word == "ENDVIEW")
		{
			readingView = false;
		}
	}

	// Close file
	saveFile.close();

	return { polygons, { imageState, viewState } };
}

/// <summary>
/// Used for auto saving and normal saving (but not save as)
/// </summary>
/// <param name="polygons"></param>
/// <param name="imageState"></param>
/// <param name="fileName"></param>
void quickSave(std::vector<Polygon> polygons, const ImageState& imageState, std::string fileName, const ViewState& viewState)
{
	std::string saveLocation = getExecutablePath();
	if (saveLocation != NULL_SAVE_PATH) {
		saveLocation += FILE_SEPARATOR + fileName;
		if (saveToFile(polygons, imageState, saveLocation, viewState))
			logger << currentDateTime() << " Saved file successfully to " << saveLocation << std::endl;
		else
			logger << currentDateTime() << " Saved file un-successfully to " << saveLocation << std::endl;
	}
}