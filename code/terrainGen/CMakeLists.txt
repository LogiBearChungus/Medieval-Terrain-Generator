#Change this for each assignment!
set(APPNAME terrainGen)

#additional source files will have to be added here
add_executable(${APPNAME}    
    main.cpp
    world/World.cpp
    noise/PerlinNoise.cpp
    terrain/TerrainGenerator.cpp
    terrain/RiverGenerator.cpp
    roads/AntColony.cpp
    render/Renderer.cpp 
    world/Tile.h)

#Link + include dependencies
target_link_libraries(${APPNAME} PUBLIC core IMGUI glm)
target_include_directories(${APPNAME} PUBLIC ${CORE_INC_DIR} ${stb_INCLUDE_DIR})