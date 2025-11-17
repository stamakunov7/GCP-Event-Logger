cmake_minimum_required(VERSION 3.20)
project(GCP-Event-Logger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Google Cloud C++ libraries
find_package(google_cloud_cpp_pubsub REQUIRED)
find_package(google_cloud_cpp_spanner REQUIRED)

# HTTP server library (cpp-httplib)
include(FetchContent)
FetchContent_Declare(
  cpphttplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(cpphttplib)

# JSON library
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# UUID library
FetchContent_Declare(
  uuid
  GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
  GIT_TAG v1.2.3
)
FetchContent_MakeAvailable(uuid)

# Add subdirectories
add_subdirectory(publisher)
add_subdirectory(subscriber)

