
# Liu I have to use the sdl library in system dir, since it is too hard for me to get extension lib in contrib/SDL-mirror
INCLUDE(FindPkgConfig)
# PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
# PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image)
# PKG_SEARCH_MODULE(SDL2TTF REQUIRED SDL2_ttf)

# All the source files for the sc2 api
# Liu: of course the second parameter of each function call is variable.
# Liu: this is used to collect all the corresponding files and store them into several variables
file(GLOB sources_sc2 "sc2api/*.cpp" "sc2api/*.cc" "../include/sc2api/*.h")
file(GLOB sources_sc2_lib "sc2lib/*.cpp" "sc2lib/*.cc" "../include/sc2lib/*.h")
file(GLOB sources_utils "sc2utils/*.cpp" "sc2utils/*.cc" "../include/sc2utils/*.h")
file(GLOB sources_renderer "sc2renderer/*.cc" "sc2renderer/*.h" "../include/sc2renderer/*.h")
# Liu: What I need to do is to add my folders and files here to let it can be built into libs
file(GLOB sources_debug_renderer "debug_renderer/*.cc" "debug_renderer/*.h" "../include/debug_renderer/*.h")

# Gather the protos.
file(GLOB proto_files
    "../protocol/s2clientprotocol/*.proto" # How can CMake use them?
)

# Liu: OKOK string equal... if there isn't anything, it means you need to reconstruct it again
if ("${proto_files}" STREQUAL "") # Liu: protocol is a submodule...
	message(FATAL_ERROR "Missing root protocol submodule.\nRemove your protocol directory and run:\ngit submodule update --recursive --init")
endif ()

# Create the output directory for generated protos.
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/generated/s2clientprotocol) # Liu: That's it, in this dir, all the files are *.h or *.cc
# Liu: Looks like the CMake can also call the proto generator to generate corresponding files

# Liu: for each proto file 
foreach(proto ${proto_files}) # it is the *.proto s
    # Liu: gets the filename without directory and longest file extension
    get_filename_component(proto_name ${proto} NAME_WE) 
    # Liu: emmm... it is used to prepare to ... generate the corresponging *.h and *.c, of course
    list(APPEND proto_src ${PROJECT_BINARY_DIR}/generated/s2clientprotocol/${proto_name}.pb.h ${PROJECT_BINARY_DIR}/generated/s2clientprotocol/${proto_name}.pb.cc)
endforeach()

# Now include that directory
include_directories(SYSTEM "${PROJECT_BINARY_DIR}/generated") # this is the directory of .hs which proto generates

# Setup MSVC parallelized builds
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

# Create the library
add_library(sc2api ${sources_sc2})
add_library(sc2lib ${sources_sc2_lib})
# Liu: ? Looks like I can add a library from an upper dir's CMakeLists.txt....
add_library(sc2protocol ${proto_src} ${proto_files}) # Liu: how can CMake use .proto files? Fuck, looks like it is able to do that
add_library(sc2utils ${sources_utils}) 

#? Liu: What are those functions used for?
set_target_properties(sc2api PROPERTIES DEBUG_POSTFIX "d") # Liu: now the target is obviously *.libs files
set_target_properties(sc2lib PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(sc2protocol PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(sc2utils PROPERTIES DEBUG_POSTFIX "d")

#? Liu: Where can it know the libprotobuf is? I didn't see a setting option in this file
target_link_libraries(sc2protocol libprotobuf)

if (APPLE)
    target_link_libraries(sc2utils "-framework Carbon")
endif ()

# Exclude SDL and related projects to work around linker issue.
# Liu: looks like I need to configure the renderer and its dependency SDL inidvidually
if (NOT APPLE)
    add_library(sc2renderer ${sources_renderer})
    set_target_properties(sc2renderer PROPERTIES DEBUG_POSTFIX "d")
    target_link_libraries(sc2renderer SDL2-static)
    include_directories("../contrib/SDL-mirror/include")
endif ()

# It requires includes to civetweb and our own headers.
#? Liu: If I have a CMakeList file like this, will I still need to write the whole dir to include a *.h?
# Liu: looks like the IDE shouldn't have added these include files to intellisense, but it does, so it gets me puzzled. I mean, I should have done it twice: first let the intellisense know, second let the compiler know
include_directories("../contrib/civetweb/include")
include_directories("../contrib/SDL-mirror/include")
include_directories("../contrib/ipv6-parse")
include_directories("../include")

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# Liu: Looks like all the options can be set in VS graphically should be set by hands in commands now
if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd /ZI /Od" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "/MT" CACHE STRING "" FORCE)

    set_target_properties(civetweb-c-library PROPERTIES COMPILE_FLAGS "/MT")
else()
    set(CMAKE_CXX_FLAGS "-fpermissive")  
endif()

# Liu: if you have this statement, Cmake will check whether the dependencies at lower level has been built, if not, it will build it first
add_dependencies(sc2protocol protoc)

foreach (proto ${proto_files})
    get_filename_component(proto_name ${proto} NAME)
    get_filename_component(proto_name_we ${proto} NAME_WE)
    set(outputCC ${PROJECT_BINARY_DIR}/generated/s2clientprotocol/${proto_name_we}.pb.cc)
    set(outputH ${PROJECT_BINARY_DIR}/generated/s2clientprotocol/${proto_name_we}.pb.h)

    # Here the original user defined how to generate all the corresponding files from *.protos
    add_custom_command(
        DEPENDS protoc
        COMMAND ${PROJECT_BINARY_DIR}/bin/protoc -I=${CMAKE_CURRENT_SOURCE_DIR}/../protocol/ --cpp_out=${PROJECT_BINARY_DIR}/generated ${proto}
        OUTPUT ${outputCC} ${outputH}
    )

endforeach()


if (MSVC)
    set_target_properties(sc2api PROPERTIES COMPILE_FLAGS "/W4 /WX")
    set_target_properties(sc2lib PROPERTIES COMPILE_FLAGS "/W4 /WX")
    set_target_properties(sc2renderer PROPERTIES COMPILE_FLAGS "/W4 /WX")
    set_target_properties(debug_renderer PROPERTIES COMPILE_FLAGS "/W4 /WX")
    set_target_properties(sc2utils PROPERTIES COMPILE_FLAGS "/W3 /WX")
    set_target_properties(sc2protocol PROPERTIES COMPILE_FLAGS "/W0")
endif (MSVC)

# Liu: come on, links all the libs once! The more based libs has been linked before
target_link_libraries(sc2api sc2protocol civetweb-c-library ipv6-parse) 
# Liu: looks like the sc2lib is at higher level than sc2api
target_link_libraries(sc2lib sc2api) 

# Liu: configure my renderer here, it may need to be handled at last, since it need sc2lib...
if(NOT APPLE)
    add_library(debug_renderer ${sources_debug_renderer})
    set_target_properties(debug_renderer PROPERTIES DEBUG_POSTFIX "d")
    # TARGET_LINK_LIBRARIES(debug_renderer ${SDL2_LIBRARIES} ${SDL2TTF_LIBRARIES})
    target_link_libraries(debug_renderer sc2api) #! Note that it should be added since debug_renderer has used it 
    #Liu ADD
    # INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
    # INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES(debug_renderer SDL2-static) #! I can not use the SDL2_ttf library, so I deleted this library here. Maybe someday I can find the solution and add it here
    # INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIRS}) 
    # include_directories("../contrib/SDL-mirror/include")
endif()
