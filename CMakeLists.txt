# 11:28 AM 1/11/2025
# by rajib chy

cmake_minimum_required(VERSION 3.12)

# Project name
project(service_manager VERSION 4.0.1.1 LANGUAGES C CXX)

# Define APP_VERSION as a preprocessor macro globally
add_definitions(-DAPP_VERSION="4.0.1.1")

# Specify the required C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suppress the deprecated warning for GCC and Clang
# e.g.  auto lambda = [this]() { return x; };  // Explicit capture of 'this'
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-deprecated)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-unused-result)
endif()

if(MSVC)
    add_compile_options(/wd4100)  # Disables warning C4100 (unreferenced formal parameter)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    /usr/include
)

# Sources
set(SOURCES
    src/config.cpp
    src/json-config.cpp
    src/manager.cpp
    src/http.cpp
    src/httpc.cpp
    src/logger.cpp
    src/handler.cpp
    src/dust-cleaner.cpp
    src/time-range.cpp
    src/service.cpp
    src/service-test.cpp
)
# Set the name of the executable
set(EXECUTABLE_NAME service_manager)

# Define output directories
set(CUSTOM_OUTPUT_BASE "${CMAKE_SOURCE_DIR}/../output/service_manager")
set(CUSTOM_BIN_DIR ${CUSTOM_OUTPUT_BASE}/bin)

# Add the executable target with the defined sources
add_executable(${EXECUTABLE_NAME} ${SOURCES})

# Specify additional include directories
target_include_directories(${EXECUTABLE_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
if(NOT WIN32)
	# Add "apt install libsdbus-c++1" at runtime while using this executable file at other machine
	# sudo apt install libsdbus-c++-dev
	# Specify sdbus-c++ and must be required for this project
	## find_package(sdbus-c++ REQUIRED)
	## target_link_libraries(${EXECUTABLE_NAME} PRIVATE sdbus-c++)

	find_package(sdbus-c++ REQUIRED)

	# Explicitly set library and include paths
	set(SDBUS_STATIC_LIB_PATH "/usr/local/lib/libsdbus-c++.a")
	set(SDBUS_INCLUDE_DIR "/usr/local/include/sdbus-c++")

	# Ensure paths exist before proceeding
	if (EXISTS ${SDBUS_STATIC_LIB_PATH} AND EXISTS ${SDBUS_INCLUDE_DIR})
		message(STATUS "Found static sdbus-c++ library: ${SDBUS_STATIC_LIB_PATH}")
		message(STATUS "Found sdbus-c++ include directory: ${SDBUS_INCLUDE_DIR}")
	else()
		message(FATAL_ERROR "sdbus-c++ static library or include path not found! Ensure it is installed correctly.")
	endif()

	# Use the correct include and library paths
	target_include_directories(${EXECUTABLE_NAME} PRIVATE ${SDBUS_INCLUDE_DIR})
	target_link_libraries(${EXECUTABLE_NAME} 
		PRIVATE
		${SDBUS_STATIC_LIB_PATH}  # Link static version
		systemd                   # Required for libsystemd
	)
else()
	message(STATUS "sdbus-c++ not supported at win32")
endif()

if(NOT WIN32)

	# Check for nlohmann/json.hpp
	find_path(NLOHMANN_JSON_INCLUDE_DIR
		nlohmann/json.hpp
		PATHS /usr/local/include /usr/include
	)

	if (NOT NLOHMANN_JSON_INCLUDE_DIR)
		message(FATAL_ERROR "nlohmann/json.hpp not found! Please ensure it is installed in /usr/local/include or /usr/include.")
	else()
		message(STATUS "Found nlohmann/json.hpp at ${NLOHMANN_JSON_INCLUDE_DIR}")
		target_include_directories(${EXECUTABLE_NAME} PRIVATE ${NLOHMANN_JSON_INCLUDE_DIR})
	endif()
	
else()

	set(NLOHMANN_JSON_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../dependency/json/include")
	
	# Ensure paths exist before proceeding
	if (NOT EXISTS ${NLOHMANN_JSON_INCLUDE_DIR})
		message(FATAL_ERROR "nlohmann/json.hpp not found! Please ensure it is installed in ${NLOHMANN_JSON_INCLUDE_DIR}")
	else()
		message(STATUS "Found nlohmann/json.hpp at ${NLOHMANN_JSON_INCLUDE_DIR}")
		target_include_directories(${EXECUTABLE_NAME} PRIVATE ${NLOHMANN_JSON_INCLUDE_DIR})
	endif()
	

endif()


# Define the USE_HTTP_DAY_STATUS option with a default value of ON
option(USE_HTTP_DAY_STATUS "Enable HTTP Day Status Check" ON)

if(USE_HTTP_DAY_STATUS)
    target_compile_definitions(${EXECUTABLE_NAME} PUBLIC USE_HTTP_DAY_STATUS)
    # Include any USE_HTTP_DAY_STATUS specific directories or libraries here if necessary
    message(STATUS "USE_HTTP_DAY_STATUS support enabled")
else()
    message(STATUS "USE_HTTP_DAY_STATUS support disabled")
endif()

# Define the USE_PRODUCTION_BUILD option with a default value of ON
option(USE_PRODUCTION_BUILD "Enable Production Build" ON)

if(USE_PRODUCTION_BUILD)
    target_compile_definitions(${EXECUTABLE_NAME} PUBLIC USE_PRODUCTION_BUILD)
    # Include any USE_PRODUCTION_BUILD specific directories or libraries here if necessary
    message(STATUS "USE_PRODUCTION_BUILD support enabled")
else()
    message(STATUS "USE_PRODUCTION_BUILD support disabled")
endif()

# Dynamically adjust output directories based on build type.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CUSTOM_BIN_DIR}/${CMAKE_BUILD_TYPE})

# Optionally add flags for optimization
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
endif()

# If needed, link libraries (example: pthread for Linux)
if(NOT WIN32)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE pthread)
endif()

# Add platform-specific settings (for Linux or Windows)
if(WIN32)
    # Example for Windows specific configuration
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES
        WIN32_EXECUTABLE YES
    )
endif()

# Custom install rules (optional)
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Debug output
message(STATUS "Executable Name: ${EXECUTABLE_NAME}")
message(STATUS "Output Directory: ${CUSTOM_BIN_DIR}")
message(STATUS "Runtime Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

## run hints
# Clear the cache (optional but recommended)
## rm -rf CMakeCache.txt CMakeFiles/

# Re-run cmake with the updated option
## cmake -DCMAKE_BUILD_TYPE=Release -DUSE_HTTP_DAY_STATUS=OFF ..
