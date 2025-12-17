#include <ew/external/opengl/include/glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "world/world.h"
#include "world/tile.h"
#include "noise/PerlinNoise.h"
#include <glm/glm.hpp>
#include <map>

// Map size
const int MAP_WIDTH = 256;
const int MAP_HEIGHT = 256;

// Error callback for GLFW
void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error: " << description << std::endl;
}

// ---------------- BIOME DECISION ----------------

Biome determineBiome(float height, float moisture, float temperature) {
    // Water first
    if (height < 0.38f)
        return Biome::Ocean;

    if (height < 0.45f)
        return Biome::Beach;

    // Mountains override climate
    if (height > 0.78f)
        return Biome::Mountain;

    // Cold regions
    if (temperature < 0.3f) {
        if (moisture > 0.5f)
            return Biome::Tundra;
        return Biome::Plains;
    }

    // Hot regions
    if (temperature > 0.7f) {
        if (moisture < 0.3f)
            return Biome::Desert;
        return Biome::Plains;
    }

    // Forest vs plains
    if (moisture > 0.5f)
        return Biome::Forest;

    return Biome::Plains;
}

// ---------------- BIOME COLORS ----------------

void biomeToColor(Biome biome, unsigned char& r, unsigned char& g, unsigned char& b) {
    switch (biome) {
    case Biome::Ocean:    r = 0;   g = 0;   b = 180; break;
    case Biome::Beach:    r = 210; g = 200; b = 140; break;
    case Biome::Plains:   r = 80;  g = 180; b = 80;  break;
    case Biome::Forest:   r = 20;  g = 120; b = 40;  break;
    case Biome::Desert:   r = 230; g = 210; b = 120; break;
    case Biome::Tundra:   r = 200; g = 220; b = 220; break;
    case Biome::Mountain: r = 120; g = 120; b = 120; break;
    }
}


int main() {
    World world(MAP_WIDTH, MAP_HEIGHT);

    PerlinNoise heightNoise(1337);
    PerlinNoise moistureNoise(4242);
    PerlinNoise temperatureNoise(9001);

    // ----------- GENERATE NOISE MAPS -----------

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            float nx = (float)x / MAP_WIDTH;
            float ny = (float)y / MAP_HEIGHT;

            // Height: continents + detail
            float baseHeight =
                heightNoise.fractalNoise(nx * 2.0f, ny * 2.0f, 4, 2.0f, 0.5f);
            float detailHeight =
                heightNoise.fractalNoise(nx * 8.0f, ny * 8.0f, 4, 2.0f, 0.3f);

            float height = baseHeight + 0.3f * detailHeight;
            height = (height + 1.0f) / 2.0f;

            // Moisture
            float moisture =
                moistureNoise.fractalNoise(nx * 4.0f, ny * 4.0f, 4, 2.0f, 0.5f);
            moisture = (moisture + 1.0f) / 2.0f;

            // Temperature (latitudinal gradient + noise)
            float latitude = 1.0f - std::abs(ny - 0.5f) * 2.0f;
            float tempNoise =
                temperatureNoise.fractalNoise(nx * 3.0f, ny * 3.0f, 4, 2.0f, 0.4f);

            float temperature = latitude * 0.7f + ((tempNoise + 1.0f) / 2.0f) * 0.3f;

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

            int index = (y * MAP_WIDTH + x) * 3;
            pixels[index + 0] = r;
            pixels[index + 1] = g;
            pixels[index + 2] = b;
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