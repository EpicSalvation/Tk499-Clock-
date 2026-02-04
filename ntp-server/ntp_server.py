#!/usr/bin/env python3
"""
Simple NTP Server Implementation (RFC 5905)

A lightweight NTP server designed for local network time synchronization.
Suitable for containerized deployments (Docker/Kubernetes).

Usage:
    python ntp_server.py [--host HOST] [--port PORT] [--stratum STRATUM]

Environment Variables:
    NTP_HOST     - Bind address (default: 0.0.0.0)
    NTP_PORT     - UDP port (default: 123)
    NTP_STRATUM  - Server stratum level (default: 2)
    NTP_LOG_LEVEL - Logging level (default: INFO)
"""

import argparse
import logging
import os
import signal
import socket
import struct
import sys
import time
from datetime import datetime
from typing import Optional, Tuple

# NTP epoch is January 1, 1900
NTP_EPOCH = datetime(1900, 1, 1)
UNIX_EPOCH = datetime(1970, 1, 1)
NTP_DELTA = (UNIX_EPOCH - NTP_EPOCH).total_seconds()  # 2208988800 seconds

# NTP packet format constants
NTP_VERSION = 4
MODE_CLIENT = 3
MODE_SERVER = 4

# Logging setup
def setup_logging(level: str = "INFO") -> logging.Logger:
    """Configure logging with the specified level."""
    log_level = getattr(logging, level.upper(), logging.INFO)
    logging.basicConfig(
        level=log_level,
        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S"
    )
    return logging.getLogger("ntp-server")


class NTPPacket:
    """
    NTP packet structure (48 bytes minimum).

    Format:
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |LI | VN  |Mode |    Stratum    |     Poll      |   Precision   |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         Root Delay                            |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         Root Dispersion                       |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                          Reference ID                         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                    Reference Timestamp (64)                   |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                    Origin Timestamp (64)                      |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                    Receive Timestamp (64)                     |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                    Transmit Timestamp (64)                    |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    """

    _FORMAT = "!B B B b I I 4s Q Q Q Q"  # Network byte order (big-endian)
    _SIZE = 48

    def __init__(self):
        self.leap_indicator: int = 0      # 2 bits: 0 = no warning
        self.version: int = NTP_VERSION   # 3 bits: NTP version
        self.mode: int = MODE_SERVER      # 3 bits: mode (server=4)
        self.stratum: int = 2             # 8 bits: stratum level
        self.poll: int = 6                # 8 bits: poll interval (2^6 = 64 sec)
        self.precision: int = -20         # 8 bits: precision (2^-20 sec ~= 1 usec)
        self.root_delay: int = 0          # 32 bits: round-trip delay to ref
        self.root_dispersion: int = 0     # 32 bits: dispersion to reference
        self.reference_id: bytes = b"LOCL"  # 32 bits: reference identifier
        self.reference_ts: int = 0        # 64 bits: last sync timestamp
        self.origin_ts: int = 0           # 64 bits: client's transmit timestamp
        self.receive_ts: int = 0          # 64 bits: server receive timestamp
        self.transmit_ts: int = 0         # 64 bits: server transmit timestamp

    @classmethod
    def from_bytes(cls, data: bytes) -> "NTPPacket":
        """Parse an NTP packet from raw bytes."""
        if len(data) < cls._SIZE:
            raise ValueError(f"NTP packet too short: {len(data)} < {cls._SIZE}")

        packet = cls()
        unpacked = struct.unpack(cls._FORMAT, data[:cls._SIZE])

        # First byte contains LI (2 bits), VN (3 bits), Mode (3 bits)
        first_byte = unpacked[0]
        packet.leap_indicator = (first_byte >> 6) & 0x03
        packet.version = (first_byte >> 3) & 0x07
        packet.mode = first_byte & 0x07

        packet.stratum = unpacked[1]
        packet.poll = unpacked[2]
        packet.precision = unpacked[3]
        packet.root_delay = unpacked[4]
        packet.root_dispersion = unpacked[5]
        packet.reference_id = unpacked[6]
        packet.reference_ts = unpacked[7]
        packet.origin_ts = unpacked[8]
        packet.receive_ts = unpacked[9]
        packet.transmit_ts = unpacked[10]

        return packet

    def to_bytes(self) -> bytes:
        """Serialize the NTP packet to bytes."""
        # Pack first byte: LI (2 bits) | VN (3 bits) | Mode (3 bits)
        first_byte = ((self.leap_indicator & 0x03) << 6 |
                      (self.version & 0x07) << 3 |
                      (self.mode & 0x07))

        return struct.pack(
            self._FORMAT,
            first_byte,
            self.stratum,
            self.poll,
            self.precision,
            self.root_delay,
            self.root_dispersion,
            self.reference_id[:4].ljust(4, b'\x00'),
            self.reference_ts,
            self.origin_ts,
            self.receive_ts,
            self.transmit_ts
        )


def unix_to_ntp_timestamp(unix_time: float) -> int:
    """
    Convert Unix timestamp to NTP 64-bit timestamp format.

    NTP timestamp: 32 bits for seconds since 1900, 32 bits for fraction.
    """
    ntp_seconds = unix_time + NTP_DELTA
    # Split into integer and fractional parts
    seconds = int(ntp_seconds)
    fraction = int((ntp_seconds - seconds) * (2**32))
    # Combine into 64-bit value
    return (seconds << 32) | fraction


def ntp_timestamp_to_unix(ntp_ts: int) -> float:
    """Convert NTP 64-bit timestamp to Unix timestamp."""
    seconds = (ntp_ts >> 32) & 0xFFFFFFFF
    fraction = ntp_ts & 0xFFFFFFFF
    return seconds - NTP_DELTA + (fraction / (2**32))


class NTPServer:
    """
    Simple NTP server that responds to client time requests.

    Attributes:
        host: IP address to bind to
        port: UDP port to listen on
        stratum: NTP stratum level (1-15)
        reference_id: 4-character reference identifier
    """

    def __init__(
        self,
        host: str = "0.0.0.0",
        port: int = 123,
        stratum: int = 2,
        reference_id: str = "LOCL",
        logger: Optional[logging.Logger] = None
    ):
        self.host = host
        self.port = port
        self.stratum = stratum
        self.reference_id = reference_id.encode()[:4].ljust(4, b'\x00')
        self.logger = logger or logging.getLogger("ntp-server")
        self.socket: Optional[socket.socket] = None
        self.running = False
        self._last_ref_time = time.time()

    def create_response(self, request: NTPPacket, receive_time: float) -> NTPPacket:
        """Create an NTP response packet for a client request."""
        response = NTPPacket()

        # Set response fields
        response.leap_indicator = 0  # No warning
        response.version = request.version if request.version in (3, 4) else NTP_VERSION
        response.mode = MODE_SERVER
        response.stratum = self.stratum
        response.poll = request.poll
        response.precision = -20  # ~1 microsecond
        response.root_delay = 0
        response.root_dispersion = 0
        response.reference_id = self.reference_id

        # Timestamps
        current_time = time.time()
        response.reference_ts = unix_to_ntp_timestamp(self._last_ref_time)
        response.origin_ts = request.transmit_ts  # Copy client's transmit time
        response.receive_ts = unix_to_ntp_timestamp(receive_time)
        response.transmit_ts = unix_to_ntp_timestamp(current_time)

        return response

    def handle_request(self, data: bytes, addr: Tuple[str, int], receive_time: float) -> Optional[bytes]:
        """Process an incoming NTP request and generate a response."""
        try:
            request = NTPPacket.from_bytes(data)

            # Only respond to client mode requests
            if request.mode != MODE_CLIENT:
                self.logger.debug(f"Ignoring non-client packet (mode={request.mode}) from {addr}")
                return None

            response = self.create_response(request, receive_time)

            self.logger.info(
                f"Served time to {addr[0]}:{addr[1]} - "
                f"v{request.version}, stratum={self.stratum}"
            )

            return response.to_bytes()

        except ValueError as e:
            self.logger.warning(f"Invalid packet from {addr}: {e}")
            return None
        except Exception as e:
            self.logger.error(f"Error handling request from {addr}: {e}")
            return None

    def start(self):
        """Start the NTP server."""
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        try:
            self.socket.bind((self.host, self.port))
        except PermissionError:
            self.logger.error(
                f"Permission denied binding to port {self.port}. "
                f"Try running with sudo or use a port > 1024."
            )
            sys.exit(1)
        except OSError as e:
            self.logger.error(f"Failed to bind to {self.host}:{self.port}: {e}")
            sys.exit(1)

        self.running = True
        self.logger.info(f"NTP server started on {self.host}:{self.port} (stratum {self.stratum})")

        while self.running:
            try:
                # Set timeout to allow graceful shutdown
                self.socket.settimeout(1.0)
                try:
                    data, addr = self.socket.recvfrom(1024)
                    receive_time = time.time()  # Record receive time immediately
                except socket.timeout:
                    continue

                response = self.handle_request(data, addr, receive_time)
                if response:
                    self.socket.sendto(response, addr)

            except Exception as e:
                if self.running:
                    self.logger.error(f"Error in main loop: {e}")

    def stop(self):
        """Stop the NTP server."""
        self.logger.info("Shutting down NTP server...")
        self.running = False
        if self.socket:
            self.socket.close()


def main():
    """Main entry point for the NTP server."""
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description="Simple NTP Server for local network time synchronization"
    )
    parser.add_argument(
        "--host",
        default=os.environ.get("NTP_HOST", "0.0.0.0"),
        help="IP address to bind to (default: 0.0.0.0)"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=int(os.environ.get("NTP_PORT", "123")),
        help="UDP port to listen on (default: 123)"
    )
    parser.add_argument(
        "--stratum",
        type=int,
        default=int(os.environ.get("NTP_STRATUM", "2")),
        help="NTP stratum level 1-15 (default: 2)"
    )
    parser.add_argument(
        "--reference-id",
        default=os.environ.get("NTP_REFERENCE_ID", "LOCL"),
        help="4-character reference identifier (default: LOCL)"
    )

    args = parser.parse_args()

    # Setup logging
    log_level = os.environ.get("NTP_LOG_LEVEL", "INFO")
    logger = setup_logging(log_level)

    # Validate stratum
    if not 1 <= args.stratum <= 15:
        logger.error("Stratum must be between 1 and 15")
        sys.exit(1)

    # Create and start server
    server = NTPServer(
        host=args.host,
        port=args.port,
        stratum=args.stratum,
        reference_id=args.reference_id,
        logger=logger
    )

    # Setup signal handlers for graceful shutdown
    def signal_handler(signum, frame):
        server.stop()

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    # Start serving
    server.start()


if __name__ == "__main__":
    main()
