#include <google/cloud/pubsub/subscriber.h>
#include <google/cloud/pubsub/subscription.h>
#include <google/cloud/spanner/client.h>
#include <google/cloud/spanner/mutations.h>
#include <google/cloud/spanner/timestamp.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <random>
#include <sstream>
#include <iomanip>

namespace pubsub = google::cloud::pubsub;
namespace spanner = google::cloud::spanner;

// Generate UUID v4
std::string generate_uuid() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 15);
  std::uniform_int_distribution<> dis2(8, 11);

  std::stringstream ss;
  ss << std::hex;
  
  for (int i = 0; i < 8; i++) {
    ss << dis(gen);
  }
  ss << "-";
  for (int i = 0; i < 4; i++) {
    ss << dis(gen);
  }
  ss << "-4";
  for (int i = 0; i < 3; i++) {
    ss << dis(gen);
  }
  ss << "-";
  ss << dis2(gen);
  for (int i = 0; i < 3; i++) {
    ss << dis(gen);
  }
  ss << "-";
  for (int i = 0; i < 12; i++) {
    ss << dis(gen);
  }
  
  return ss.str();
}

int main(int argc, char* argv[]) {
  // Get environment variables
  const char* project_id = std::getenv("GOOGLE_CLOUD_PROJECT");
  const char* subscription_id = std::getenv("PUBSUB_SUBSCRIPTION");
  const char* instance_id = std::getenv("SPANNER_INSTANCE");
  const char* database_id = std::getenv("SPANNER_DATABASE");
  
  if (!project_id) {
    std::cerr << "ERROR: GOOGLE_CLOUD_PROJECT environment variable not set\n";
    return 1;
  }
  
  if (!subscription_id) {
    std::cerr << "ERROR: PUBSUB_SUBSCRIPTION environment variable not set\n";
    return 1;
  }
  
  if (!instance_id) {
    std::cerr << "ERROR: SPANNER_INSTANCE environment variable not set\n";
    return 1;
  }
  
  if (!database_id) {
    std::cerr << "ERROR: SPANNER_DATABASE environment variable not set\n";
    return 1;
  }

  // Initialize Pub/Sub subscriber
  auto subscriber = pubsub::Subscriber(
    pubsub::MakeSubscriberConnection(
      pubsub::Subscription(std::string(project_id), std::string(subscription_id))
    )
  );

  // Initialize Spanner client
  auto spanner_client = spanner::Client(
    spanner::MakeConnection(
      spanner::Database(std::string(project_id), std::string(instance_id), std::string(database_id))
    )
  );

  std::cout << "Subscriber starting...\n";
  std::cout << "Project ID: " << project_id << "\n";
  std::cout << "Subscription: " << subscription_id << "\n";
  std::cout << "Spanner Instance: " << instance_id << "\n";
  std::cout << "Spanner Database: " << database_id << "\n";
  std::cout << "Listening for messages...\n\n";

  // Subscribe and process messages
  auto session = subscriber.Subscribe(
    [&spanner_client](pubsub::Message const& m, pubsub::AckHandler h) {
      try {
        // Get event text from message
        std::string event_text = m.data();
        
        // Generate UUID for EventId
        std::string event_id = generate_uuid();
        
        std::cout << "Received event: " << event_text << "\n";
        std::cout << "Generated EventId: " << event_id << "\n";
        
        // Insert into Spanner
        auto mutation = spanner::MakeInsertMutation(
          "Events",
          {"EventId", "EventText", "CreatedAt"},
          spanner::Value(event_id),
          spanner::Value(event_text),
          spanner::CommitTimestamp{}
        );
        
        auto commit_result = spanner_client.Commit(
          [&mutation](spanner::Transaction const&) -> google::cloud::StatusOr<spanner::Mutations> {
            return spanner::Mutations{mutation};
          }
        );
        
        if (!commit_result) {
          std::cerr << "Error writing to Spanner: " << commit_result.status() << "\n";
          std::move(h).nack();
          return;
        }
        
        std::cout << "Successfully written to Spanner\n\n";
        
        // Acknowledge message
        std::move(h).ack();
        
      } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << "\n";
        std::move(h).nack();
      }
    }
  );

  // Wait for messages (blocking)
  std::cout << "Press Ctrl+C to stop...\n";
  auto status = session.get();
  
  if (!status.ok()) {
    std::cerr << "Subscription error: " << status << "\n";
    return 1;
  }

  return 0;
}