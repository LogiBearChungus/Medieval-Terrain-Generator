#include <ew/external/opengl/include/glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "world/world.h"
#include "world/tile.h"
#include "noise/PerlinNoise.h"
#include <glm/glm.hpp>
#include <cstdlib>
#include <ctime>
#include <cmath>

// Map size
const int MAP_WIDTH = 256;
const int MAP_HEIGHT = 256;

// Error callback for GLFW
void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error: " << description << std::endl;
}

// Clamp function for C++11/14 compatibility
float clamp(float x, float min, float max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

// Smooth interpolation function
float smoothstep(float edge0, float edge1, float x) {
    x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

// ---------------- BIOME DECISION ----------------

Biome determineBiome(float height, float moisture, float temperature) {
    // Bigger oceans - raised threshold
    if (height < 0.42f)
        return Biome::Ocean;

    // Narrow beach zone
    if (height < 0.47f)
        return Biome::Beach;

    // More mountains - lowered threshold
    if (height > 0.68f) {
        // Snow caps on very tall mountains in cold areas
        if (height > 0.78f && temperature < 0.4f)
            return Biome::Tundra;
        return Biome::Mountain;
    }

    // Cold regions (polar/high latitude)
    if (temperature < 0.25f) {
        if (moisture > 0.4f)
            return Biome::Tundra;
        return Biome::Tundra; // Cold deserts still look tundra-ish
    }

    // Temperate cold
    if (temperature < 0.45f) {
        if (moisture > 0.55f)
            return Biome::Forest; // Boreal/Taiga forest
        return Biome::Plains;
    }

    // Temperate
    if (temperature < 0.65f) {
        if (moisture > 0.6f)
            return Biome::Forest; // Temperate forest
        if (moisture > 0.35f)
            return Biome::Plains; // Grasslands
        return Biome::Plains; // Dry plains
    }

    // Hot regions
    if (moisture < 0.25f)
        return Biome::Desert; // Hot desert

    if (moisture < 0.45f)
        return Biome::Plains; // Savanna/dry grassland

    return Biome::Forest; // Tropical/subtropical forest
}

// ---------------- BIOME COLORS ----------------

void biomeToColor(Biome biome, unsigned char& r, unsigned char& g, unsigned char& b) {
    switch (biome) {
    case Biome::Ocean:    r = 25;  g = 60;  b = 140; break;  // Deeper blue
    case Biome::Beach:    r = 220; g = 205; b = 150; break;  // Sandy
    case Biome::Plains:   r = 100; g = 165; b = 80;  break;  // Grassland green
    case Biome::Forest:   r = 30;  g = 105; b = 50;  break;  // Deep forest green
    case Biome::Desert:   r = 210; g = 180; b = 100; break;  // Sandy brown
    case Biome::Tundra:   r = 210; g = 225; b = 230; break;  // Icy white-blue
    case Biome::Mountain: r = 110; g = 100; b = 90;  break;  // Rocky gray-brown
    }
}

int main() {
    std::srand(std::time(0));
    World world(MAP_WIDTH, MAP_HEIGHT);

    PerlinNoise heightNoise(rand());
    PerlinNoise moistureNoise(rand());
    PerlinNoise temperatureNoise(rand());

    // ----------- GENERATE NOISE MAPS -----------

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            float nx = (float)x / MAP_WIDTH;
            float ny = (float)y / MAP_HEIGHT;

            // Height: Multiple octaves for natural looking terrain
            // Large scale landmass shape
            float continents = heightNoise.fractalNoise(nx * 2.2f, ny * 2.2f, 3, 2.0f, 0.5f);
            // Medium scale features (hills, valleys)
            float mediumDetail = heightNoise.fractalNoise(nx * 5.0f, ny * 5.0f, 4, 2.0f, 0.5f);
            // Fine detail
            float fineDetail = heightNoise.fractalNoise(nx * 12.0f, ny * 12.0f, 3, 2.0f, 0.4f);

            // Blend the scales with appropriate weights
            float height = continents * 0.55f + mediumDetail * 0.3f + fineDetail * 0.15f;

            // Apply island mask for single continent with natural coastlines
            float centerX = nx - 0.5f;
            float centerY = ny - 0.5f;
            float distFromCenter = std::sqrt(centerX * centerX + centerY * centerY);
            float islandMask = 1.0f - smoothstep(0.25f, 0.48f, distFromCenter);
            height = height * (0.3f + 0.7f * islandMask); // Stronger island effect for single continent

            height = (height + 1.0f) / 2.0f;
            height = clamp(height, 0.0f, 1.0f);

            // Moisture: affected by distance from water
            float baseMoisture = moistureNoise.fractalNoise(nx * 3.5f, ny * 3.5f, 4, 2.1f, 0.5f);
            baseMoisture = (baseMoisture + 1.0f) / 2.0f;

            // Increase moisture near water
            if (height < 0.45f) {
                baseMoisture = std::min(1.0f, baseMoisture + 0.3f);
            }

            float moisture = clamp(baseMoisture, 0.0f, 1.0f);

            // Temperature: More localized variation for continent-scale
            // No strong latitude gradient - just regional variation
            float tempNoise = temperatureNoise.fractalNoise(nx * 2.8f, ny * 2.8f, 4, 2.0f, 0.5f);
            tempNoise = (tempNoise + 1.0f) / 2.0f;

            // Temperature decreases with elevation (mountains are cooler)
            float elevationCooling = smoothstep(0.5f, 0.85f, height) * 0.35f;

            // Combine: mostly noise-driven with elevation effect
            float temperature = tempNoise * 0.85f + 0.15f - elevationCooling;
            temperature = clamp(temperature, 0.0f, 1.0f);

            Tile& t = world.at(x, y);
            t.height = height;
            t.moisture = moisture;
            t.temperature = temperature;
            t.biome = determineBiome(height, moisture, temperature);
        }
    }

    // ----------- BUILD PIXEL BUFFER -----------

    std::vector<unsigned char> pixels(MAP_WIDTH * MAP_HEIGHT * 3);

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            unsigned char r, g, b;
            biomeToColor(world.at(x, y).biome, r, g, b);

            // Add subtle height-based shading for more depth
            float heightShade = world.at(x, y).height;
            float shadeFactor = 0.7f + 0.3f * heightShade;

            int index = (y * MAP_WIDTH + x) * 3;
            pixels[index + 0] = (unsigned char)(r * shadeFactor);
            pixels[index + 1] = (unsigned char)(g * shadeFactor);
            pixels[index + 2] = (unsigned char)(b * shadeFactor);
        }
    }

    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Request OpenGL 3.3 core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "DND Map Generator", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glViewport(0, 0, 800, 800);

    // Create OpenGL texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MAP_WIDTH, MAP_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Simple quad for fullscreen texture
    float vertices[] = {
        // positions   // texcoords
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texcoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Simple shaders
    const char* vertexShaderSource = R"glsl(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTex;
        out vec2 TexCoord;
        void main() {
            TexCoord = aTex;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )glsl";

    const char* fragmentShaderSource = R"glsl(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D mapTexture;
        void main() {
            FragColor = texture(mapTexture, TexCoord);
        }
    )glsl";

    auto compileShader = [](GLenum type, const char* source) -> GLuint {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info[512];
            glGetShaderInfoLog(shader, 512, nullptr, info);
            std::cerr << "Shader compile error: " << info << std::endl;
        }
        return shader;
        };

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "mapTexture"), 0);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &texture);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}