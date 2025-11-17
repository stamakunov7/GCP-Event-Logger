add_executable(subscriber main.cpp)

target_link_libraries(subscriber
  PRIVATE
    google-cloud-cpp::pubsub
    google-cloud-cpp::spanner
    Threads::Threads
)

target_compile_options(subscriber PRIVATE -Wall -Wextra)

