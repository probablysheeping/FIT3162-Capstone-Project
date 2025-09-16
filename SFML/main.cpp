#include "vectordefs.h"

#include "imgui-SFML.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "polygon.h"
#include "saving.h"
#include "filelocationchooser.h"
#include "logging.h"
#include "tutorial.h"

#include <glad/glad.h> 
#include <GLFW/glfw3.h>


#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <filesystem>
#include <cstdio>
#include <map>

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 800
#define FRAME_LIMIT 60
#define WINDOW_DISPLAY_NAME "Convex Polygon IoU"
#define ICON_SIZE 36

static bool selectedpolygon = false;

// How long until we autosave
static const sf::Time autosaveTime = sf::seconds(30.f);

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    unsigned int image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

// Open and read a file, then forward to LoadTextureFromMemory()
bool LoadTextureFromFile(const char* file_name, GLuint* out_texture, int* out_width, int* out_height)
{   
    std::cout << file_name << std::endl;
    FILE* f;
    fopen_s(&f, file_name, "rb");
    if (f == NULL)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == -1)
        return false;
    fseek(f, 0, SEEK_SET);
    void* file_data = IM_ALLOC(file_size);
    fread(file_data, 1, file_size, f);
    fclose(f);
    bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
    IM_FREE(file_data);
    return ret;
}

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
int runProgram()
{
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    // One can set the dimensions based on screen size.
    sf::RenderWindow window(sf::VideoMode(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT)), WINDOW_DISPLAY_NAME);
    window.setSize(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT));
    window.setFramerateLimit(FRAME_LIMIT);
    if (!ImGui::SFML::Init(window))
        throw std::runtime_error("SFML Window could not initialise!");

    sf::Clock deltaClock;

    // This is the data for a polygon not the actual displayed shape
    struct {
        bool drawPolygon = false;
        bool createPolygon = false;
		bool menuInteraction = false;
        bool colourPicker = false;
    } status;

    typedef struct {
        unsigned int textureID;
        ImTextureID texture;
        int width;
        int height;
    } image;

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

	ImVec2 mainMenuBarSize = ImVec2(0, 0);

    // Autosaving clock
    sf::Clock autosaveClock;

    // Setup tutorial
    Tutorial tutorial;
    tutorial.addStep("Welcome", "Welcome to Convex Polygon IoU. Let's run through how this program works");
    tutorial.addStep("Item1", "wasd");
    tutorial.addStep("End", "You have successfully completed the tutorial! If you want to go through this tutorial again please press the 'Tutorial' button on the main menu bar.");
    
    glfwInit();
    gladLoadGL();

    // Load assets

    

    std::string dirpath = std::filesystem::current_path().string();
    
    std::string icon_names[] = {"drag", "draw", "bin", "shapes", "colour-pallet", "intersect"};
    std::map<std::string, image> icons;

    for (std::string i : icon_names) {
        image img;
        bool ret = LoadTextureFromFile((dirpath + "\\assets\\" + i + ".png").c_str(), &img.textureID, &img.width, &img.height);
        IM_ASSERT(ret);
		img.texture = (ImTextureID)(intptr_t)img.textureID;
        icons[i] = img;

    }

    

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
                        ImVec2 mousepos = sf::Mouse::getPosition(window);
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
                            }
                        }
                        else {

                            vertices.push_back(mousepos);

                            newPolygonOutline.back().position = mousepos;
                            newPolygonOutline.push_back(sf::Vertex{ mousepos, sf::Color::Black });
                            firstVertex = false;
                        }
                    }
                    else {
                        if (!status.menuInteraction) {
                            if (selectedPolygons.size() > 0 and !ImGui::IsKeyDown(ImGuiKey_ModShift)) {

                                for (int j : selectedPolygons) {
                                    polygons.at(j).render.setOutlineThickness(0.f);
                                }
                                selectedPolygons.clear();
                                area = -1;

                            }

                            ImVec2 p = sf::Mouse::getPosition(window);
                            Polygon* polygon;
                            int i = 0;
                            for (i; i < polygons.size(); i++) {
                                polygon = &polygons.at(i);
                                if (std::find(selectedPolygons.begin(), selectedPolygons.end(), i) == selectedPolygons.end() && polygon->pointInPolygon(p)) {

                                    selectedPolygons.push_back(i);
                                    polygon->render.setOutlineThickness(1.f);
                                    polygon->render.setOutlineColor(sf::Color::Cyan);

                                    break;
                                }

                            }
                            if (selectedPolygons.size() == 2) {
                                Polygon intersection = intersectingPolygon(&polygons.at(selectedPolygons.at(0)), &polygons.at(selectedPolygons.at(1)));

                                area = polygons.at(selectedPolygons.at(0)).polygonArea() + polygons.at(selectedPolygons.at(1)).polygonArea() - intersection.polygonArea();
                            }
                            if (selectedPolygons.size() == 1) {
                                area = polygons.at(selectedPolygons.at(0)).polygonArea();
                            }
                            else {
                                area = -1;
                            }
                            std::cout << selectedPolygons.size() << std::endl;
                        }
                    }
                }
            }
        }
        
        

        ImGui::SFML::Update(window, deltaClock.restart());
        

        //TODO: Add functionality to main menu
        if (ImGui::BeginMainMenuBar()) {
			mainMenuBarSize = ImGui::GetWindowSize();
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open", "CTRL+O")) {
                    std::string openLocation = OpenFileDialog();
                    polygons = openFile(openLocation);
                    selectedPolygons.clear();

                    logger << currentDateTime() << " File opened from " << openLocation << std::endl;

                    logger << "Polygons in file: \n";
                    for (Polygon polygon : polygons)
                        logger << polygon;

                }

                if (ImGui::MenuItem("Save", "CTRL+S")) {
                    quickSave(polygons, "save.sav");
                }

                if (ImGui::MenuItem("Save As", "CTRL+SHIFT+S")) {
                    std::string saveLocation = SaveFileDialog() + ".sav";
                    if (saveToFile(polygons, saveLocation))
                        logger << currentDateTime() << " Saved file successfully to " << saveLocation << std::endl;
                    else
                        logger << currentDateTime() << " Saved file un-successfully to " << saveLocation << std::endl;

                }
                ImGui::EndMenu();

            }
            createToolTip("Save and open files", tooltipsEnabled);

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
                if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {} // Disabled item
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "CTRL+X")) {}
                if (ImGui::MenuItem("Copy", "CTRL+C")) {}
                if (ImGui::MenuItem("Paste", "CTRL+V")) {}
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

            if (ImGui::MenuItem("Exit")) {
                logger << currentDateTime() << " User exit.\n";
                window.close();
            }
            createToolTip("Exit from program", tooltipsEnabled);

            ImGui::EndMainMenuBar();
        }

        // Autosaving functionality
        if (autosaveClock.getElapsedTime() >= autosaveTime) {
            quickSave(polygons, "autosave.sav");
            autosaveClock.restart();
        }

        // Window used for creating polygons
        // Needs to be formatted properly. This is just a placeholder UI
        

        ImGui::SetNextWindowSize(ImVec2(280, 60));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        
        ImGui::SetNextWindowPos(ImVec2((WINDOW_WIDTH - 280) / 2, mainMenuBarSize.y));
        if (ImGui::Begin("Polygon Creator", nullptr, ImGuiWindowFlags_NoScrollWithMouse + ImGuiWindowFlags_NoScrollbar + 7)) {
            ImVec2 currentWindowSize = ImGui::GetWindowSize();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

            ImGui::SameLine();

            if (ImGui::ImageButton("Create Polygon", icons["draw"].texture, ImVec2(ICON_SIZE, ICON_SIZE))) {
                // Create Polygon
                status.createPolygon = true;

                vertices.clear();
                newPolygon = Polygon();
                newPolygonOutline.clear();
                newPolygonOutline.push_back(sf::Vertex{ ImVec2(0,0), sf::Color::Black });

                firstVertex = true;

            }
            createToolTip("Click on canvas to create vertices", tooltipsEnabled);
            ImGui::SameLine();
			
            if (ImGui::ImageButton("Select Shape", icons["shapes"].texture, ImVec2(ICON_SIZE, ICON_SIZE))) {
                // Select Shape
            }
            createToolTip("Click to create polygon", tooltipsEnabled);
            ImGui::SameLine();
            
            if (ImGui::ImageButton("Delete Polygon", icons["bin"].texture, ImVec2(ICON_SIZE, ICON_SIZE))) {
                // Delete Polygon
                for (int i : selectedPolygons) {
                    polygons.erase(polygons.begin() + i);
                    logger << currentDateTime() << ": Polygon deleted at " << i << "\n";

                }
                selectedPolygons.clear();
                
            }
            createToolTip("Select polygon and then click delete", tooltipsEnabled);

            ImGui::SameLine();
            if (ImGui::ImageButton("Colour", icons["colour-pallet"].texture, ImVec2(ICON_SIZE, ICON_SIZE))) {
                // Change Colour
                status.colourPicker = true;
			}
			createToolTip("Change selected polygon colour", tooltipsEnabled);
           
            

            ImGui::SameLine();
            if (ImGui::ImageButton("Compute IoU", icons["intersect"].texture, ImVec2(ICON_SIZE, ICON_SIZE)) && selectedPolygons.size() == 2) {
                // Save when computing IoU
                if (autosaveEnabled)
                    quickSave(polygons, "autosave.sav");

                Polygon intersection = polygons.at(selectedPolygons.at(0));
                for (int i = 1; i < selectedPolygons.size(); i++) {
                    intersection = intersectingPolygon(&intersection, &polygons.at(selectedPolygons.at(i)));
                }

                intersection.setColour(polygonColour);
                polygons.push_back(intersection);

                // TODO: Calculate IoU Metric and display result.
                IoUArea = intersection.polygonArea() / (polygons.at(selectedPolygons.at(0)).polygonArea() + polygons.at(selectedPolygons.at(1)).polygonArea());
            }
            createToolTip("Select polygons and then calculate", tooltipsEnabled);
            /*
            
            */
            createToolTip("Change selected polygon colour", tooltipsEnabled);
            ImGui::PopStyleColor();
            ImGui::Text("Area:");
            ImGui::SameLine(); ImGui::Text("%s", area == -1 ? "" : std::to_string(area).c_str());
            ImGui::Text("IoU metric:");
            ImGui::SameLine(); ImGui::Text("%s", IoUArea == -1 ? "" : std::to_string(IoUArea).c_str());
            
        }

        if (status.colourPicker) {
            ImGui::BeginChild("Select Colour");
            if (ImGui::ColorPicker3("Select Colour", polygonColour)) {
                //Alter Polygon Colour
                if (!selectedPolygons.empty()) {
                    for (int i : selectedPolygons) {
                        polygons.at(i).setColour(polygonColour);
                    }
                }
            }
            ImGui::EndChild();
        }
        
        ImGui::End();

        ImGui::PopStyleColor();
        window.clear(sf::Color({ 243,243,243,1 }));

        // Draw everything here

        for (Polygon polygon : polygons) {
            window.draw(polygon.render);
        }

        if (status.createPolygon && !firstVertex) {
            // Draw boundary of supposed polygon
            ImVec2 mousepos = sf::Mouse::getPosition(window);
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
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{   



    

    try {
        return runProgram();
    }
    catch (const std::exception& ex) {
        logger << "Unhandled exception: " << ex.what() << std::endl;
        saveLogToFile("crash");
        return 1;
    }

    
    return 0;

}
