#include "saving.h"

#include <iostream>
#include <fstream>
#include <cstddef>
#include <filesystem>

// I cannot pull in all of windows.h without breaking program
#ifdef _WIN32
extern "C" __declspec(dllimport)
unsigned long __stdcall GetModuleFileNameA(
	void* hModule,
	char* lpFilename,
	unsigned long nSize
);
#define MAX_PATH 260
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

template <typename T>
void appendToBuffer(std::vector<std::byte>& buffer, const T& value) {
	const std::byte* raw = reinterpret_cast<const std::byte*>(&value);
	buffer.insert(buffer.end(), raw, raw + sizeof(T));
}

/// <summary>
/// Save to file based on fileLocation
/// </summary>
/// <param name="polygons">The list of polygons to save to file</param>
/// <param name="fileLocation">The location that the save file will be written to</param>
/// <returns>A boolean output determining whether file was successfully saved or not</returns>
bool saveToFile(std::vector<Polygon>& polygons, std::string fileLocation)
{
	std::ofstream saveFile;
	saveFile.open(fileLocation, std::ios::out | std::ios::binary | std::ios::trunc);

	if (!saveFile.is_open())
		return false;

	std::vector<std::byte> buffer;

	appendToBuffer(buffer, static_cast<uint32_t>(polygons.size()));
	for (Polygon polygon : polygons)
	{
		appendToBuffer(buffer, static_cast<uint32_t>(polygon.getVertices().size()));
		for (ImVec2 vertex : polygon.getVertices())
		{
			appendToBuffer(buffer, vertex.x);
			appendToBuffer(buffer, vertex.y);
		}
		appendToBuffer(buffer, polygon.getColour(0));
		appendToBuffer(buffer, polygon.getColour(1));
		appendToBuffer(buffer, polygon.getColour(2));
	}

	saveFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
	if (!saveFile)
		return false;

	saveFile.close();
	return true;
}


std::vector<Polygon> openFile(std::string fileLocation)
{
	std::vector<Polygon> polygons;

	// Write from binary file into buffer

	// Convert buffer back into polygons vector

	return polygons;
}