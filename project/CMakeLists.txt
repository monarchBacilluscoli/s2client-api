# Come on, how can I write my own CMakeLists...

include_directories("../include")
include_directories("./")

# Setup MSVC parallelized builds
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

# Include
include_directories(SYSTEM "${PROJECT_BINARY_DIR}/generated")

# Link all the *.libs
# I can collect all the *.cc one time and store them into one variable
# GLOB_RECURSE means to collect all the files even in subfolders 
file(GLOB_RECURSE project_sources "*.cc" "*.cpp" "*.h") # emmm... *.h is also source file...
# make all the *.cc into an executable
add_executable(project ${project_sources} ${rolling_bot_sources})
# Set the output folder, since the binary dir has been set in main CMakeList, I can just set the subfolder.
set_target_properties(project PROPERTIES FOLDER project)
# Link all the libraries this project will use
target_link_libraries(project sc2api sc2lib sc2utils debug_renderer) # here can be multiple libs to be added