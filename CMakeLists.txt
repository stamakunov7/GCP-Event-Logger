cmake_minimum_required(VERSION 3.20)
project(GCP-Event-Logger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Google Cloud C++ libraries using FetchContent
include(FetchContent)
FetchContent_Declare(
  google_cloud_cpp
  GIT_REPOSITORY https://github.com/googleapis/google-cloud-cpp.git
  GIT_TAG v3.0.0
)
set(GOOGLE_CLOUD_CPP_ENABLE "pubsub;spanner" CACHE STRING "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(google_cloud_cpp)

# HTTP server library (cpp-httplib) - only for publisher
FetchContent_Declare(
  cpphttplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(cpphttplib)

# JSON library - only for publisher
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Add subdirectories
add_subdirectory(publisher)
add_subdirectory(subscriber)

