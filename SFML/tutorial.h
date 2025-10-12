#pragma once
#include <string>
#include <vector>
#include <imgui.h>

enum class TutorialTargetType {
    NONE,              // No specific target
    MENU_BAR,          // Top menu bar
    FILE_MENU,         // File menu
    EDIT_MENU,         // Edit menu
    VIEW_MENU,         // View menu
    HELP_MENU,         // Help menu
    CREATE_BUTTON,     // Create Polygon button
    SHAPE_BUTTON,       // Use pre-defined shape
    DELETE_BUTTON,     // Delete Polygon button
    MOVE_BUTTON,       // Move Polygon button
    COMPUTE_BUTTON,    // Compute IoU button
    CLEAR_BUTTON,      // Clear Selected button
    COLOR_PICKER,      // Color picker area
    CANVAS,            // Main canvas area
    POLYGON_CREATOR    // Entire Polygon Creator window
};

struct TutorialStep {
    std::string title;
    std::string text;
    TutorialTargetType targetType;
    ImVec2 targetPosition;     // Position to point at
    ImVec2 targetSize;         // Size of target area
    ImVec2 windowPosition;     // Where to position the tutorial window
    bool autoPosition;         // Auto-position based on target
};

class Tutorial {
public:
    Tutorial();

    void addStep(const std::string& title, const std::string& text, 
                 TutorialTargetType targetType = TutorialTargetType::NONE);
    
    void nextStep();
    void previousStep();
    void skip();               // Skip the entire tutorial

    // Render the tutorial window; returns true if the tutorial is still active
    bool render();

    void start();              // start the tutorial
    void reset();              // reset to first step
    bool isActive() const;
    int getCurrentStep() const;
    int getTotalSteps() const;

    // Update target positions (call this each frame to track UI elements)
    void updateTargetPosition(TutorialTargetType type, ImVec2 pos, ImVec2 size);

private:
    std::vector<TutorialStep> steps;
    int currentStep;
    bool active;
    bool showSkipConfirm;      // Show skip confirmation dialog

    void renderHighlight(const TutorialStep& step);
    ImVec2 calculateWindowPosition(const TutorialStep& step);
};