# GCP Event Logger

Two small Python services that turn any HTTP event into a durable record inside Google Cloud. The publisher exposes a `/log` endpoint, drops every payload onto Pub/Sub, and the subscriber pulls those messages and writes them into a Spanner table. Both services expose `/health` so you can keep an eye on them.

## What’s inside
- `publisher/`: Flask app that accepts `{"event": "..."}` JSON and publishes to Pub/Sub.
- `subscriber/`: Background Pub/Sub subscriber that stores each event in the `Events` table (columns `EventId`, `EventText`, `CreatedAt`) and serves a simple health check.
- `Dockerfile.publisher` / `Dockerfile.subscriber`: Two-stage images used in Cloud Build or locally.
- `cloudbuild.yaml`: Builds and pushes both images (`gcr.io/$PROJECT_ID/publisher` and `.../subscriber`).

## Environment
Set these variables for both local runs and container deployments:

| Variable | Publisher | Subscriber |
| --- | --- | --- |
| `GOOGLE_CLOUD_PROJECT` | ✅ | ✅ |
| `PUBSUB_TOPIC` | ✅ | – |
| `PUBSUB_SUBSCRIPTION` | – | ✅ |
| `SPANNER_INSTANCE` | – | ✅ |
| `SPANNER_DATABASE` | – | ✅ |
| `PORT` (optional) | ✅ | ✅ |

The subscriber expects a Spanner instance/database with an `Events` table containing the columns listed above. Both services use Application Default Credentials (ADC), so run them where the default service account has Pub/Sub and Spanner permissions or provide a service-account key via `GOOGLE_APPLICATION_CREDENTIALS`.

**Note:** Using Spanner Emulator for local development due to GCP costing (~$0.30/hour per node). Set `SPANNER_EMULATOR_HOST=localhost:9010` and run `gcloud emulators spanner start --host-port=localhost:9010`. The subscriber automatically uses the emulator when this variable is set.

### Spanner schema
Use this DDL once per database to create the table expected by the subscriber:

```sql
CREATE TABLE Events (
  EventId STRING(36) NOT NULL,
  EventText STRING(MAX) NOT NULL,
  CreatedAt TIMESTAMP NOT NULL OPTIONS (allow_commit_timestamp = true)
) PRIMARY KEY (EventId);

-- Index for efficient time-based queries
CREATE INDEX EventsByCreatedAt ON Events(CreatedAt DESC);
```

The schema is optimized for low-latency writes:
- Primary key on `EventId` (UUID) for fast lookups
- `COMMIT_TIMESTAMP` for automatic timestamping without extra round-trips
- Secondary index on `CreatedAt DESC` for efficient time-range queries
- `STRING(MAX)` for `EventText` to avoid length constraints

### Pub/Sub reliability
The subscription is configured with retry and dead-letter handling:
- **Retry policy**: Messages are retried up to 5 times with exponential backoff
- **Dead-letter topic**: Messages that fail after max retries are forwarded to `event-logs-dlq` for manual inspection
- **Ack deadline**: 10 seconds (messages are redelivered if not acknowledged)

To set this up:
```bash
# Create dead-letter topic
gcloud pubsub topics create event-logs-dlq

# Configure subscription with dead-letter policy
gcloud pubsub subscriptions update event-logs-sub \
  --dead-letter-topic=event-logs-dlq \
  --max-delivery-attempts=5
```

## Local development
```bash
python -m venv .venv && source .venv/bin/activate
pip install -r publisher/requirements.txt
pip install -r subscriber/requirements.txt

# Publisher
export GOOGLE_CLOUD_PROJECT=your-project
export PUBSUB_TOPIC=event-logs-topic
python publisher/main.py

# Subscriber (in another shell)
export PUBSUB_SUBSCRIPTION=event-logs-sub
export SPANNER_INSTANCE=event-logger-instance
export SPANNER_DATABASE=event-logs-db
python subscriber/main.py
```

POST an event:
```bash
curl -X POST http://localhost:8080/log \
  -H "Content-Type: application/json" \
  -d '{"event":"demo from README"}'
```

## Building containers
Local Docker builds:
```bash
docker build -f Dockerfile.publisher -t publisher:local .
docker build -f Dockerfile.subscriber -t subscriber:local .
```

Cloud Build (pushes both images defined in `cloudbuild.yaml`):
```bash
gcloud builds submit --config cloudbuild.yaml
```

### Deploying to Cloud Run (example)
```bash
# Publisher
gcloud run deploy event-logger-publisher \
  --image gcr.io/$PROJECT_ID/publisher:latest \
  --region=us-central1 \
  --platform=managed \
  --set-env-vars=GOOGLE_CLOUD_PROJECT=$PROJECT_ID,PUBSUB_TOPIC=event-logs-topic \
  --allow-unauthenticated

# Subscriber
gcloud run deploy event-logger-subscriber \
  --image gcr.io/$PROJECT_ID/subscriber:latest \
  --region=us-central1 \
  --platform=managed \
  --set-env-vars=GOOGLE_CLOUD_PROJECT=$PROJECT_ID,PUBSUB_SUBSCRIPTION=event-logs-sub,\
SPANNER_INSTANCE=event-logger-instance,SPANNER_DATABASE=event-logs-db
```

Grant each service account Pub/Sub and Spanner permissions, or specify `--service-account` if you need separate principals.

## Health checks
- Publisher: `GET /health` returns `{"status": "healthy"}`.
- Subscriber: Flask app on its configured port exposes the same endpoint; the worker thread continues to process messages in the background.

That’s it—small, auditable, and ready for your event trail.
