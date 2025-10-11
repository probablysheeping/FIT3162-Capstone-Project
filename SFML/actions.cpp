#include "actions.h"
#include "filelocationchooser.h"
#include "saving.h"
#include "logging.h"

#include "imgui.h"     // ImGui::GetMousePos()
#include <numeric>     // for accumulate if needed

std::pair<std::vector<Polygon>, std::pair<ImageState, ViewState>> Actions::OpenFile(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons, sf::View& view, float& zoomLevel, sf::RenderWindow& window)
{
    std::string openLocation = OpenFileDialog();

	auto [loadedPolygons, statesPair] = openFromFile(openLocation);
	auto [imageState, viewState] = statesPair;

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

    window.setView(view);

    logger << "Zoom: " << viewState.zoomLevel << ", Center: (" << viewState.centerX << ", " << viewState.centerY << ")" << std::endl;
    
    return { loadedPolygons, { imageState, viewState } };
}

void Actions::SaveFile(std::vector<Polygon>& polygons, const ImageState& imageState, std::vector<int>& selectedPolygons, sf::View& view, float& zoomLevel, const sf::RenderWindow& window)
{
	sf::Vector2f center = view.getCenter();
	ViewState viewState = { zoomLevel, center.x, center.y };
    quickSave(polygons, imageState, "save.sav", viewState);
}

void Actions::SaveFileAs(std::vector<Polygon>& polygons, const ImageState& imageState, std::vector<int>& selectedPolygons, sf::View& view, float& zoomLevel, const sf::RenderWindow& window)
{
    std::string saveLocation = SaveFileDialog() + ".sav";
	sf::Vector2f center = view.getCenter();
	ViewState viewState = { zoomLevel, center.x, center.y };
    if (saveToFile(polygons, imageState, saveLocation, viewState))
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

std::vector<int> sanitizeSelection(const std::vector<int>& selected, size_t size, const char* actionName) {
    std::vector<int> indices = selected;
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

    // remove invalid ones
    indices.erase(std::remove_if(indices.begin(), indices.end(), [=](int i) {
        if (i < 0 || i >= static_cast<int>(size)) {
            logger << currentDateTime() << " Warning: " << actionName
                << " index " << i << " out of range. Skipped.\n";
            return true;
        }
        return false;
        }), indices.end());

    return indices;
}

void Actions::CopyCut(std::vector<Polygon>& polygons,
    std::vector<int>& selectedPolygons,
    std::vector<Polygon>& clipboard,
    bool cut)
{
    clipboard.clear();
    if (selectedPolygons.empty()) {
        logger << currentDateTime() << (cut ? " Nothing to cut.\n" : " Nothing to copy.\n");
        return;
    }

    auto indices = sanitizeSelection(selectedPolygons, polygons.size(), cut ? "Cut" : "Copy");

    // Remove outlines
    for (int j : indices) {
        polygons[j].render.setOutlineThickness(0.f);
    }

    // Copy polygons
    for (int idx : indices) {
        clipboard.push_back(polygons[idx]);
    }

    // If cutting, erase in descending order
    if (cut) {
        std::sort(indices.rbegin(), indices.rend());
        for (int idx : indices) {
            polygons.erase(polygons.begin() + idx);
        }
    }

    selectedPolygons.clear();
    logger << currentDateTime() << (cut ? " Polygons cut.\n" : " Polygons copied.\n");
}

void Actions::Paste(std::vector<Polygon>& polygons, std::vector<Polygon>& clipboard)
{
    if (clipboard.empty()) {
        logger << currentDateTime() << " Clipboard empty, nothing to paste.\n";
        return;
    }

    // Get mouse position (must be called while an ImGui frame is active)
    ImVec2 mousePos = ImGui::GetMousePos();

    // Compute group centroid: mean of all vertices across all clipboard polygons
    ImVec2 groupCentroid = { 0.f, 0.f };
    size_t totalVerts = 0;
    for (Polygon poly : clipboard) {
        auto verts = poly.getVertices(); // returns copy in your header
        for (const auto& v : verts) {
            groupCentroid.x += v.x;
            groupCentroid.y += v.y;
        }
        totalVerts += verts.size();
    }
    if (totalVerts == 0) {
        // nothing to paste (degenerate)
        logger << currentDateTime() << " Clipboard polygons have no vertices.\n";
        return;
    }
    groupCentroid.x /= static_cast<float>(totalVerts);
    groupCentroid.y /= static_cast<float>(totalVerts);

    ImVec2 delta = { mousePos.x - groupCentroid.x, mousePos.y - groupCentroid.y };

    // Paste each polygon translated by delta
    for (auto poly : clipboard) { // copy on purpose
        poly.translate(delta);
        polygons.push_back(poly);
    }

    logger << currentDateTime() << " Polygons pasted at mouse position ("
        << mousePos.x << ", " << mousePos.y << ").\n";
}

void Actions::Delete(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons)
{
    if (selectedPolygons.empty()) {
        logger << currentDateTime() << " Delete called but no polygons selected.\n";
        return;
    }

    auto indices = sanitizeSelection(selectedPolygons, polygons.size(), "Delete");

    std::sort(indices.rbegin(), indices.rend());
    for (int idx : indices) {
        polygons.erase(polygons.begin() + idx);
    }

    selectedPolygons.clear();
    logger << currentDateTime() << " Polygon(s) deleted.\n";
}

void Actions::ClearSelected(std::vector<Polygon>& polygons, std::vector<int>& selectedPolygons, double& area)
{
    auto indices = sanitizeSelection(selectedPolygons, polygons.size(), "ClearSelected");

    for (int j : indices) {
        polygons[j].render.setOutlineThickness(0.f);
    }

    selectedPolygons.clear();
    area = -1;
    logger << currentDateTime() << " Clear selected.\n";
}