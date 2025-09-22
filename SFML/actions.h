#pragma once

#include "polygon.h"


class Actions {
public:
	void OpenFile(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons);
	void SaveFile(std::vector<Polygon>& polygons);
	void SaveFileAs(std::vector<Polygon>& polygons);
	void Undo(std::vector<ImVec2>& vertices, std::vector<sf::Vertex>& newPolygonOutline, bool& createPolygon, ImVec2& undoVertex, sf::Vertex& undoPolygonOutline);
	void Redo(std::vector<ImVec2>& vertices, std::vector<sf::Vertex>& newPolygonOutline, bool& createPolygon, ImVec2& undoVertex, sf::Vertex& undoPolygonOutline);
	void CopyCut(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons, std::vector<Polygon>& clipboard, bool cut);
	void Paste(std::vector<Polygon>& polygons, std::vector<Polygon>& clipboard);
	void Delete(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons);
	void ClearSelected(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons, double& area);
};