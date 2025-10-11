#include "vectordefs.h"

#include "imgui-SFML.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "polygon.h"
#include "saving.h"
#include "filelocationchooser.h"
#include "logging.h"
#include "tutorial.h"
#include "actions.h"

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include <fstream>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <filesystem>
#include <cstdio>
#include <map>

int WINDOW_WIDTH = 1000;
int WINDOW_HEIGHT = 800;
#define FRAME_LIMIT 60
#define WINDOW_DISPLAY_NAME "Convex Polygon IoU"
int ICON_SIZE = 36;

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

#include <fstream>
bool LoadTextureFromFile(const char* file_name, GLuint* out_texture, int* out_width, int* out_height)
{
    std::cout << file_name << std::endl;
    std::ifstream file(file_name, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    std::streamsize file_size = file.tellg();
    if (file_size <= 0)
        return false;

    file.seekg(0, std::ios::beg);
    void* file_data = IM_ALLOC(static_cast<size_t>(file_size));
    if (!file.read(reinterpret_cast<char*>(file_data), file_size)) {
        IM_FREE(file_data);
        return false;
    }
    file.close();

    bool ret = LoadTextureFromMemory(file_data, static_cast<size_t>(file_size), out_texture, out_width, out_height);
    IM_FREE(file_data);
    return ret;
}

/*

// Open and read a file, then forward to LoadTextureFromMemory()
bool LoadTextureFromFile(const char* file_name, GLuint* out_texture, int* out_width, int* out_height)
{   
    std::cout << file_name << std::endl;
    FILE* f = nullptr;
    if (fopen_s(&f, file_name, "rb") != 0 || f == nullptr)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == (size_t)-1)
    {
        fclose(f);
        return false;
    }
    fseek(f, 0, SEEK_SET);
    void* file_data = IM_ALLOC(file_size);
    fread(file_data, 1, file_size, f);
    fclose(f);
    bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
    IM_FREE(file_data);
    return ret;
}
*/

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
        ImGui::Text("%s", toolTipStr);
        ImGui::EndTooltip();
    }
}

ImVec2 getMousePos(sf::RenderWindow &window) {
    sf::View view = window.getDefaultView();
    sf::Vector2f lastWorldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
    static ImVec2 lastMousePos = ImVec2(lastWorldPos.x, lastWorldPos.y);
    sf::Vector2f currentWorldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
    return ImVec2(currentWorldPos.x, currentWorldPos.y);
}

/// <summary>
/// Draws a grid background on the window
/// </summary>
/// <param name="window">The SFML render window</param>
/// <param name="gridSize">Size of each grid cell in pixels</param>
/// <param name="gridColor">Color of the grid lines</param>
void drawGrid(sf::RenderWindow& window, float gridSize = 50.0f, sf::Color gridColor = sf::Color(220, 220, 220))
{
    sf::Vector2u windowSize = window.getSize();
    std::vector<sf::Vertex> gridLines;
    
    // Vertical lines
    for (float x = 0; x <= windowSize.x; x += gridSize) {
        sf::Vertex v1, v2;
        v1.position = sf::Vector2f(x, 0);
        v1.color = gridColor;
        v2.position = sf::Vector2f(x, windowSize.y);
        v2.color = gridColor;
        gridLines.push_back(v1);
        gridLines.push_back(v2);
    }
    
    // Horizontal lines
    for (float y = 0; y <= windowSize.y; y += gridSize) {
        sf::Vertex v1, v2;
        v1.position = sf::Vector2f(0, y);
        v1.color = gridColor;
        v2.position = sf::Vector2f(windowSize.x, y);
        v2.color = gridColor;
        gridLines.push_back(v1);
        gridLines.push_back(v2);
    }
    
    window.draw(gridLines.data(), gridLines.size(), sf::PrimitiveType::Lines);
}

/// <summary>
/// Loads a background image from file with specified resize mode
/// </summary>
/// <param name="texture">The texture to load the image into</param>
/// <param name="sprite">The sprite to display the image</param>
/// <param name="filename">Path to the image file</param>
/// <param name="windowSize">Size of the window for scaling</param>
/// <param name="resizeMode">0: Fit to window, 1: Original size, 2: Custom size</param>
/// <param name="customWidth">Custom width (used when resizeMode = 2)</param>
/// <param name="customHeight">Custom height (used when resizeMode = 2)</param>
/// <returns>True if image loaded successfully</returns>
bool loadBackgroundImage(sf::Texture& texture, sf::Sprite& sprite, const std::string& filename, sf::Vector2u windowSize, int resizeMode = 0, float customWidth = 800.0f, float customHeight = 600.0f)
{
    if (!texture.loadFromFile(filename)) {
        logger << currentDateTime() << " ERROR: Failed to load image from " << filename << std::endl;
        return false;
    }
    
    sprite.setTexture(texture);
    
    sf::Vector2u imageSize = texture.getSize();
    float scaleX = 1.0f, scaleY = 1.0f;
    sf::Vector2f position(0.0f, 0.0f);
    
    switch (resizeMode) {
        case 0: // Fit to window
        {
            scaleX = static_cast<float>(windowSize.x) / imageSize.x;
            scaleY = static_cast<float>(windowSize.y) / imageSize.y;
            float scale = std::min(scaleX, scaleY);
            scaleX = scaleY = scale;
            
            // Center the image
            sf::Vector2f scaledSize = sf::Vector2f(imageSize.x * scale, imageSize.y * scale);
            position = sf::Vector2f((windowSize.x - scaledSize.x) / 2.0f, (windowSize.y - scaledSize.y) / 2.0f);
            break;
        }
        case 1: // Original size
        {
            scaleX = scaleY = 1.0f;
            // Center the image
            position = sf::Vector2f((windowSize.x - imageSize.x) / 2.0f, (windowSize.y - imageSize.y) / 2.0f);
            break;
        }
        case 2: // Custom size
        {
            scaleX = customWidth / imageSize.x;
            scaleY = customHeight / imageSize.y;
            // Center the image
            position = sf::Vector2f((windowSize.x - customWidth) / 2.0f, (windowSize.y - customHeight) / 2.0f);
            break;
        }
    }
    
    sprite.setScale(sf::Vector2f(scaleX, scaleY));
    sprite.setPosition(position);
    
    logger << currentDateTime() << " Image loaded successfully from " << filename << " with resize mode " << resizeMode << std::endl;
    return true;
}

/// <summary>
/// Gets the bounding rectangle of the current image
/// </summary>
sf::FloatRect getImageBounds(sf::Sprite* sprite) {
    if (!sprite) return sf::FloatRect(sf::Vector2f(0, 0), sf::Vector2f(0, 0));
    return sprite->getGlobalBounds();
}

/// <summary>
/// Gets resize handle rectangles for the image
/// </summary>
std::vector<sf::FloatRect> getResizeHandles(sf::FloatRect imageBounds, float handleSize) {
    std::vector<sf::FloatRect> handles;
    float hs = handleSize / 2.0f;
    
    // Corner handles (0-3)
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x - hs, imageBounds.position.y - hs), sf::Vector2f(handleSize, handleSize))); // Top-left
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x + imageBounds.size.x - hs, imageBounds.position.y - hs), sf::Vector2f(handleSize, handleSize))); // Top-right
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x + imageBounds.size.x - hs, imageBounds.position.y + imageBounds.size.y - hs), sf::Vector2f(handleSize, handleSize))); // Bottom-right
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x - hs, imageBounds.position.y + imageBounds.size.y - hs), sf::Vector2f(handleSize, handleSize))); // Bottom-left
    
    // Edge handles (4-7)
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x + imageBounds.size.x/2 - hs, imageBounds.position.y - hs), sf::Vector2f(handleSize, handleSize))); // Top
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x + imageBounds.size.x - hs, imageBounds.position.y + imageBounds.size.y/2 - hs), sf::Vector2f(handleSize, handleSize))); // Right
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x + imageBounds.size.x/2 - hs, imageBounds.position.y + imageBounds.size.y - hs), sf::Vector2f(handleSize, handleSize))); // Bottom
    handles.push_back(sf::FloatRect(sf::Vector2f(imageBounds.position.x - hs, imageBounds.position.y + imageBounds.size.y/2 - hs), sf::Vector2f(handleSize, handleSize))); // Left
    
    return handles;
}

/// <summary>
/// Draws resize handles for the image
/// </summary>
void drawResizeHandles(sf::RenderWindow& window, sf::FloatRect imageBounds, float handleSize) {
    auto handles = getResizeHandles(imageBounds, handleSize);
    
    for (const auto& handle : handles) {
        sf::RectangleShape handleRect(sf::Vector2f(handleSize, handleSize));
        handleRect.setPosition(handle.position);
        handleRect.setFillColor(sf::Color::White);
        handleRect.setOutlineColor(sf::Color::Black);
        handleRect.setOutlineThickness(1.0f);
        window.draw(handleRect);
    }
    
    // Draw selection border
    sf::RectangleShape border(imageBounds.size);
    border.setPosition(imageBounds.position);
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color::Blue);
    border.setOutlineThickness(2.0f);
    window.draw(border);
}

/// <summary>
/// Creates an ImageState from current application state
/// </summary>
ImageState getCurrentImageState(bool hasImage, const std::string& imagePath, sf::Sprite* sprite, float opacity, bool enabled) {
    ImageState state;
    state.hasImage = hasImage;
    state.imagePath = imagePath;
    state.opacity = opacity;
    state.enabled = enabled;
    
    if (hasImage && sprite) {
        sf::Vector2f position = sprite->getPosition();
        sf::Vector2f scale = sprite->getScale();
        state.positionX = position.x;
        state.positionY = position.y;
        state.scaleX = scale.x;
        state.scaleY = scale.y;
    }
    
    return state;
}

/// <summary>
/// This is the SFML window where polygons will appear.
/// </summary>

#include <filesystem>
int runProgram()
{
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    // One can set the dimensions based on screen size.

    float SCALE_FACTOR = 1.f;

    
    WINDOW_HEIGHT *= SCALE_FACTOR;
    WINDOW_WIDTH *= SCALE_FACTOR;
    ICON_SIZE *= SCALE_FACTOR;

    
    sf::RenderWindow window(
        sf::VideoMode(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT)),
        WINDOW_DISPLAY_NAME,
        sf::Style::Titlebar | sf::Style::Close
);
    window.setSize(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT));
    window.setFramerateLimit(FRAME_LIMIT);


    std::filesystem::path assetPath = "assets";
    if (!std::filesystem::exists(assetPath)) {
        assetPath = std::filesystem::current_path().parent_path() / "assets";
    }
    if (!ImGui::SFML::Init(window))
        throw std::runtime_error("SFML Window could not initialise!");

    ImGuiIO& io = ImGui::GetIO();

    std::cout << "File path:" << assetPath << std::endl;
    
    
    ImFont* largeFont = io.Fonts->AddFontFromFileTTF(
        (assetPath / "Roboto-VariableFont_wdth,wght.ttf").string().c_str(), // Use the correct extension if it's .ttf or .otf
        18.0f * SCALE_FACTOR
    );
    io.FontDefault = largeFont; // Set as default font
    ImGui::SFML::UpdateFontTexture();

    
    ImGui::GetStyle().ScaleAllSizes(SCALE_FACTOR);
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
        bool menuInteraction = false;
        bool adjustVertices = true;
        bool draggingPolygons = false;
        bool draggingVertex = false;
        bool createTemplatePolygon = false;
        bool createTemplateCircle = false;
        bool templateStage = 0; //0 for first part, 1 for second part (eg top left bot right corners)
		ImVec2 lastMousePos = ImVec2(0, 0);
        bool movePolygon = false;
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
    bool gridEnabled = true;
    float gridSize = 50.0f;
    bool imageEnabled = false;
    float imageOpacity = 0.7f;
    float imageScale = 1.0f;

    float polygonColour[3] = { 0.f, 0.f, 0.f };

    std::vector<Polygon> polygons;

    // Background image variables
    sf::Texture backgroundTexture;
    sf::Sprite* backgroundSprite = nullptr;
    bool hasBackgroundImage = false;
    std::string currentImagePath = "";
    
    // Image resize dialog variables
    bool showImageResizeDialog = false;
    std::string pendingImagePath = "";
    int resizeMode = 0; // 0: Fit to window, 1: Original size, 2: Custom size
    float customWidth = 800.0f;
    float customHeight = 600.0f;
    sf::Vector2u originalImageSize;
    
    // Interactive image resize variables
    bool imageResizeMode = false;
    bool isDraggingImage = false;
    int dragHandle = -1; // -1: none, 0-3: corners, 4-7: edges, 8: move
    sf::Vector2f dragStartPos;
    sf::Vector2f imageStartPos;
    sf::Vector2f imageStartSize;
    const float handleSize = 8.0f;

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

	ImVec2 mainMenuBarSize = ImVec2(0, 0);
    
    
    // Autosaving clock
    sf::Clock autosaveClock;
    const sf::Time autosaveTime = sf::seconds(60); // Autosave every 60 seconds
    
    glfwInit();
    gladLoadGL();

    // Load assets

    

    std::string dirpath = std::filesystem::current_path().string();
    //
    std::string icon_names[] = {"drag", "draw", "bin", "shapes", "colour-pallet", "intersect", "circle", "triangle", "rectangle", "square" };
    std::map<std::string, image> icons;

    for (std::string i : icon_names) {
        image img;
        std::cout << (assetPath / (i + ".png")).string() << std::endl;
        bool ret = LoadTextureFromFile((assetPath / (i + ".png")).string().c_str(), &img.textureID, &img.width, &img.height);
        IM_ASSERT(ret);
		img.texture = (ImTextureID)(intptr_t)img.textureID;
        icons[i] = img;

    }
    
    // Welcome
    tutorial.addStep("Welcome! 👋", 
        "Welcome to Convex Polygon IoU!\n\n"
        "This quick tutorial will show you how to use the main features.\n\n"
        "You can skip this tutorial at any time and restart it from the Help menu.",
        TutorialTargetType::NONE);
    
    // Menu bar overview
    tutorial.addStep("Menu Bar", 
        "This is the menu bar where you can access File operations, Edit tools, View options, and Help.",
        TutorialTargetType::MENU_BAR);
    
    // File menu
    tutorial.addStep("File Menu", 
        "Use the File menu to:\n"
        "• Open existing polygon files (Ctrl+O)\n"
        "• Save your work (Ctrl+S)\n"
        "• Save to a new file (Ctrl+Shift+S)\n"
        "• Import background images",
        TutorialTargetType::FILE_MENU);
    
    // Polygon Creator window
    tutorial.addStep("Your Workspace", 
        "This is your main workspace - the Polygon Creator panel.\n\n"
        "Here you can create polygons, select colors, and calculate IoU values.",
        TutorialTargetType::POLYGON_CREATOR);
    
    // Color picker
    tutorial.addStep("Choose a Color", 
        "Before creating a polygon, select a color here.\n\n"
        "• Click in the color square to pick a color\n"
        "• Use RGB/HSV sliders for precise control\n"
        "• Enter hex codes directly",
        TutorialTargetType::COLOR_PICKER);
    
    // Create polygon button
    tutorial.addStep("Create Polygon", 
        "Click this button to start drawing a polygon.\n\n"
        "Then click on the canvas to place vertices. Click the first vertex again to close the polygon.",
        TutorialTargetType::CREATE_BUTTON);
    
    // Canvas
    tutorial.addStep("The Canvas", 
        "This is where you draw and interact with polygons.\n\n"
        "• Click to place vertices when creating\n"
        "• Click polygons to select them (cyan outline)\n"
        "• Selected polygons can be edited or deleted",
        TutorialTargetType::CANVAS);
    
    // Compute IoU
    tutorial.addStep("Calculate IoU", 
        "To calculate Intersection over Union:\n\n"
        "1. Select two polygons by clicking them\n"
        "2. Click this button\n"
        "3. The IoU value appears at the bottom of the panel",
        TutorialTargetType::COMPUTE_BUTTON);
    
    // Delete
    tutorial.addStep("Delete Polygons", 
        "Select one or more polygons, then click this button to delete them.\n\n"
        "Shortcut: Delete key",
        TutorialTargetType::DELETE_BUTTON);

    // Move
    tutorial.addStep("Move Polygons",
        "Click this button, then select one or more polygons and click to drag.\n\n"
        "Press escape to deselect polygon before pressing this button again to deactivate.",
        TutorialTargetType::MOVE_BUTTON);
    
    // Clear selection
    tutorial.addStep("Clear Selection", 
        "Click here to deselect all polygons.\n\n"
        "Shortcut: Escape key",
        TutorialTargetType::CLEAR_BUTTON);
    
    // Completion
    tutorial.addStep("You're Ready! 🎉", 
        "That's it! You now know the basics.\n\n"
        "Tips:\n"
        "• Hover over buttons for tooltips\n"
        "• Your work auto-saves periodically\n"
        "• Check the Help menu to restart this tutorial\n\n"
        "Happy polygon drawing!",
        TutorialTargetType::NONE);

    // Setup actions
    Actions actions;

    // Clipboard and undo/redo action
    std::vector<Polygon> clipboard;
    ImVec2 undoVertex;
    sf::Vertex undoPolygonOutline;



    while (window.isOpen())
    {   
        
        status.menuInteraction = ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::GetIO().WantCaptureMouse;
        while (const auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto mouseButtonMoved = event->getIf<sf::Event::MouseMoved>()) {
                if (status.draggingPolygons) {
                    ImVec2 delta = { getMousePos(window).x - status.lastMousePos.x, getMousePos(window).y - status.lastMousePos.y };
                    for (int i : selectedPolygons) {
                        polygons.at(i).shift(delta);
                    }
                }
                status.lastMousePos = getMousePos(window);
            }
            else if (const auto mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
                status.draggingPolygons = false;

            }
            else if (const auto mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {

                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    ImVec2 mousepos = getMousePos(window);
      
                    // Handle image resize mode
                    if (imageResizeMode && hasBackgroundImage && imageEnabled) {
                        sf::FloatRect imageBounds = getImageBounds(backgroundSprite);
                        auto handles = getResizeHandles(imageBounds, handleSize);
                        
                        // Check if clicking on a resize handle
                        for (int i = 0; i < handles.size(); i++) {
                            if (handles[i].contains(sf::Vector2f(mousepos.x, mousepos.y))) {
                                isDraggingImage = true;
                                dragHandle = i;
                                dragStartPos = mousepos;
                                imageStartPos = backgroundSprite->getPosition();
                                imageStartSize = imageBounds.size;
                                break;
                            }
                        }
                        
                        // Check if clicking inside image for moving
                        if (!isDraggingImage && imageBounds.contains(sf::Vector2f(mousepos.x, mousepos.y))) {
                            isDraggingImage = true;
                            dragHandle = 8; // Move mode
                            dragStartPos = mousepos;
                            imageStartPos = backgroundSprite->getPosition();
                        }
                        
                        // If we're in resize mode and clicked somewhere, don't create polygons
                        if (isDraggingImage) {
                            continue;
                        }
                    }
                    
                    
                    if (status.createPolygon) {
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
                                if (status.adjustVertices) adjustVertices(vertices);
                                newPolygon.setVertices(vertices);
                                newPolygon.setColour(polygonColour);

                                logger << currentDateTime() << " NEW " << newPolygon;

                                polygons.push_back(newPolygon);

                                vertices.clear();

                                status.createPolygon = false;

                                if (autosaveEnabled) {
                                    sf::Vector2f center = view.getCenter();
                                    ViewState viewState{ zoomLevel, center.x, center.y };
                                    ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
                                    quickSave(polygons, imageState, "autosave.sav", viewState);
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
                        if (!status.menuInteraction and !status.draggingPolygons and !status.draggingVertex) {
                            //select polygon
                            if (selectedPolygons.size() > 0 and !ImGui::IsKeyDown(ImGuiKey_ModShift)) {

                                for (int j : selectedPolygons) {
                                    polygons.at(j).toggleSelected();
                                }
                                selectedPolygons.clear();
                                area = -1;
                            }


                        // Left click to select a polygon. We unfortunately need to check each one.
                        ImVec2 mousepos = getMousePos(window);
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

                            ImVec2 p = getMousePos(window);
                            Polygon* polygon;
                            int i = 0;
                            for (i; i < polygons.size(); i++) {
                                polygon = &polygons.at(i);
                                if (polygon->pointInPolygon(p)) {
                                    if (std::find(selectedPolygons.begin(), selectedPolygons.end(), i) == selectedPolygons.end()) {
                                        selectedPolygons.push_back(i);
                                        polygons.at(i).toggleSelected();
                                    }


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
                            std::cout << "Number of Selected Polygons: " << selectedPolygons.size() << std::endl;
                            status.lastMousePos = mousepos;
                            status.draggingPolygons = true;
                            status.menuInteraction = true;
                        }
                    }
                }
                if (mouseButtonPressed->button == sf::Mouse::Button::Right) {
                    isPanning = true;
                    panStart = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
				}
            }

            
            if (const auto mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mouseButtonReleased->button == sf::Mouse::Button::Left) {
                    isDraggingImage = false;
                    dragHandle = -1;
                }
                if (mouseButtonReleased->button == sf::Mouse::Button::Right) {
                    isPanning = false;
				}
            }
            
            if (const auto mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                if (isDraggingImage && hasBackgroundImage && backgroundSprite) {
                    ImVec2 currentPos = sf::Mouse::getPosition(window);
                    sf::Vector2f delta = sf::Vector2f(currentPos.x - dragStartPos.x, currentPos.y - dragStartPos.y);
                    
                    if (dragHandle == 8) { // Move mode
                        backgroundSprite->setPosition(imageStartPos + delta);
                    } else if (dragHandle >= 0 && dragHandle <= 7) { // Resize handles
                        sf::Vector2f newScale = backgroundSprite->getScale();
                        sf::Vector2f newPos = backgroundSprite->getPosition();
                        
                        // Calculate new scale based on handle
                        sf::Vector2u textureSize = backgroundTexture.getSize();
                        float scaleFactorX = 1.0f;
                        float scaleFactorY = 1.0f;
                        
                        switch (dragHandle) {
                            case 0: // Top-left corner
                                scaleFactorX = (imageStartSize.x - delta.x) / textureSize.x;
                                scaleFactorY = (imageStartSize.y - delta.y) / textureSize.y;
                                newPos = imageStartPos + delta;
                                break;
                            case 1: // Top-right corner
                                scaleFactorX = (imageStartSize.x + delta.x) / textureSize.x;
                                scaleFactorY = (imageStartSize.y - delta.y) / textureSize.y;
                                newPos = sf::Vector2f(imageStartPos.x, imageStartPos.y + delta.y);
                                break;
                            case 2: // Bottom-right corner
                                scaleFactorX = (imageStartSize.x + delta.x) / textureSize.x;
                                scaleFactorY = (imageStartSize.y + delta.y) / textureSize.y;
                                break;
                            case 3: // Bottom-left corner
                                scaleFactorX = (imageStartSize.x - delta.x) / textureSize.x;
                                scaleFactorY = (imageStartSize.y + delta.y) / textureSize.y;
                                newPos = sf::Vector2f(imageStartPos.x + delta.x, imageStartPos.y);
                                break;
                            case 4: // Top edge
                                scaleFactorY = (imageStartSize.y - delta.y) / textureSize.y;
                                newPos = sf::Vector2f(imageStartPos.x, imageStartPos.y + delta.y);
                                scaleFactorX = backgroundSprite->getScale().x;
                                break;
                            case 5: // Right edge
                                scaleFactorX = (imageStartSize.x + delta.x) / textureSize.x;
                                scaleFactorY = backgroundSprite->getScale().y;
                                break;
                            case 6: // Bottom edge
                                scaleFactorY = (imageStartSize.y + delta.y) / textureSize.y;
                                scaleFactorX = backgroundSprite->getScale().x;
                                break;
                            case 7: // Left edge
                                scaleFactorX = (imageStartSize.x - delta.x) / textureSize.x;
                                scaleFactorY = backgroundSprite->getScale().y;
                                newPos = sf::Vector2f(imageStartPos.x + delta.x, imageStartPos.y);
                                break;
                        }
                        
                        // Prevent negative scaling
                        scaleFactorX = std::max(0.1f, scaleFactorX);
                        scaleFactorY = std::max(0.1f, scaleFactorY);
                        
                        backgroundSprite->setScale(sf::Vector2f(scaleFactorX, scaleFactorY));
                        backgroundSprite->setPosition(newPos);
                        
                        // Update the global imageScale to match the sprite's scale
                        imageScale = scaleFactorX; // Assuming uniform scaling
                    }
                }
                if (isPanning) {
                    sf::Vector2f newPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
                    sf::Vector2f delta = panStart - newPos;

                    view.move(delta);
                    window.setView(view);
                }
        }

            // Menu shortcuts
            if (const auto key = event->getIf<sf::Event::KeyPressed>()) {
                // Open file
                if (key->code == sf::Keyboard::Key::O &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl))) {
                    auto [loadedPolygons, statesPair] = actions.OpenFile(polygons, selectedPolygons, view, zoomLevel, window);
                    auto [loadedImageState, loadedViewState] = statesPair;
                    
                    // Restore image state if present
                    if (loadedImageState.hasImage && !loadedImageState.imagePath.empty()) {
                        if (backgroundSprite) {
                            delete backgroundSprite;
                        }
                        backgroundSprite = new sf::Sprite(backgroundTexture);
                        
                        if (backgroundTexture.loadFromFile(loadedImageState.imagePath)) {
                            backgroundSprite->setTexture(backgroundTexture);
                            backgroundSprite->setPosition(sf::Vector2f(loadedImageState.positionX, loadedImageState.positionY));
                            backgroundSprite->setScale(sf::Vector2f(loadedImageState.scaleX, loadedImageState.scaleY));
                            
                            // Set the initial color/opacity
                            sf::Color imageColor = sf::Color::White;
                            imageColor.a = static_cast<std::uint8_t>(loadedImageState.opacity * 255);
                            backgroundSprite->setColor(imageColor);
                            
                            hasBackgroundImage = true;
                            currentImagePath = loadedImageState.imagePath;
                            imageOpacity = loadedImageState.opacity;
                            imageEnabled = loadedImageState.enabled;
                            imageScale = loadedImageState.scaleX;
                            
                            logger << currentDateTime() << " Image restored from " << loadedImageState.imagePath << std::endl;
                        } else {
                            logger << currentDateTime() << " ERROR: Failed to load saved image from " << loadedImageState.imagePath << std::endl;
                            hasBackgroundImage = false;
                            currentImagePath = "";
                        }
                    } else {
                        // Clear any existing image
                        if (backgroundSprite) {
                            delete backgroundSprite;
                            backgroundSprite = nullptr;
                        }
                        hasBackgroundImage = false;
                        currentImagePath = "";
                        imageEnabled = false;
                    }
                }
                // Save file
                if (key->code == sf::Keyboard::Key::S &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl)) &&
                    !(sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift))) {
                    ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
                    actions.SaveFile(polygons, imageState, selectedPolygons, view, zoomLevel, window);
                }
                // Save file as
                if (key->code == sf::Keyboard::Key::S &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl)) &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift))) {
                    ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
                    actions.SaveFileAs(polygons, imageState, selectedPolygons, view, zoomLevel, window);

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
        }
        //After SFML Stuff, before ImGui stuff
        ImGui::SFML::Update(window, deltaClock.restart());
          
        sf::Vector2u size = window.getSize();
        WINDOW_WIDTH = size.x;
        WINDOW_HEIGHT = size.y;
        const float idk = (WINDOW_WIDTH - 280 * SCALE_FACTOR) / 2; //toolazytochange

        // Menu bar
        if (ImGui::BeginMainMenuBar()) {
            mainMenuBarSize = ImGui::GetWindowSize();
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open", "CTRL+O")) {
                    auto [loadedPolygons, statesPair] = actions.OpenFile(polygons, selectedPolygons, view, zoomLevel, window);
                    auto [loadedImageState, loadedViewState] = statesPair;
                    
                    // Restore image state if present
                    if (loadedImageState.hasImage && !loadedImageState.imagePath.empty()) {
                        if (backgroundSprite) {
                            delete backgroundSprite;
                        }
                        backgroundSprite = new sf::Sprite(backgroundTexture);
                        
                        if (backgroundTexture.loadFromFile(loadedImageState.imagePath)) {
                            backgroundSprite->setTexture(backgroundTexture);
                            backgroundSprite->setPosition(sf::Vector2f(loadedImageState.positionX, loadedImageState.positionY));
                            backgroundSprite->setScale(sf::Vector2f(loadedImageState.scaleX, loadedImageState.scaleY));
                            
                            // Set the initial color/opacity
                            sf::Color imageColor = sf::Color::White;
                            imageColor.a = static_cast<std::uint8_t>(loadedImageState.opacity * 255);
                            backgroundSprite->setColor(imageColor);
                            
                            hasBackgroundImage = true;
                            currentImagePath = loadedImageState.imagePath;
                            imageOpacity = loadedImageState.opacity;
                            imageEnabled = loadedImageState.enabled;
                            imageScale = loadedImageState.scaleX;
                            
                            logger << currentDateTime() << " Image restored from " << loadedImageState.imagePath << std::endl;
                        } else {
                            logger << currentDateTime() << " ERROR: Failed to load saved image from " << loadedImageState.imagePath << std::endl;
                            hasBackgroundImage = false;
                            currentImagePath = "";
                        }
                    } else {
                        // Clear any existing image
                        if (backgroundSprite) {
                            delete backgroundSprite;
                            backgroundSprite = nullptr;
                        }
                        hasBackgroundImage = false;
                        currentImagePath = "";
                        imageEnabled = false;
                    }
                }
                if (ImGui::MenuItem("Save", "CTRL+S")) {
                    ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
                    actions.SaveFile(polygons, imageState, selectedPolygons, view, zoomLevel, window);
                }
                if (ImGui::MenuItem("Save As", "CTRL+SHIFT+S")) {
                    ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
                    actions.SaveFileAs(polygons, imageState, selectedPolygons, view, zoomLevel, window);
                }

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
				        ImGui::MenuItem("Adjust Vertices", nullptr, &status.adjustVertices);
                ImGui::MenuItem("Grid", nullptr, &gridEnabled);
                ImGui::SliderFloat("Grid Size", &gridSize, 10.0f, 100.0f);
                ImGui::Separator();
                if (hasBackgroundImage) {
                    ImGui::MenuItem("Show Image", nullptr, &imageEnabled);
                    ImGui::MenuItem("Image Resize Mode", nullptr, &imageResizeMode);
                    createToolTip("Enable interactive resizing with mouse drag handles", tooltipsEnabled);
                    ImGui::SliderFloat("Image Opacity", &imageOpacity, 0.1f, 1.0f);
                    ImGui::SliderFloat("Image Scale", &imageScale, 0.1f, 3.0f);
                }
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

        // Autosaving functionality
        if (autosaveClock.getElapsedTime() >= autosaveTime) {
            ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
            sf::Vector2f center = view.getCenter();
            ViewState viewState{ zoomLevel, center.x, center.y };
            quickSave(polygons, imageState, "autosave.sav", viewState);
            autosaveClock.restart();
        }


        // Window used for creating polygons
        ImGui::SetNextWindowSize(ImVec2(280 * SCALE_FACTOR, 60 * SCALE_FACTOR));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::SetNextWindowPos(ImVec2(idk, mainMenuBarSize.y));

        if (ImGui::Begin("Polygon Creator", nullptr, 7 + ImGuiWindowFlags_NoScrollbar + ImGuiWindowFlags_NoScrollWithMouse)) {

            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            tutorial.updateTargetPosition(TutorialTargetType::POLYGON_CREATOR, windowPos, windowSize);

            

            if (ImGui::IsWindowHovered()) {
                // Mouse is over the popup
                status.menuInteraction = true;
            }
            ImVec2 currentWindowSize = ImGui::GetWindowSize();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // Normal: white
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.9f, 0.9f, 1.0f)); // Hovered: light gray
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f)); // Active: slightly darker gray


            ImGui::SameLine();

            // Capture position BEFORE drawing button
            ImVec2 createButtonPos = ImGui::GetCursorScreenPos();
            if (ImGui::ImageButton("Create Polygon", icons["draw"].texture, ImVec2(ICON_SIZE, ICON_SIZE))) {
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
                    ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
                    quickSave(polygons, imageState, "autosave.sav", viewState);
                }

                logger << currentDateTime() << " Began polygon creation.\n";
            }

            tutorial.updateTargetPosition(TutorialTargetType::CREATE_BUTTON, createButtonPos, ImVec2(120, 30));
          
            createToolTip("Click on canvas to create vertices", tooltipsEnabled);
            ImGui::SameLine();
			
            if (ImGui::ImageButton("Select Shape", icons["shapes"].texture, ImVec2(ICON_SIZE, ICON_SIZE))) {
                // Select Shape
                
                ImGui::OpenPopup("ShapeSelector");
            }
            createToolTip("Click to create polygon", tooltipsEnabled);
            ImGui::SameLine();
            
            ImVec2 deleteButtonPos = ImGui::GetCursorScreenPos();
          
            if (ImGui::ImageButton("Delete Polygon", icons["bin"].texture, ImVec2(ICON_SIZE, ICON_SIZE))) {
                // Delete Polygon
                for (int i : selectedPolygons) {
                    polygons.erase(polygons.begin() + i);
                    logger << currentDateTime() << ": Polygon deleted at " << i << "\n";

                }
                selectedPolygons.clear();
                
            }

            ImGui::SameLine();
            if (ImGui::ImageButton("Colour", icons["colour-pallet"].texture, ImVec2(ICON_SIZE, ICON_SIZE))) {
                // Change Colour
				ImGui::OpenPopup("ColourPicker");
			}
			createToolTip("Change selected polygon colour", tooltipsEnabled);
           
            

            ImGui::SameLine();
            ImGui::BeginDisabled(selectedPolygons.size() != 2);
            if (selectedPolygons.size() != 2) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.6f, 1.0f)); // Disabled: gray
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // Normal: white
            }
            if (ImGui::ImageButton("Compute IoU", icons["intersect"].texture, ImVec2(ICON_SIZE, ICON_SIZE)) && selectedPolygons.size() == 2) {
                // Save when computing IoU
                if (autosaveEnabled) {
                    ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
                    sf::Vector2f center = view.getCenter();
                    ViewState viewState{ zoomLevel, center.x, center.y };
                    quickSave(polygons, imageState, "autosave.sav", viewState);
                }


                Polygon intersection = polygons.at(selectedPolygons.at(0));
                for (int i = 1; i < selectedPolygons.size(); i++) {
                    intersection = intersectingPolygon(&intersection, &polygons.at(selectedPolygons.at(i)));
                }

                polygons.push_back(intersection);

                // TODO: Calculate IoU Metric and display result.
                IoUArea = intersection.polygonArea();
                area = polygons.at(selectedPolygons.at(0)).polygonArea() + polygons.at(selectedPolygons.at(1)).polygonArea() - IoUArea;
                IoUMetric = IoUArea / area;

                if (autosaveEnabled) {
                    sf::Vector2f center = view.getCenter();
                    ViewState viewState{ zoomLevel, center.x, center.y };
                    ImageState imageState = getCurrentImageState(hasBackgroundImage, currentImagePath, backgroundSprite, imageOpacity, imageEnabled);
                    quickSave(polygons, imageState, "autosave.sav", viewState);
                }

                logger << currentDateTime << " Computed IoU.\n";
            }
            createToolTip("Select two polygons and then calculate", tooltipsEnabled);
            ImGui::PopStyleColor();
            ImGui::Text("Area:");
            ImGui::SameLine(); ImGui::Text("%s", area == -1 ? "" : std::to_string(area).c_str());
            ImGui::Text("IoU Area:");
            ImGui::SameLine(); ImGui::Text("%s", IoUArea == -1 ? "" : std::to_string(IoUArea).c_str());
            ImGui::Text("IoU metric:");
            ImGui::SameLine(); ImGui::Text("%s", IoUMetric == -1 ? "" : std::to_string(IoUMetric).c_str());
            


        }
        tutorial.updateTargetPosition(TutorialTargetType::COMPUTE_BUTTON, computeButtonPos, ImVec2(120, 30));
        ImGui::PopStyleColor(4);
        ImGui::EndDisabled();


        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f); // Set to your desired border thickness
        ImGui::SetNextWindowSize(ImVec2(ICON_SIZE*2, 400*SCALE_FACTOR), ImGuiCond_Appearing);
        if (ImGui::BeginPopup("ShapeSelector", ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // Normal: white
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.9f, 0.9f, 1.0f)); // Hovered: light gray
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f)); // Active: slightly darker gray
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0, 0, 0, 1));


            if (ImGui::ImageButton("Triangle", icons["triangle"].texture, ImVec2(ICON_SIZE, ICON_SIZE))) {
                newPolygon = Polygon();
                vertices = { ImVec2(100,100), ImVec2(150,200), ImVec2(50,200) };
                if (status.adjustVertices)
                    adjustVertices(vertices);
                newPolygon.setVertices(vertices);
                newPolygon.setColour(polygonColour);
                polygons.push_back(newPolygon);
                logger << currentDateTime() << " NEW " << newPolygon;
                newPolygon = Polygon();
                vertices.clear();
                ImGui::CloseCurrentPopup();
            }
            createToolTip("Create triangle", tooltipsEnabled);
            ImGui::SameLine();
            if (ImGui::ImageButton("Circle", icons["circle"].texture, ImVec2(ICON_SIZE, ICON_SIZE))) {

                ImGui::CloseCurrentPopup();
            }
            createToolTip("Create a circle by clicking the centre and moving outwards to define radius", tooltipsEnabled);

            if (ImGui::ImageButton("Rectangle", icons["rectangle"].texture, ImVec2(ICON_SIZE, ICON_SIZE))) {

                ImGui::CloseCurrentPopup();
            }
            createToolTip("Create rectangle by clicking top left corner and dragging the bottom right corner out", tooltipsEnabled);
            ImGui::SameLine();
            if (ImGui::ImageButton("Square", icons["square"].texture, ImVec2(ICON_SIZE, ICON_SIZE))) {

                ImGui::CloseCurrentPopup();
            }
            createToolTip("Create square by clicking top left corner and dragging the bottom right corner out", tooltipsEnabled);

            ImGui::PopStyleColor(4);
            ImGui::EndPopup();
        }
        
        if (ImGui::BeginPopup("ColourPicker",ImGuiWindowFlags_AlwaysAutoResize + ImGuiWindowFlags_NoScrollbar + ImGuiWindowFlags_NoTitleBar)) {
            
            // Use dark button colors for contrast on white background
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.25f, 0.25f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.10f, 0.10f, 0.15f, 1.0f));

            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.15f, 0.18f, 1.0f)); // Normal: dark gray
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.30f, 1.0f)); // Hovered: lighter gray
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.10f, 0.10f, 0.15f, 1.0f)); // Active: even darker


            // Capture position BEFORE drawing color picker
            ImVec2 colorPickerPos = ImGui::GetCursorScreenPos();
            if (ImGui::ColorPicker3("Select Colour", polygonColour)) {
                //Alter Polygon Colour
                if (!selectedPolygons.empty()) {
                    for (int i : selectedPolygons) {
                        polygons.at(i).setColour(polygonColour);
                    }
                }
                
            }
            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(6);

            ImGui::EndPopup();
        }
        ImVec2 colorPickerSize = ImGui::GetItemRectSize();
        tutorial.updateTargetPosition(TutorialTargetType::COLOR_PICKER, colorPickerPos, colorPickerSize);
          
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        
        ImGui::End();

        // Update canvas target (main window minus UI)
        ImVec2 canvasPos = ImVec2(350, 20);  // After polygon creator window
        ImVec2 canvasSize = ImVec2(window.getSize().x - 350, window.getSize().y - 20);
        tutorial.updateTargetPosition(TutorialTargetType::CANVAS, canvasPos, canvasSize);

        // Image Resize Dialog
        if (showImageResizeDialog) {
            ImGui::SetNextWindowSize(ImVec2(400, 300));
            ImGui::SetNextWindowPos(ImVec2(window.getSize().x / 2 - 200, window.getSize().y / 2 - 150));
            if (ImGui::Begin("Import Image - Resize Options", &showImageResizeDialog, ImGuiWindowFlags_NoResize)) {
                
                ImGui::Text("Original Size: %dx%d", originalImageSize.x, originalImageSize.y);
                ImGui::Text("Window Size: %dx%d", window.getSize().x, window.getSize().y);
                ImGui::Separator();
                
                ImGui::Text("Choose resize option:");
                ImGui::RadioButton("Fit to Window (maintain aspect ratio)", &resizeMode, 0);
                createToolTip("Scale image to fit window while keeping proportions", tooltipsEnabled);
                
                ImGui::RadioButton("Original Size (no scaling)", &resizeMode, 1);
                createToolTip("Keep image at its original pixel dimensions", tooltipsEnabled);
                
                ImGui::RadioButton("Custom Size", &resizeMode, 2);
                createToolTip("Specify exact width and height for the image", tooltipsEnabled);
                
                if (resizeMode == 2) {
                    ImGui::Indent();
                    ImGui::SliderFloat("Width", &customWidth, 50.0f, static_cast<float>(window.getSize().x * 2));
                    ImGui::SliderFloat("Height", &customHeight, 50.0f, static_cast<float>(window.getSize().y * 2));
                    
                    if (ImGui::Button("Reset to Original")) {
                        customWidth = static_cast<float>(originalImageSize.x);
                        customHeight = static_cast<float>(originalImageSize.y);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Fit to Window")) {
                        float scaleX = static_cast<float>(window.getSize().x) / originalImageSize.x;
                        float scaleY = static_cast<float>(window.getSize().y) / originalImageSize.y;
                        float scale = std::min(scaleX, scaleY);
                        customWidth = originalImageSize.x * scale;
                        customHeight = originalImageSize.y * scale;
                    }
                    ImGui::Unindent();
                }
                
                ImGui::Separator();
                
                if (ImGui::Button("Import", ImVec2(120, 30))) {
                    if (backgroundSprite) {
                        delete backgroundSprite;
                    }
                    backgroundSprite = new sf::Sprite(backgroundTexture);
                    hasBackgroundImage = loadBackgroundImage(backgroundTexture, *backgroundSprite, pendingImagePath, window.getSize(), resizeMode, customWidth, customHeight);
                    if (hasBackgroundImage) {
                        // Set the initial color/opacity
                        sf::Color imageColor = sf::Color::White;
                        imageColor.a = static_cast<std::uint8_t>(imageOpacity * 255);
                        backgroundSprite->setColor(imageColor);
                        
                        // Update global scale to match the loaded image's scale
                        imageScale = backgroundSprite->getScale().x;
                        
                        imageEnabled = true;
                        currentImagePath = pendingImagePath;
                    }
                    showImageResizeDialog = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 30))) {
                    showImageResizeDialog = false;
                }
            }
            ImGui::End();
        }

        window.clear(sf::Color({ 243,243,243,1 }));
        
        // Draw background image
        if (hasBackgroundImage && imageEnabled) {
            // Update sprite properties based on settings
            backgroundSprite->setScale(sf::Vector2f(imageScale, imageScale));
            sf::Color imageColor = backgroundSprite->getColor();
            imageColor.a = static_cast<std::uint8_t>(imageOpacity * 255);
            backgroundSprite->setColor(imageColor);
            window.draw(*backgroundSprite);
        }
        
        // Draw grid background
        if (gridEnabled) {
            drawGrid(window, gridSize);
        }

        // Draw everything here

        for (Polygon polygon : polygons) {
            window.draw(polygon.render);
        }
        
        // Draw resize handles if in resize mode
        if (imageResizeMode && hasBackgroundImage && imageEnabled && backgroundSprite) {
            sf::FloatRect imageBounds = getImageBounds(backgroundSprite);
            drawResizeHandles(window, imageBounds, handleSize);
        }

        if (status.createTemplatePolygon) {

        }

        if (status.createPolygon && !firstVertex) {
            // Draw boundary of supposed polygon
            ImVec2 mousepos = getMousePos(window);
            newPolygonOutline.back().position = mousepos;
            window.draw(newPolygonOutline.data(), newPolygonOutline.size(), sf::PrimitiveType::LineStrip);
        }

        if (tutorial.isActive())
            tutorial.render();


        ImGui::SFML::Render(window);
        window.display();

    }

    // Cleanup
    if (backgroundSprite) {
        delete backgroundSprite;
    }

    ImGui::SFML::Shutdown();

    // Save logs to file
    if (logSavingEnabled) {
        saveLogToFile("log");
    }
    glfwTerminate();
    return 0;
}


int main()
{   
    try {
        return runProgram();
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
