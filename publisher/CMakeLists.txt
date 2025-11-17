add_executable(publisher main.cpp)

target_link_libraries(publisher
  PRIVATE
    google-cloud-cpp::pubsub
    google-cloud-cpp::common
    cpphttplib::cpphttplib
    nlohmann_json::json
    Threads::Threads
)

target_compile_options(publisher PRIVATE -Wall -Wextra)

