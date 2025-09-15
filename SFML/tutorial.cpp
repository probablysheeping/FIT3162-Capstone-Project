#include "tutorial.h"
#include <imgui.h>

Tutorial::Tutorial() : currentStep(0), active(false) {}

void Tutorial::addStep(const std::string& title, const std::string& text) {
    steps.push_back({ title, text });
}

void Tutorial::start() {
    active = true;
    currentStep = 0;
}

void Tutorial::reset() {
    currentStep = 0;
    active = false;
}

void Tutorial::nextStep() {
    if (currentStep < static_cast<int>(steps.size()) - 1)
        currentStep++;
}

void Tutorial::previousStep() {
    if (currentStep > 0)
        currentStep--;
}

bool Tutorial::isActive() const {
    return active;
}

bool Tutorial::render() {
    if (!active || steps.empty())
        return false;

    const TutorialStep& step = steps[currentStep];

    ImGui::Begin(step.title.c_str(), &active);

    ImGui::TextWrapped("%s", step.text.c_str());

    if (currentStep < static_cast<int>(steps.size()) - 1) {
        if (ImGui::Button("Next")) nextStep();
    }
    else {
        // Last step: show Finish button
        if (ImGui::Button("Finish")) active = false;
    }

    if (currentStep > 0) {
        ImGui::SameLine();
        if (ImGui::Button("Back")) previousStep();
    }

    ImGui::End();

    return active; // return true if still active
}