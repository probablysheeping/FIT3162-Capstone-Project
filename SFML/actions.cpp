#include "actions.h"
#include "filelocationchooser.h"
#include "saving.h"
#include "logging.h"

void Actions::OpenFile(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons, sf::View& view, float& zoomLevel, const sf::RenderWindow& window)
{
    std::string openLocation = OpenFileDialog();

	auto [loadedPolygons, viewState] = openFile(openLocation);

    polygons = loadedPolygons;
    selectedPolygons.clear();

    logger << currentDateTime() << " File opened from " << openLocation << std::endl;

    logger << "Polygons in file: \n";
    for (Polygon polygon : polygons)
        logger << polygon;

    zoomLevel = viewState.zoomLevel;
    view = window.getDefaultView();

    view.setCenter(sf::Vector2f(viewState.centerX, viewState.centerY));
    view.zoom(zoomLevel);

    logger << " - Zoom: " << viewState.zoomLevel << ", Center: (" << viewState.centerX << ", " << viewState.centerY << ")" << std::endl;
}

void Actions::SaveFile(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons, sf::View& view, float& zoomLevel, const sf::RenderWindow& window)
{
	sf::Vector2f center = view.getCenter();
	ViewState viewState = { zoomLevel, center.x, center.y };
    quickSave(polygons, "save.sav", viewState);
}

void Actions::SaveFileAs(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons, sf::View& view, float& zoomLevel, const sf::RenderWindow& window)
{
    std::string saveLocation = SaveFileDialog() + ".sav";
	sf::Vector2f center = view.getCenter();
	ViewState viewState = { zoomLevel, center.x, center.y };
    if (saveToFile(polygons, saveLocation, viewState))
        logger << currentDateTime() << " Saved file successfully to " << saveLocation << std::endl;
    else
        logger << currentDateTime() << " Saved file un-successfully to " << saveLocation << std::endl;
}

void Actions::Undo(std::vector<ImVec2>& vertices, std::vector<sf::Vertex>& newPolygonOutline, bool& createPolygon, ImVec2& undoVertex, sf::Vertex& undoPolygonOutline)
{
    if (createPolygon)
    {
        if (vertices.size() <= 1)
            createPolygon = false;
        else {
            undoVertex = vertices.back();
            undoPolygonOutline = newPolygonOutline.back();
            vertices.pop_back();
            newPolygonOutline.pop_back();
        }
        logger << currentDateTime() << " Undo on vertex\n";
    }
}

void Actions::Redo(std::vector<ImVec2>& vertices, std::vector<sf::Vertex>& newPolygonOutline, bool& createPolygon, ImVec2& undoVertex, sf::Vertex& undoPolygonOutline)
{
    if (createPolygon)
    {
        if (undoVertex.x != -1) {
            vertices.push_back(undoVertex);
            newPolygonOutline.push_back(undoPolygonOutline);
            undoVertex = ImVec2(-1, -1);
            logger << currentDateTime() << " Redo on vertex\n";
        }
    }
}

void Actions::CopyCut(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons, std::vector<Polygon>& clipboard, bool cut)
{
    clipboard.clear();
    for (int i : selectedPolygons) {
        clipboard.push_back(polygons[i]);
        if (cut)
            polygons.erase(polygons.begin() + i);
    }
    selectedPolygons.clear();
    if (cut)
        logger << currentDateTime() << " Polygons cut.\n";
    else
        logger << currentDateTime() << " Polygons copied.\n";
}

void Actions::Paste(std::vector<Polygon>& polygons, std::vector<Polygon>& clipboard)
{
    for (Polygon polygon : clipboard)
        polygons.push_back(polygon);
}

void Actions::Delete(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons)
{
    for (int i : selectedPolygons)
        polygons.erase(polygons.begin() + i);
    selectedPolygons.clear();
    logger << currentDateTime() << " Polygon deleted.\n";
}

void Actions::ClearSelected(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons, double& area)
{
    for (int j : selectedPolygons) {
        polygons.at(j).render.setOutlineThickness(0.f);
    }
    selectedPolygons.clear();
    area = -1;
    logger << currentDateTime() << " Clear selected.\n";
}