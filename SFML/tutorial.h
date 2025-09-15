#pragma once
#include <string>
#include <vector>

struct TutorialStep {
    std::string title;
    std::string text;
};

class Tutorial {
public:
    Tutorial();

    void addStep(const std::string& title, const std::string& text);
    void nextStep();
    void previousStep();

    // Render the tutorial window; returns true if the tutorial is still active
    bool render();

    void start();      // start the tutorial
    void reset();      // reset to first step
    bool isActive() const;

private:
    std::vector<TutorialStep> steps;
    int currentStep;
    bool active;
};