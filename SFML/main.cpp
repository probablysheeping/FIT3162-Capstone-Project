#include "vectordefs.h"
#include "imgui-SFML.h"
#include "polygon.h"
#include "saving.h"
#include "filelocationchooser.h"
#include "logging.h"
#include "tutorial.h"
#include "actions.h"

#include <SFML/Graphics.hpp>
// TODO: Set up boost.geometry
#include <iostream>
#include <string>


#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 800
#define FRAME_LIMIT 60
#define WINDOW_DISPLAY_NAME "Convex Polygon IoU"

static bool selectedpolygon = false;

void adjustVertices(std::vector<ImVec2>& vertices) {
    /*
    For each angle check if it is close to 90, 60, 45, 30 or 0 degrees (or 180, etc i cbf typing them all out)
    Vertices MUST BE ORDERED CLOCKWISE
    */
    ImVec2 p, q, r;
    const float delta = 5;
    const int angles[] = { 30, 45, 60, 90, 120, 135, 150, 180 };
    const int n = vertices.size();
    ImVec2 unitvec; //not rly unit vec cuz its not magnitude 1 but pretend it is.
    float t, o1, o2, pqr;
    bool adjusted = false;
    float angle2, angle3, dot;

    ImVec2 qr;

    std::vector<ImVec2> result;
    for (int i = 0; i < n; i++) {
        p = vertices.at(i - 1 >= 0 ? i - 1 : i - 1 + n);
        q = vertices.at(i);
        r = vertices.at(i + 1 < n ? i + 1 : i + 1 - n);
        pqr = static_cast<float>(angle(p, q, r)*180/M_PI);
        logger << currentDateTime() << " PQR: " << pqr << std::endl;
        for (int x : angles) {
            if (abs(abs(pqr) - x) <= delta) {

                // using vector projections
                // we need to get a vector in the direction of "where we want QR' to face" (where R' is adjusted vertex)

                if (i == 1) {
                    q = result.at(0);

                }
                else if (i > 1) {
                    q = result.at(i - 1);
                    p = result.at(i - 2);

                }

                o1 = sgn(sideOfLine(p, q, { q.x + 1, q.y }));
                o2 = sgn(sideOfLine(r, p, q));

                angle2 = angle(p, q, { q.x + 1,q.y }); //angle between [0, pi]
                angle3 = -o1*angle2 + o2 * x * M_PI / 180;
                
                unitvec = {std::cosf(angle3), -std::sinf(angle3)};


                //finally project QR onto Q + unitvec
                qr = { r.x - q.x, r.y - q.y };
                dot = dotProduct(qr, unitvec);

                result.push_back({ q.x + dot * unitvec.x, q.y + dot * unitvec.y });
                adjusted = true;
            }
        }

        if (!adjusted) {
            result.push_back(r);
        }


        adjusted = false;    
    }
    vertices = result;

}

void createToolTip(const char* toolTipStr, bool tooltipsEnabled)
{
    if (ImGui::IsItemHovered() && tooltipsEnabled) {
        ImGui::BeginTooltip();
        ImGui::Text(toolTipStr);
        ImGui::EndTooltip();
    }
}


/// <summary>
/// This is the SFML window where polygons will appear.
/// </summary>
void runProgram()
{
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    // One can set the dimensions based on screen size.
    sf::RenderWindow window(sf::VideoMode(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT)), WINDOW_DISPLAY_NAME);
    window.setSize(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT));
    window.setFramerateLimit(FRAME_LIMIT);
    if (!ImGui::SFML::Init(window))
        throw std::runtime_error("SFML Window could not initialise!");

    sf::View view = window.getDefaultView();
    float zoomLevel = 1.0f;
    const float zoomSpeed = 0.1f;
    const float minZoom = 0.1f;
    const float maxZoom = 5.0f;
    bool isPanning = false;
    sf::Vector2f panStart;

    sf::Clock deltaClock;

    // This is the data for a polygon not the actual displayed shape
    struct {
        bool createPolygon = false;
        bool movePolygon = false;
    } status;

    // Settings
    bool logSavingEnabled = true;
    bool autosaveEnabled = true;
    bool fullscreenEnabled = false;
    bool tooltipsEnabled = true;

    float polygonColour[3] = { 0.f, 0.f, 0.f };

    std::vector<Polygon> polygons;

    // Creating Polygon Variables
    Polygon newPolygon;
    std::vector<ImVec2> vertices;
    std::vector<sf::Vertex> newPolygonOutline;

    bool firstVertex = true;
    bool test = true;
    std::vector<int> selectedPolygons;

    double area = -1;
    double IoUArea = -1;
    double IoUMetric = -1;

    // Autosaving clock
    sf::Clock autosaveClock;

    // Setup actions
    Actions actions;

    // Clipboard and undo/redo action
    std::vector<Polygon> clipboard;
    ImVec2 undoVertex;
    sf::Vertex undoPolygonOutline;

    // Setup tutorial
    Tutorial tutorial;
    tutorial.addStep("Welcome", "Welcome to Convex Polygon IoU. Let's run through how this program works!");
    tutorial.addStep("Selecting a Colour", "Before choosing a polygon, you must first select its colour.\n\nThe program provides several ways to select your desired colour:\n\n1. Colour Picker\n    - On the left you will see a colour square.\n    - Click anywhere inside the square to pick a base colour.\n    - The selected colour will appear in the \"Select Colour\" preview box to the right.\n\n2. Using the RGB and HSV Sliders\n    - Below the colour picker, there are boxes for RGB and HSV values. Each box acts as a slider. Drag to increase/decrease values (0-255).\n\n3. Hex Codes\n    - At the bottom, there is a hexidecimal input field.\n    - Click the box and type your desired hex value, the colour will actively update as you type.");
    tutorial.addStep("Drawing a Polygon", "After choosing a colour, the nexts step is to draw your polygon.\nThis is done by selecting vertices on the canvas.\n\n1. Placing vertices\n    - Click anywhere on the canvas to place the first vertex.\n    - Continue clicking to add additional vertices. Each click will mark a new corner of your polygon.\n\n2. Connecting the Shape\n    - As vertices are added, lines will automatically connect them.\n    - When you click back on the origin vertex, the outline closes and the polygon is created.\n\nNote that at any point after your polygon is created, you can change the colour. Simply select the polygon you wish to recolour and go back through the steps as in the previous page.");
    tutorial.addStep("Calculating the Intersection over Union", "Once two polygons have been created, we can measure their IoU value.\n\n1. Selecting Polygons\n    - Click on the first polygon you wish to calculate the IoU value for. The selected polygon will be outlined with a cyan line edge.\n    - Select the second polygon.\n\n2. Click the \"Compute IoU\" button.\n    - The calculated IoU will be printed in the bottom of the polygon creator menu.");
    tutorial.addStep("Deleting Polygons", "Deleting a polygon is as simple as selecting the single polygon you wish to delete (outlined in cyan), and clicking the \"Delete Polygon\" button in the menu.");
    tutorial.addStep("Shortcuts", "Ctrl+O - Open File\nCtrl+S - Save File\nCtrl+Shift+S - Save File As\nCtrl+Z - Undo Vertex\nCtrl+Shift+Z - Redo Vertex\nCtrl+X - Cut Polygons\nCtrl+C - Copy Polygons\nCtrl+V - Paste Polygons\nDel - Delete Polygons\nEsc - Clear Selected Polygons\n");
    tutorial.addStep("End", "You have successfully completed the tutorial! If you want to go through this tutorial again please press the 'Tutorial' button on the main menu bar.");

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {

                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    if (status.createPolygon) {
                        sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
                        ImVec2 mousepos = ImVec2(worldPos.x, worldPos.y);
                        if (!firstVertex && distanceL2(mousepos, vertices.front()) <= 10) {
                            if (vertices.size() < 3) {
                                logger << currentDateTime() << " ERROR: Number of vertices less than three and instead is " << vertices.size() << std::endl;
                            }
                            else {

                                newPolygon.setVertices(vertices);

                                //orient vertices clockwise
                                if (sgn(newPolygon.signedArea()) == -1) {
                                    std::reverse(vertices.begin(), vertices.end());
                                }

                                adjustVertices(vertices);
                                newPolygon.setVertices(vertices);
                                newPolygon.setColour(polygonColour);

                                logger << currentDateTime() << " NEW " << newPolygon;

                                polygons.push_back(newPolygon);

                                vertices.clear();
                                newPolygon = Polygon();
                                status.createPolygon = false;

                                if (autosaveEnabled) {
                                    sf::Vector2f center = view.getCenter();
                                    ViewState viewState{ zoomLevel, center.x, center.y };
                                    quickSave(polygons, "autosave.sav", viewState);
                                }
                            }
                        }
                        else {

                            vertices.push_back(mousepos);

                            newPolygonOutline.back().position = mousepos;
                            newPolygonOutline.push_back(sf::Vertex{ mousepos, sf::Color::Black });
                            undoVertex = ImVec2(-1, -1);
                            firstVertex = false;
                        }
                    }
                    else {
                        // Left click to select a polygon. We unfortunately need to check each one.
                        sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
                        ImVec2 mousepos = ImVec2(worldPos.x, worldPos.y);
                        Polygon* polygon;
                        int i;
                        for (i = 0; i < polygons.size(); i++) {
                            polygon = &polygons.at(i);
                            if (std::find(selectedPolygons.begin(), selectedPolygons.end(), i) == selectedPolygons.end() && polygon->pointInPolygon(mousepos)) {

                                selectedPolygons.push_back(i);
                                polygon->render.setOutlineThickness(1.f);
                                polygon->render.setOutlineColor(sf::Color::Cyan);

                                break;
                            }

                        }
                        if (selectedPolygons.size() == 2) {
                            Polygon intersection = intersectingPolygon(&polygons.at(selectedPolygons.at(0)), &polygons.at(selectedPolygons.at(1)));

                            logger << currentDateTime() << " Intersection " << intersection;

                            area = polygons.at(selectedPolygons.at(0)).polygonArea() + polygons.at(selectedPolygons.at(1)).polygonArea() - intersection.polygonArea();
                        }
                        if (selectedPolygons.size() == 1) {
                            area = polygons.at(selectedPolygons.at(0)).polygonArea();
                        }
                        else {
                            area = -1;
                        }
                    }
                }
                if (mouseButtonPressed->button == sf::Mouse::Button::Right) {
                    isPanning = true;
                    panStart = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
				}
            }

            if (const auto mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mouseButtonReleased->button == sf::Mouse::Button::Right) {
                    isPanning = false;
                }
            }

            if (const auto mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                if (isPanning) {
                    sf::Vector2f newPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
                    sf::Vector2f delta = panStart - newPos;
                    view.move(delta);
                    window.setView(view);
                }
            }

            if (const auto mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (mouseWheel->wheel == sf::Mouse::Wheel::Vertical) {
                    // Get mouse position before zoom
                    sf::Vector2f beforeCoord = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);

                    // Zoom in or out
                    if (mouseWheel->delta > 0) {
                        // Zoom in
                        zoomLevel = std::max(minZoom, zoomLevel - zoomSpeed);
                    }
                    else {
                        // Zoom out
                        zoomLevel = std::min(maxZoom, zoomLevel + zoomSpeed);
                    }

                    view.setSize(window.getDefaultView().getSize());
                    view.zoom(zoomLevel);

                    // Get mouse position after zoom
                    sf::Vector2f afterCoord = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);

                    // Adjust view to keep mouse position consistent
                    sf::Vector2f offset = beforeCoord - afterCoord;
                    view.move(offset);

                    window.setView(view);
                }
            }

            // Menu shortcuts
            if (const auto key = event->getIf<sf::Event::KeyPressed>()) {
                // Open file
                if (key->code == sf::Keyboard::Key::O &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl))) {
                    actions.OpenFile(polygons, selectedPolygons, view, zoomLevel, window);
                }
                // Save file
                if (key->code == sf::Keyboard::Key::S &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl)) &&
                    !(sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift))) {
                    actions.SaveFile(polygons, selectedPolygons, view, zoomLevel, window);
                }
                // Save file as
                if (key->code == sf::Keyboard::Key::S &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl)) &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift))) {
                    actions.SaveFileAs(polygons, selectedPolygons, view, zoomLevel, window);
                }
                // Undo
                if (key->code == sf::Keyboard::Key::Z &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl)) &&
                    !(sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift))) {
                    actions.Undo(vertices, newPolygonOutline, status.createPolygon, undoVertex, undoPolygonOutline);
                }
                // Redo (DISABLED)
                if (key->code == sf::Keyboard::Key::Z &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl)) &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift))) {
                    //actions.Redo(vertices, newPolygonOutline, status.createPolygon, undoVertex, undoPolygonOutline);
                }
                // Cut
                if (key->code == sf::Keyboard::Key::X &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl))) {
                    actions.CopyCut(polygons, selectedPolygons, clipboard, true);
                }
                // Copy
                if (key->code == sf::Keyboard::Key::C &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl))) {
                    actions.CopyCut(polygons, selectedPolygons, clipboard, false);
                }
                // Paste
                if (key->code == sf::Keyboard::Key::V &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl))) {
                    actions.Paste(polygons, clipboard);
                }
                // Delete
                if (key->code == sf::Keyboard::Key::Delete) {
                    actions.Delete(polygons, selectedPolygons);
                }
                // Clear selected
                if (key->code == sf::Keyboard::Key::Escape) {
                    actions.ClearSelected(polygons, selectedPolygons, area);
                }
            }
        }


        ImGui::SFML::Update(window, deltaClock.restart());

        // Menu bar
        if (ImGui::BeginMainMenuBar()) {

            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open", "CTRL+O"))
                    actions.OpenFile(polygons, selectedPolygons, view, zoomLevel, window);
                if (ImGui::MenuItem("Save", "CTRL+S"))
                    actions.SaveFile(polygons, selectedPolygons, view, zoomLevel, window);
                if (ImGui::MenuItem("Save As", "CTRL+SHIFT+S"))
                    actions.SaveFileAs(polygons, selectedPolygons, view, zoomLevel, window);

                ImGui::EndMenu();

            }
            createToolTip("Save and open files", tooltipsEnabled);

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "CTRL+Z"))
                    actions.Undo(vertices, newPolygonOutline, status.createPolygon, undoVertex, undoPolygonOutline);
                if (ImGui::MenuItem("Redo", "CTRL+SHIFT+Z", false, false))
                    actions.Redo(vertices, newPolygonOutline, status.createPolygon, undoVertex, undoPolygonOutline);
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "CTRL+X"))
                    actions.CopyCut(polygons, selectedPolygons, clipboard, true);
                if (ImGui::MenuItem("Copy", "CTRL+C"))
                    actions.CopyCut(polygons, selectedPolygons, clipboard, false);
                if (ImGui::MenuItem("Paste", "CTRL+V"))
                    actions.Paste(polygons, clipboard);

                ImGui::EndMenu();
            }
            createToolTip("Modify program state", tooltipsEnabled);

            if (ImGui::BeginMenu("Settings")) {
                ImGui::MenuItem("Logging", nullptr, &logSavingEnabled);
                ImGui::MenuItem("Autosaving", nullptr, &autosaveEnabled);
                ImGui::MenuItem("Tooltips", nullptr, &tooltipsEnabled);
                if (ImGui::MenuItem("Full Screen", nullptr, &fullscreenEnabled)) {
                    logger << currentDateTime() << " Full screen " << (fullscreenEnabled ? "enabled." : "disabled.") << std::endl;
                    window.close();
                    if (fullscreenEnabled)
                        window.create(sf::VideoMode::getDesktopMode(), WINDOW_DISPLAY_NAME, sf::State::Fullscreen);
                    else
                        window.create(sf::VideoMode::getDesktopMode(), WINDOW_DISPLAY_NAME, sf::State::Windowed);
                }
                ImGui::EndMenu();
            }
            createToolTip("Change program functionality", tooltipsEnabled);

            if (ImGui::MenuItem("Tutorial")) {
                logger << currentDateTime() << " Tutorial started.\n";
                tutorial.start();
            }
            createToolTip("Start tutorial", tooltipsEnabled);

            if (ImGui::MenuItem("Contact")) {
                logger << currentDateTime() << " User opened contact window.\n";
                ImGui::OpenPopup("Contact");
            }

            if (ImGui::BeginPopup("Contact")) {
                ImGui::Text("Please send any questions, log or crash reports to the lead developer's email here: mmun0026@student.monash.edu.");
                if (ImGui::Button("Close")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            createToolTip("Contact information", tooltipsEnabled);

            if (ImGui::MenuItem("Exit")) {
                logger << currentDateTime() << " User exit.\n";
                window.close();
            }
            createToolTip("Exit from program", tooltipsEnabled);

            if (ImGui::Button("Zoom Out")) {
				zoomLevel = std::min(maxZoom, zoomLevel + 0.2f);
                view.setSize(window.getDefaultView().getSize());
                view.zoom(zoomLevel);
                window.setView(view);
            }

            if (ImGui::Button("Zoom In")) {
                zoomLevel = std::max(minZoom, zoomLevel - 0.2f);
                view.setSize(window.getDefaultView().getSize());
                view.zoom(zoomLevel);
                window.setView(view);
            }

            if (ImGui::Button("Reset Zoom Level")) {
                zoomLevel = 1.0f;
                view.setSize(window.getDefaultView().getSize());
                view.zoom(zoomLevel);
                window.setView(view);
            }

            ImGui::EndMainMenuBar();
        }

        // Window used for creating polygons
        // Needs to be formatted properly. This is just a placeholder UI
        ImGui::SetNextWindowSize(ImVec2(350, 600));
        if (ImGui::Begin("Polygon Creator")) {
            // Create polygon toggle colour
            bool wasActive = status.createPolygon;
            if (wasActive) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.2f, 1.0f));
            }
            if (ImGui::Button("Create Polygon", ImVec2(120, 30))) {
                // Create Polygon
                status.createPolygon = true;

                vertices.clear();
                newPolygon = Polygon();
                newPolygonOutline.clear();
                newPolygonOutline.push_back(sf::Vertex{ ImVec2(0,0), sf::Color::Black });
                undoVertex = ImVec2(-1, -1);

                firstVertex = true;

                if (autosaveEnabled) {
                    sf::Vector2f center = view.getCenter();
                    ViewState viewState{ zoomLevel, center.x, center.y };
                    quickSave(polygons, "autosave.sav", viewState);
                }

                logger << currentDateTime() << " Began polygon creation.\n";
            }
            if (wasActive)
                ImGui::PopStyleColor(3);
            createToolTip("Click on canvas to create vertices", tooltipsEnabled);

            if (ImGui::Button("Delete Polygon", ImVec2(120, 30)))
                actions.Delete(polygons, selectedPolygons);
            createToolTip("Select polygon and then click delete (DEL)", tooltipsEnabled);

            // Move polygon toggle colour
            wasActive = status.movePolygon;
            if (wasActive) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.2f, 1.0f));
            }
            if (ImGui::Button("Move Polygon", ImVec2(120, 30))) {
                status.movePolygon = !status.movePolygon;
                logger << currentDateTime() << " Move polygon toggled.\n";
            }
            if (wasActive)
                ImGui::PopStyleColor(3);
            createToolTip("Select polygons and then drag to move", tooltipsEnabled);

            if (ImGui::Button("Compute IoU", ImVec2(120, 30)) && selectedPolygons.size() == 2) {
                Polygon intersection = polygons.at(selectedPolygons.at(0));
                for (int i = 1; i < selectedPolygons.size(); i++) {
                    intersection = intersectingPolygon(&intersection, &polygons.at(selectedPolygons.at(i)));
                }

                polygons.push_back(intersection);

                // TODO: Calculate IoU Metric and display result.
                IoUArea = intersection.polygonArea();
                area = polygons.at(selectedPolygons.at(0)).polygonArea() + polygons.at(selectedPolygons.at(1)).polygonArea();
                IoUMetric = IoUArea / area;

                if (autosaveEnabled) {
                    sf::Vector2f center = view.getCenter();
                    ViewState viewState{ zoomLevel, center.x, center.y };
                    quickSave(polygons, "autosave.sav", viewState);
                }

                logger << currentDateTime << " Computed IoU.\n";
            }
            createToolTip("Select polygons and then calculate", tooltipsEnabled);

            if (ImGui::Button("Clear Selected", ImVec2(120, 30))) {
                actions.ClearSelected(polygons, selectedPolygons, area);
            }
            createToolTip("Unselect all polygons (ESC)", tooltipsEnabled);

            if (ImGui::ColorPicker3("Select Colour", polygonColour)) {
                //Alter Polygon Colour
                if (!selectedPolygons.empty()) {
                    for (int i : selectedPolygons) {
                        polygons.at(i).setColour(polygonColour);
                    }
                }
            }
            createToolTip("Change selected polygon colour", tooltipsEnabled);

            ImGui::Text("Area:");
            ImGui::SameLine(); ImGui::Text("%s", area == -1 ? "" : std::to_string(area).c_str());
            ImGui::Text("IoU Area:");
            ImGui::SameLine(); ImGui::Text("%s", IoUArea == -1 ? "" : std::to_string(IoUArea).c_str());
            ImGui::Text("IoU metric:");
            ImGui::SameLine(); ImGui::Text("%s", IoUMetric == -1 ? "" : std::to_string(IoUMetric).c_str());

        }

        // When left mouse is held, move polygons
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !selectedPolygons.empty() && status.movePolygon) {
            sf::Vector2f lastWorldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
            static ImVec2 lastMousePos = ImVec2(lastWorldPos.x, lastWorldPos.y);
            
            sf::Vector2f currentWorldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
            ImVec2 currentMousePos = ImVec2(currentWorldPos.x, currentWorldPos.y);

            ImVec2 delta = { currentMousePos.x - lastMousePos.x, currentMousePos.y - lastMousePos.y };

            if (delta.x != 0 || delta.y != 0) {
                for (int i : selectedPolygons) {
                    polygons[i].translate(delta);
                }
            }

            lastMousePos = currentMousePos;
        }

        ImGui::End();

        window.clear(sf::Color::White);

        // Draw everything here

        for (Polygon polygon : polygons) {
            window.draw(polygon.render);
        }

        if (status.createPolygon && !firstVertex) {
            // Draw boundary of supposed polygon
            sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
            ImVec2 mousepos = ImVec2(worldPos.x, worldPos.y);
            newPolygonOutline.back().position = mousepos;
            window.draw(newPolygonOutline.data(), newPolygonOutline.size(), sf::PrimitiveType::LineStrip);
        }

        if (tutorial.isActive())
            tutorial.render();


        ImGui::SFML::Render(window);
        window.display();

    }

    ImGui::SFML::Shutdown();

    // Save logs to file
    if (logSavingEnabled) {
        saveLogToFile("log");
    }
}

int main()
{
    try {
        runProgram();
    }
    catch (const std::exception& ex) {
        // Try to log into your logger, but don't rely on it
        try {
            logger << "Unhandled exception: " << ex.what() << std::endl;
        }
        catch (...) {
            // logger itself broke, fallback to cerr
            std::cerr << "Logger failed while handling std::exception!" << std::endl;
        }

        // Always attempt saving log, regardless of logger state
        saveLogToFile("crash");

        // Also send to stderr for immediate visibility
        std::cerr << "Unhandled exception: " << ex.what() << std::endl;

        return 1;
    }
    catch (...) {
        try {
            logger << "Unhandled unknown exception." << std::endl;
        }
        catch (...) {
            std::cerr << "Logger failed while handling unknown exception!" << std::endl;
        }

        saveLogToFile("crash");
        std::cerr << "Unhandled unknown exception." << std::endl;

        return 2;
    }

    return 0;
}
