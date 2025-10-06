#include "tutorial.h"
#include <imgui.h>
#include <cmath>

Tutorial::Tutorial() : currentStep(0), active(false), showSkipConfirm(false) {}

void Tutorial::addStep(const std::string& title, const std::string& text, TutorialTargetType targetType) {
    TutorialStep step;
    step.title = title;
    step.text = text;
    step.targetType = targetType;
    step.targetPosition = ImVec2(0, 0);
    step.targetSize = ImVec2(0, 0);
    step.autoPosition = true;
    steps.push_back(step);
}

void Tutorial::start() {
    active = true;
    currentStep = 0;
    showSkipConfirm = false;
}

void Tutorial::reset() {
    currentStep = 0;
    active = false;
    showSkipConfirm = false;
}

void Tutorial::nextStep() {
    if (currentStep < static_cast<int>(steps.size()) - 1)
        currentStep++;
    else
        active = false;  // Finished tutorial
}

void Tutorial::previousStep() {
    if (currentStep > 0)
        currentStep--;
}

void Tutorial::skip() {
    showSkipConfirm = true;
}

bool Tutorial::isActive() const {
    return active;
}

int Tutorial::getCurrentStep() const {
    return currentStep;
}

int Tutorial::getTotalSteps() const {
    return static_cast<int>(steps.size());
}

void Tutorial::updateTargetPosition(TutorialTargetType type, ImVec2 pos, ImVec2 size) {
    for (auto& step : steps) {
        if (step.targetType == type) {
            step.targetPosition = pos;
            step.targetSize = size;
        }
    }
}

ImVec2 Tutorial::calculateWindowPosition(const TutorialStep& step) {
    if (!step.autoPosition) {
        return step.windowPosition;
    }

    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    ImVec2 windowSize(380, 200);  // Approximate tutorial window size

    // Position window to avoid overlapping with target
    ImVec2 pos;
    
    if (step.targetType == TutorialTargetType::NONE) {
        // Center of screen
        pos.x = (screenSize.x - windowSize.x) * 0.5f;
        pos.y = (screenSize.y - windowSize.y) * 0.5f;
    } else {
        // Position relative to target
        float padding = 30.0f;
        
        // Try to position to the right
        if (step.targetPosition.x + step.targetSize.x + padding + windowSize.x < screenSize.x) {
            pos.x = step.targetPosition.x + step.targetSize.x + padding;
            pos.y = step.targetPosition.y;
        }
        // Otherwise position to the left
        else if (step.targetPosition.x - padding - windowSize.x > 0) {
            pos.x = step.targetPosition.x - padding - windowSize.x;
            pos.y = step.targetPosition.y;
        }
        // Otherwise position below
        else if (step.targetPosition.y + step.targetSize.y + padding + windowSize.y < screenSize.y) {
            pos.x = step.targetPosition.x;
            pos.y = step.targetPosition.y + step.targetSize.y + padding;
        }
        // Otherwise position above
        else {
            pos.x = step.targetPosition.x;
            pos.y = step.targetPosition.y - padding - windowSize.y;
        }
        
        // Clamp to screen bounds
        pos.x = std::max(10.0f, std::min(pos.x, screenSize.x - windowSize.x - 10.0f));
        pos.y = std::max(30.0f, std::min(pos.y, screenSize.y - windowSize.y - 10.0f));
    }
    
    return pos;
}

void Tutorial::renderHighlight(const TutorialStep& step) {
    if (step.targetType == TutorialTargetType::NONE) {
        return;
    }

    // Skip if target hasn't been positioned yet
    if (step.targetSize.x <= 0 || step.targetSize.y <= 0) {
        return;
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    // Pulsing animation
    float time = ImGui::GetTime();
    float pulse = 0.5f + 0.5f * sinf(time * 2.5f);
    
    ImVec2 topLeft = step.targetPosition;
    ImVec2 bottomRight = ImVec2(
        step.targetPosition.x + step.targetSize.x,
        step.targetPosition.y + step.targetSize.y
    );
    
    // Add padding to the highlight
    float padding = 4.0f;
    topLeft.x -= padding;
    topLeft.y -= padding;
    bottomRight.x += padding;
    bottomRight.y += padding;
    
    // Very subtle background tint
    ImU32 highlightColor = IM_COL32(100, 150, 255, static_cast<int>(15 + 10 * pulse));
    drawList->AddRectFilled(topLeft, bottomRight, highlightColor, 3.0f);
    
    // Main border - bright and pulsing
    float thickness = 2.0f + pulse * 0.5f;
    ImU32 borderColor = IM_COL32(50, 150, 255, 255);
    drawList->AddRect(topLeft, bottomRight, borderColor, 3.0f, 0, thickness);
    
    // Outer glow layer
    ImU32 glowColor = IM_COL32(50, 150, 255, static_cast<int>(80 + 80 * pulse));
    drawList->AddRect(
        ImVec2(topLeft.x - 2, topLeft.y - 2), 
        ImVec2(bottomRight.x + 2, bottomRight.y + 2), 
        glowColor, 3.0f, 0, 1.5f
    );
}

bool Tutorial::render() {
    if (!active || steps.empty())
        return false;

    const TutorialStep& step = steps[currentStep];

    // Render highlight if there's a target
    if (step.targetType != TutorialTargetType::NONE) {
        renderHighlight(step);
    }

    // Position and render tutorial window
    ImVec2 windowPos = calculateWindowPosition(step);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(380, 0), ImGuiCond_Always);
    
    // Custom window styling - fully opaque white background
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // Fully opaque white
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));      // Dark text
    
    bool windowOpen = true;
    ImGui::Begin(step.title.c_str(), &windowOpen, 
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    // Progress indicator - medium gray
    ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), 
                      "Step %d of %d", currentStep + 1, static_cast<int>(steps.size()));
    
    ImGui::Separator();
    ImGui::Spacing();

    // Tutorial content - dark text for readability
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));  // Very dark text
    ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - 20);
    ImGui::TextWrapped("%s", step.text.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Navigation buttons
    float buttonWidth = 100.0f;
    
    // Previous button
    if (currentStep > 0) {
        if (ImGui::Button("< Previous", ImVec2(buttonWidth, 30))) {
            previousStep();
        }
        ImGui::SameLine();
    }
    
    // Next/Finish button
    if (currentStep < static_cast<int>(steps.size()) - 1) {
        if (ImGui::Button("Next >", ImVec2(buttonWidth, 30))) {
            nextStep();
        }
    } else {
        if (ImGui::Button("Finish", ImVec2(buttonWidth, 30))) {
            active = false;
        }
    }
    
    // Skip button
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - buttonWidth - 10);
    if (ImGui::Button("Skip Tutorial", ImVec2(buttonWidth, 30))) {
        skip();
    }

    ImGui::End();
    
    ImGui::PopStyleColor(4);  // Pop all 4 style colors

    // Skip confirmation dialog
    if (showSkipConfirm) {
        ImGui::OpenPopup("Skip Tutorial?");
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal("Skip Tutorial?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to skip the tutorial?");
            ImGui::Text("You can restart it anytime from the Help menu.");
            ImGui::Spacing();
            
            if (ImGui::Button("Yes, Skip", ImVec2(120, 0))) {
                active = false;
                showSkipConfirm = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No, Continue", ImVec2(120, 0))) {
                showSkipConfirm = false;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }

    // If user closed window via X button
    if (!windowOpen) {
        skip();
    }

    return active;
}