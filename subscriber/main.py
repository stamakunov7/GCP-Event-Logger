import os
import uuid
import threading
from flask import Flask
from google.cloud import pubsub_v1, spanner

# Environment variables
PROJECT_ID = os.environ.get('GOOGLE_CLOUD_PROJECT')
SUBSCRIPTION_ID = os.environ.get('PUBSUB_SUBSCRIPTION', 'event-logs-sub')
INSTANCE_ID = os.environ.get('SPANNER_INSTANCE', 'event-logger-instance')
DATABASE_ID = os.environ.get('SPANNER_DATABASE', 'event-logs-db')

# Initialize Spanner client
spanner_client = spanner.Client(project=PROJECT_ID)
instance = spanner_client.instance(INSTANCE_ID)
database = instance.database(DATABASE_ID)

# Flask app for health check
app = Flask(__name__)

@app.route('/health', methods=['GET'])
def health():
    return {'status': 'healthy'}, 200

def callback(message):
    """Process messages from Pub/Sub"""
    try:
        # Get event text from message
        event_text = message.data.decode('utf-8')
        
        # Generate UUID for EventId
        event_id = str(uuid.uuid4())
        
        print(f'Received event: {event_text}')
        print(f'Generated EventId: {event_id}')
        
        # Insert into Spanner
        def insert_event(transaction):
            transaction.insert(
                table='Events',
                columns=['EventId', 'EventText', 'CreatedAt'],
                values=[(event_id, event_text, spanner.COMMIT_TIMESTAMP)]
            )
        
        database.run_in_transaction(insert_event)
        
        print(f'Successfully written to Spanner\n')
        
        # Acknowledge message
        message.ack()
        
    except Exception as e:
        print(f'Error processing message: {str(e)}')
        message.nack()

def start_subscriber():
    """Subscribe to Pub/Sub and process messages"""
    subscriber = pubsub_v1.SubscriberClient()
    subscription_path = subscriber.subscription_path(PROJECT_ID, SUBSCRIPTION_ID)
    
    print(f'Subscriber starting...')
    print(f'Project ID: {PROJECT_ID}')
    print(f'Subscription: {SUBSCRIPTION_ID}')
    print(f'Spanner Instance: {INSTANCE_ID}')
    print(f'Spanner Database: {DATABASE_ID}')
    print(f'Listening for messages...\n')
    
    streaming_pull_future = subscriber.subscribe(subscription_path, callback=callback)
    
    try:
        streaming_pull_future.result()
    except Exception as e:
        print(f'Subscriber error: {str(e)}')

if __name__ == '__main__':
    # Start subscriber in background thread
    subscriber_thread = threading.Thread(target=start_subscriber, daemon=True)
    subscriber_thread.start()
    
    # Start Flask health check server
    port = int(os.environ.get('PORT', 8080))
    app.run(host='0.0.0.0', port=port)

