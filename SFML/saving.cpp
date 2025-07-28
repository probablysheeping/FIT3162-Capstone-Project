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
	std::ifstream loadFile(fileLocation, std::ios::in | std::ios::binary);

	if (!loadFile.is_open())
		return polygons;

	std::vector<char> charBuffer((std::istreambuf_iterator<char>(loadFile)),
		std::istreambuf_iterator<char>());

	std::vector<std::byte> buffer(charBuffer.begin(), charBuffer.end());

	loadFile.close();

	size_t offset = 0;

	auto readFromBuffer = [&](auto& out) {
		using T = std::remove_reference_t<decltype(out)>;
		if (offset + sizeof(T) > buffer.size()) throw std::runtime_error("File corrupt or incomplete.");
		std::memcpy(&out, buffer.data() + offset, sizeof(T));
		offset += sizeof(T);
	};

	// Convert buffer back into polygons vector

	uint32_t polygonCount;
	readFromBuffer(polygonCount);

	for (uint32_t i = 0; i < polygonCount; ++i) {
		uint32_t vertexCount;
		readFromBuffer(vertexCount);

		Polygon polygon;

		std::vector<ImVec2> vertices;

		for (uint32_t v = 0; v < vertexCount; ++v)
		{
			ImVec2 vertex;
			readFromBuffer(vertex.x);
			readFromBuffer(vertex.y);
			vertices.push_back(vertex);
		}

		polygon.setVertices(vertices);

		float r, g, b;
		readFromBuffer(r);
		readFromBuffer(g);
		readFromBuffer(b);
		polygon.setColour(r, g, b);

		polygons.push_back(polygon);
	}
	

	return polygons;
}