#include <ew/external/opengl/include/glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "world/world.h"
#include "world/tile.h"
#include "noise/PerlinNoise.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>

// Map size
const int MAP_WIDTH = 256;
const int MAP_HEIGHT = 256;

// Camera state
float cameraDistance = 300.0f;
float cameraAngleX = 45.0f;
float cameraAngleY = 45.0f;
double lastMouseX = 400.0;
double lastMouseY = 400.0;
bool firstMouse = true;
bool mousePressed = false;


void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error: " << description << std::endl;
}

float clamp(float x, float min, float max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

float smoothstep(float edge0, float edge1, float x) {
    x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

BiomeType determineBiomeType(float height, float moisture, float temperature) {
    if (height < 0.30f) return BiomeType::DeepOcean;
    if (height < 0.38f) return BiomeType::Ocean;
    if (height < 0.42f) return BiomeType::Beach;

    if (height > 0.82f) {
        if (temperature < 0.35f) return BiomeType::SnowPeak;
        return BiomeType::Mountain;
    }
    if (height > 0.72f) {
        if (temperature < 0.3f) return BiomeType::SnowPeak;
        return BiomeType::Mountain;
    }

    if (temperature < 0.25f) {
        if (moisture > 0.4f) return BiomeType::Taiga;
        return BiomeType::Tundra;
    }
    if (temperature < 0.40f) {
        if (moisture > 0.6f) return BiomeType::Taiga;
        if (moisture > 0.35f) return BiomeType::Grassland;
        return BiomeType::Scrubland;
    }
    if (temperature < 0.55f) {
        if (moisture > 0.7f) return BiomeType::DenseForest;
        if (moisture > 0.5f) return BiomeType::Forest;
        if (moisture > 0.3f) return BiomeType::Grassland;
        return BiomeType::Scrubland;
    }
    if (temperature < 0.70f) {
        if (moisture > 0.65f) return BiomeType::Forest;
        if (moisture > 0.45f) return BiomeType::Grassland;
        if (moisture > 0.25f) return BiomeType::Savanna;
        return BiomeType::Desert;
    }

    if (moisture < 0.20f) return BiomeType::Desert;
    if (moisture < 0.40f) return BiomeType::Savanna;
    if (moisture > 0.75f) return BiomeType::Swamp;
    return BiomeType::Forest;
}

void biomeToColor(BiomeType biome, float& r, float& g, float& b) {
    switch (biome) {
    case BiomeType::DeepOcean:   r = 15 / 255.0f;  g = 45 / 255.0f;  b = 120 / 255.0f; break;
    case BiomeType::Ocean:       r = 25 / 255.0f;  g = 70 / 255.0f;  b = 160 / 255.0f; break;
    case BiomeType::Beach:       r = 220 / 255.0f; g = 205 / 255.0f; b = 150 / 255.0f; break;
    case BiomeType::Grassland:   r = 110 / 255.0f; g = 170 / 255.0f; b = 75 / 255.0f;  break;
    case BiomeType::Forest:      r = 40 / 255.0f;  g = 120 / 255.0f; b = 55 / 255.0f;  break;
    case BiomeType::DenseForest: r = 20 / 255.0f;  g = 85 / 255.0f;  b = 40 / 255.0f;  break;
    case BiomeType::Desert:      r = 210 / 255.0f; g = 180 / 255.0f; b = 100 / 255.0f; break;
    case BiomeType::Tundra:      r = 210 / 255.0f; g = 225 / 255.0f; b = 230 / 255.0f; break;
    case BiomeType::Taiga:       r = 50 / 255.0f;  g = 95 / 255.0f;  b = 75 / 255.0f;  break;
    case BiomeType::Mountain:    r = 110 / 255.0f; g = 100 / 255.0f; b = 90 / 255.0f;  break;
    case BiomeType::SnowPeak:    r = 240 / 255.0f; g = 245 / 255.0f; b = 250 / 255.0f; break;
    case BiomeType::Swamp:       r = 60 / 255.0f;  g = 90 / 255.0f;  b = 70 / 255.0f;  break;
    case BiomeType::Savanna:     r = 150 / 255.0f; g = 160 / 255.0f; b = 80 / 255.0f;  break;
    case BiomeType::Scrubland:   r = 130 / 255.0f; g = 140 / 255.0f; b = 90 / 255.0f;  break;
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mousePressed = true;
            firstMouse = true;
        }
        else if (action == GLFW_RELEASE) {
            mousePressed = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!mousePressed) return;

    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastMouseX;
    float yoffset = lastMouseY - ypos;
    lastMouseX = xpos;
    lastMouseY = ypos;

    cameraAngleY += xoffset * 0.3f;
    cameraAngleX += yoffset * 0.3f;
    cameraAngleX = clamp(cameraAngleX, -89.0f, 89.0f);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraDistance -= yoffset * 15.0f;
    cameraDistance = clamp(cameraDistance, 50.0f, 600.0f);
}

int main() {
    std::srand(std::time(0));
    World world(MAP_WIDTH, MAP_HEIGHT);

    PerlinNoise heightNoise(rand());
    PerlinNoise moistureNoise(rand());
    PerlinNoise temperatureNoise(rand());

    std::vector<BiomeType> biomeMap(MAP_WIDTH * MAP_HEIGHT);

    // Generate terrain
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            float nx = (float)x / MAP_WIDTH;
            float ny = (float)y / MAP_HEIGHT;

            float continents = heightNoise.fractalNoise(nx * 2.2f, ny * 2.2f, 3, 2.0f, 0.5f);
            float mediumDetail = heightNoise.fractalNoise(nx * 5.0f, ny * 5.0f, 4, 2.0f, 0.5f);
            float fineDetail = heightNoise.fractalNoise(nx * 12.0f, ny * 12.0f, 3, 2.0f, 0.4f);

            float height = continents * 0.55f + mediumDetail * 0.3f + fineDetail * 0.15f;

            float centerX = nx - 0.5f;
            float centerY = ny - 0.5f;
            float distFromCenter = std::sqrt(centerX * centerX + centerY * centerY);
            float islandMask = 1.0f - smoothstep(0.25f, 0.48f, distFromCenter);
            height = height * (0.3f + 0.7f * islandMask);

            height = (height + 1.0f) / 2.0f;
            height = clamp(height, 0.0f, 1.0f);

            float baseMoisture = moistureNoise.fractalNoise(nx * 3.5f, ny * 3.5f, 4, 2.1f, 0.5f);
            baseMoisture = (baseMoisture + 1.0f) / 2.0f;
            if (height < 0.45f) baseMoisture = std::min(1.0f, baseMoisture + 0.3f);
            float moisture = clamp(baseMoisture, 0.0f, 1.0f);

            float tempNoise = temperatureNoise.fractalNoise(nx * 2.8f, ny * 2.8f, 4, 2.0f, 0.5f);
            tempNoise = (tempNoise + 1.0f) / 2.0f;
            float elevationCooling = smoothstep(0.5f, 0.85f, height) * 0.35f;
            float temperature = tempNoise * 0.85f + 0.15f - elevationCooling;
            temperature = clamp(temperature, 0.0f, 1.0f);

            Tile& t = world.at(x, y);
            t.height = height;
            t.moisture = moisture;
            t.temperature = temperature;

            BiomeType biomeType = determineBiomeType(height, moisture, temperature);
            biomeMap[y * MAP_WIDTH + x] = biomeType;
        }
    }

    // Build 3D mesh with normals for lighting
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float heightScale = 50.0f;

    // First pass: create vertices with positions and colors
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            float height = world.at(x, y).height;
            BiomeType biome = biomeMap[y * MAP_WIDTH + x];

            float r, g, b;
            biomeToColor(biome, r, g, b);

            // Enhanced shading
            float shadeFactor = 0.5f + 0.7f * height;
            if (height > 0.65f) shadeFactor += 0.15f * (height - 0.65f);
            shadeFactor = clamp(shadeFactor, 0.3f, 1.3f);

            r *= shadeFactor;
            g *= shadeFactor;
            b *= shadeFactor;

            // Calculate normal for lighting (simple finite difference)
            float normalX = 0.0f, normalZ = 0.0f;
            if (x > 0 && x < MAP_WIDTH - 1) {
                float heightL = world.at(x - 1, y).height;
                float heightR = world.at(x + 1, y).height;
                normalX = (heightL - heightR) * heightScale;
            }
            if (y > 0 && y < MAP_HEIGHT - 1) {
                float heightD = world.at(x, y - 1).height;
                float heightU = world.at(x, y + 1).height;
                normalZ = (heightD - heightU) * heightScale;
            }

            glm::vec3 normal = glm::normalize(glm::vec3(normalX, 2.0f, normalZ));

            // Position (centered), Color, Normal
            vertices.push_back(x - MAP_WIDTH / 2.0f);
            vertices.push_back(height * heightScale);
            vertices.push_back(y - MAP_HEIGHT / 2.0f);
            vertices.push_back(r);
            vertices.push_back(g);
            vertices.push_back(b);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
        }
    }

    // Generate indices for triangle strips
    for (int y = 0; y < MAP_HEIGHT - 1; ++y) {
        for (int x = 0; x < MAP_WIDTH - 1; ++x) {
            int topLeft = y * MAP_WIDTH + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * MAP_WIDTH + x;
            int bottomRight = bottomLeft + 1;

            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 900, "DND 3D Map Generator", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glViewport(0, 0, 1200, 900);
    glEnable(GL_DEPTH_TEST);

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    const char* vertexShaderSource = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        layout(location = 2) in vec3 aNormal;
        
        out vec3 vertexColor;
        out vec3 normal;
        out vec3 fragPos;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            fragPos = vec3(model * vec4(aPos, 1.0));
            normal = mat3(transpose(inverse(model))) * aNormal;
            vertexColor = aColor;
        }
    )glsl";

    const char* fragmentShaderSource = R"glsl(
        #version 330 core
        in vec3 vertexColor;
        in vec3 normal;
        in vec3 fragPos;
        out vec4 FragColor;
        
        uniform vec3 lightDir;
        uniform vec3 viewPos;
        
        void main() {
            // Ambient
            float ambientStrength = 0.4;
            vec3 ambient = ambientStrength * vertexColor;
            
            // Diffuse
            vec3 norm = normalize(normal);
            vec3 lightDirection = normalize(lightDir);
            float diff = max(dot(norm, lightDirection), 0.0);
            vec3 diffuse = diff * vertexColor;
            
            // Specular (subtle)
            vec3 viewDir = normalize(viewPos - fragPos);
            vec3 reflectDir = reflect(-lightDirection, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = 0.15 * spec * vec3(1.0, 1.0, 1.0);
            
            vec3 result = ambient + diffuse + specular;
            FragColor = vec4(result, 1.0);
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

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // Sky blue background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Camera
        float camX = cameraDistance * cos(glm::radians(cameraAngleX)) * cos(glm::radians(cameraAngleY));
        float camY = cameraDistance * sin(glm::radians(cameraAngleX));
        float camZ = cameraDistance * cos(glm::radians(cameraAngleX)) * sin(glm::radians(cameraAngleY));

        glm::vec3 cameraPos = glm::vec3(camX, camY, camZ);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(
            cameraPos,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1200.0f / 900.0f, 0.1f, 1000.0f);

        // Light from upper right
        glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 2.0f, 1.0f));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}