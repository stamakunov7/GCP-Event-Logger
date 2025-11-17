#include <google/cloud/pubsub/publisher.h>
#include <google/cloud/pubsub/message.h>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <cstdlib>

using json = nlohmann::json;

namespace pubsub = google::cloud::pubsub;

int main(int argc, char* argv[]) {
  // Get environment variables
  const char* project_id = std::getenv("GOOGLE_CLOUD_PROJECT");
  const char* topic_id = std::getenv("PUBSUB_TOPIC");
  
  if (!project_id) {
    std::cerr << "ERROR: GOOGLE_CLOUD_PROJECT environment variable not set\n";
    return 1;
  }
  
  if (!topic_id) {
    std::cerr << "ERROR: PUBSUB_TOPIC environment variable not set\n";
    return 1;
  }

  // Initialize Pub/Sub publisher
  auto publisher = pubsub::Publisher(
    pubsub::MakePublisherConnection(
      pubsub::Topic(std::string(project_id), std::string(topic_id))
    )
  );

  // Create HTTP server
  httplib::Server svr;

  // POST /log endpoint
  svr.Post("/log", [&publisher](const httplib::Request& req, httplib::Response& res) {
    try {
      // Parse JSON body
      auto body_json = json::parse(req.body);
      
      if (!body_json.contains("event") || !body_json["event"].is_string()) {
        res.status = 400;
        res.set_content(json({{"error", "Missing or invalid 'event' field"}}).dump(), "application/json");
        return;
      }

      std::string event_text = body_json["event"];
      
      // Create Pub/Sub message
      auto message = pubsub::MessageBuilder{}
        .SetData(event_text)
        .Build();

      // Publish message
      auto future = publisher.Publish(std::move(message));
      auto message_id = future.get();
      
      if (!message_id) {
        std::cerr << "Error publishing message: " << message_id.status() << "\n";
        res.status = 500;
        res.set_content(json({{"error", "Failed to publish message"}}).dump(), "application/json");
        return;
      }

      // Success response
      res.status = 200;
      res.set_content(json({
        {"status", "success"},
        {"message_id", *message_id}
      }).dump(), "application/json");
      
      std::cout << "Published event: " << event_text << " (message_id: " << *message_id << ")\n";
      
    } catch (const json::parse_error& e) {
      res.status = 400;
      res.set_content(json({{"error", "Invalid JSON"}}).dump(), "application/json");
    } catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << "\n";
      res.status = 500;
      res.set_content(json({{"error", "Internal server error"}}).dump(), "application/json");
    }
  });

  // Health check endpoint
  svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
    res.status = 200;
    res.set_content(json({{"status", "healthy"}}).dump(), "application/json");
  });

  // Start server
  const char* port_env = std::getenv("PORT");
  int port = port_env ? std::stoi(port_env) : 8080;
  
  std::cout << "Publisher server starting on port " << port << "\n";
  std::cout << "Project ID: " << project_id << "\n";
  std::cout << "Topic: " << topic_id << "\n";
  
  if (!svr.listen("0.0.0.0", port)) {
    std::cerr << "Failed to start server\n";
    return 1;
  }

  return 0;
}