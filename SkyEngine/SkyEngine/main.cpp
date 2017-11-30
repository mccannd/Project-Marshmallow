#pragma once
#include "VulkanApplication.h"
#include "ImageUtils.h"

int main() {
    VulkanApplication app = VulkanApplication();



    // remove this pls
    try {
        app.run();
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}