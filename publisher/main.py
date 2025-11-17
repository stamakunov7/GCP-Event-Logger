import os
import json
from flask import Flask, request, jsonify
from google.cloud import pubsub_v1

app = Flask(__name__)

# Environment variables
PROJECT_ID = os.environ.get('GOOGLE_CLOUD_PROJECT')
TOPIC_ID = os.environ.get('PUBSUB_TOPIC', 'event-logs-topic')

# Initialize Pub/Sub publisher
publisher = pubsub_v1.PublisherClient()
topic_path = publisher.topic_path(PROJECT_ID, TOPIC_ID)

@app.route('/log', methods=['POST'])
def log_event():
    """Endpoint to receive events and publish to Pub/Sub"""
    try:
        data = request.get_json()
        
        if not data or 'event' not in data:
            return jsonify({'error': 'Missing or invalid "event" field'}), 400
        
        event_text = data['event']
        
        # Publish to Pub/Sub
        message_data = event_text.encode('utf-8')
        future = publisher.publish(topic_path, message_data)
        message_id = future.result()
        
        print(f'Published event: {event_text} (message_id: {message_id})')
        
        return jsonify({
            'status': 'success',
            'message_id': message_id
        }), 200
        
    except Exception as e:
        print(f'Error: {str(e)}')
        return jsonify({'error': 'Internal server error'}), 500

@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint"""
    return jsonify({'status': 'healthy'}), 200

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 8080))
    app.run(host='0.0.0.0', port=port)

