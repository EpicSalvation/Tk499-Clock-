# Local NTP Server

A lightweight Python NTP server for local network time synchronization. Designed for containerized deployments (Docker/Kubernetes) to provide time services to IoT devices like the TK499 clock.

## Features

- RFC 5905 compliant NTP v4 server
- Zero external dependencies (Python standard library only)
- Configurable via environment variables or command-line arguments
- Docker and Kubernetes ready
- Minimal resource footprint (~32MB memory)

## Quick Start

### Run Locally (Development)

```bash
# Run on default port 123 (requires root/sudo)
sudo python3 ntp_server.py

# Run on non-privileged port
python3 ntp_server.py --port 8123

# With custom settings
python3 ntp_server.py --port 8123 --stratum 3 --reference-id LOCL
```

### Run with Docker

```bash
# Build the image
docker build -t ntp-server:latest .

# Run the container
docker run -d \
  --name ntp-server \
  -p 123:123/udp \
  ntp-server:latest

# Run on non-privileged port
docker run -d \
  --name ntp-server \
  -p 8123:8123/udp \
  -e NTP_PORT=8123 \
  ntp-server:latest
```

### Deploy to Kubernetes

```bash
# Using kustomize (recommended)
kubectl apply -k kubernetes/

# Or apply individual manifests
kubectl apply -f kubernetes/namespace.yaml
kubectl apply -f kubernetes/configmap.yaml
kubectl apply -f kubernetes/deployment.yaml
kubectl apply -f kubernetes/service.yaml

# Check deployment status
kubectl -n ntp-server get pods
kubectl -n ntp-server get services
```

## Configuration

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `NTP_HOST` | `0.0.0.0` | IP address to bind to |
| `NTP_PORT` | `123` | UDP port to listen on |
| `NTP_STRATUM` | `2` | NTP stratum level (1-15) |
| `NTP_REFERENCE_ID` | `LOCL` | 4-character reference identifier |
| `NTP_LOG_LEVEL` | `INFO` | Logging level (DEBUG, INFO, WARNING, ERROR) |

### Command-Line Arguments

```
usage: ntp_server.py [-h] [--host HOST] [--port PORT] [--stratum STRATUM]
                     [--reference-id REFERENCE_ID]

Simple NTP Server for local network time synchronization

options:
  -h, --help            show this help message and exit
  --host HOST           IP address to bind to (default: 0.0.0.0)
  --port PORT           UDP port to listen on (default: 123)
  --stratum STRATUM     NTP stratum level 1-15 (default: 2)
  --reference-id REFERENCE_ID
                        4-character reference identifier (default: LOCL)
```

## Testing

### Using ntpdate (Linux)

```bash
# Query the NTP server
ntpdate -q <server-ip>

# If using a non-standard port, use ntpdate with -u flag
ntpdate -q -u <server-ip>
```

### Using ntpdig/sntp

```bash
# Query server
sntp <server-ip>

# With custom port
sntp <server-ip>:8123
```

### Using Python

```python
import socket
import struct
import time

def query_ntp(host, port=123):
    """Simple NTP client query."""
    # Create NTP request packet
    packet = bytearray(48)
    packet[0] = 0x1B  # LI=0, VN=3, Mode=3 (client)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5)
    sock.sendto(packet, (host, port))
    data, _ = sock.recvfrom(48)
    sock.close()

    # Extract transmit timestamp (bytes 40-47)
    transmit_ts = struct.unpack('!Q', data[40:48])[0]
    seconds = (transmit_ts >> 32) - 2208988800  # Convert to Unix time
    print(f"Server time: {time.ctime(seconds)}")

query_ntp('localhost', 123)
```

## Kubernetes Deployment Notes

### Accessing from Local Network Devices

For IoT devices on your local network to reach the NTP server running in Kubernetes:

1. **NodePort Service**: The included `ntp-server-nodeport` service exposes the NTP server on a high port (30000-32767) on every cluster node.

   ```bash
   # Get the assigned NodePort
   kubectl -n ntp-server get svc ntp-server-nodeport
   ```

2. **LoadBalancer Service** (if available): Modify `service.yaml` to use `type: LoadBalancer` for cloud deployments.

3. **Host Network Mode**: For port 123 access, edit the deployment to use `hostNetwork: true`.

### Resource Requirements

The NTP server is lightweight:
- CPU: 10m request, 100m limit
- Memory: 32Mi request, 64Mi limit

## Integration with TK499 Clock

The TK499 clock currently uses HTTP-based time synchronization via the ESP8266 WiFi module. To use this NTP server instead:

### Option 1: Update ESP8266 Firmware

If you upgrade the ESP8266 firmware to v1.7.0+, you can use SNTP AT commands:

```
AT+CIPSNTPCFG=1,-5,"<ntp-server-ip>"
AT+CIPSNTPTIME?
```

### Option 2: Create HTTP Time Endpoint

You can run a simple HTTP server alongside or instead of NTP that returns time in the format the clock expects. The clock queries `worldclockapi.com/api/json/utc/now` for a JSON response.

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    NTP Client (TK499)                   │
└─────────────────────┬───────────────────────────────────┘
                      │ UDP Port 123
                      ▼
┌─────────────────────────────────────────────────────────┐
│              Kubernetes Service (NodePort)              │
│                   ntp-server-nodeport                   │
└─────────────────────┬───────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────┐
│                   NTP Server Pod                        │
│  ┌───────────────────────────────────────────────────┐  │
│  │              ntp_server.py                        │  │
│  │  - Receives NTP client requests                   │  │
│  │  - Returns current system time                    │  │
│  │  - RFC 5905 compliant                            │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## Troubleshooting

### Port 123 Permission Denied

On Linux, binding to ports below 1024 requires root privileges:

```bash
# Option 1: Run as root
sudo python3 ntp_server.py

# Option 2: Use a high port
python3 ntp_server.py --port 8123

# Option 3: Grant capability (Linux)
sudo setcap 'cap_net_bind_service=+ep' $(which python3)
```

### No Response from Server

1. Check firewall rules allow UDP port 123
2. Verify the server is running: `netstat -ulnp | grep 123`
3. Check logs: `docker logs ntp-server` or `kubectl -n ntp-server logs -l app.kubernetes.io/name=ntp-server`

### Time Drift

This server uses the host system's clock. For accurate time:
- Ensure the host/node is synced to an upstream NTP server
- Consider running `chronyd` or `ntpd` on the host

## License

Part of the TK499 Clock project.
