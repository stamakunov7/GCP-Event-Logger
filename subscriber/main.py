import os
import uuid
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

def main():
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
    
    print('Press Ctrl+C to stop...')
    
    try:
        streaming_pull_future.result()
    except KeyboardInterrupt:
        streaming_pull_future.cancel()
        print('\nSubscriber stopped.')

if __name__ == '__main__':
    main()

