# Come on, how can I write my own CMakeLists...

INCLUDE(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image)
PKG_SEARCH_MODULE(SDL2TTF REQUIRED SDL2_ttf)

include_directories("../include")

# Setup MSVC parallelized builds
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

# Include
include_directories(SYSTEM "${PROJECT_BINARY_DIR}/generated")

# Link all the *.libs
# I can collect all the *.cc one time and store them into one variable
file(GLOB project_sources "project/*.cc" "project/*.cpp" "project/*.h") # emmm... *.h is also source file...
# make all the *.cc into an executable
add_executable(project ${project_sources})
# Set the output folder, since the binary dir has been set in main CMakeList, I can just set the subfolder.
set_target_properties(project PROPERTIES FOLDER project)
# Link all the libraries this project will use
target_link_libraries(project sc2api sc2lib sc2utils liu_renderer) # here can be multiple libs to be added

