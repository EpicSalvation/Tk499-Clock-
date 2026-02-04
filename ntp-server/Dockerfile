# NTP Server Docker Image
# Lightweight Python-based NTP server for local network time synchronization

FROM python:3.12-slim

# Labels for container metadata
LABEL maintainer="TK499 Clock Project"
LABEL description="Simple NTP server for local network time synchronization"
LABEL version="1.0.0"

# Create non-root user for security
RUN groupadd -r ntp && useradd -r -g ntp ntp

# Set working directory
WORKDIR /app

# Copy application files
COPY ntp_server.py .
COPY requirements.txt .

# Install dependencies (none required for basic operation)
RUN pip install --no-cache-dir -r requirements.txt 2>/dev/null || true

# Environment variables with defaults
ENV NTP_HOST=0.0.0.0 \
    NTP_PORT=123 \
    NTP_STRATUM=2 \
    NTP_REFERENCE_ID=LOCL \
    NTP_LOG_LEVEL=INFO \
    PYTHONUNBUFFERED=1

# Expose NTP port (UDP)
EXPOSE 123/udp

# Health check - verify the process is running
# Note: NTP uses UDP so we can't do a simple TCP health check
HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
    CMD pgrep -f "ntp_server.py" > /dev/null || exit 1

# Run as root to bind to privileged port 123
# For non-privileged ports (>1024), uncomment the USER line
# USER ntp

# Start the NTP server
ENTRYPOINT ["python", "-u", "ntp_server.py"]
