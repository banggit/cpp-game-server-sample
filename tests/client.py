#!/usr/bin/env python3
"""
Game Server Sample - Test Client

A simple Python test client for the C++ game server sample.
Uses only the standard library (no pip install required).

Usage:
    python3 client.py login --account 1000
    python3 client.py echo --message "hello"
    python3 client.py heartbeat
    python3 client.py timeout --account 1000
    python3 client.py duplicate --account 1000
    python3 client.py interactive
"""

import argparse
import socket
import struct
import sys
import time
from typing import Optional, Tuple


# ────────────────────────────────────────────
# Protocol constants
# ────────────────────────────────────────────
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 7777

PACKET_HEADER_SIZE = 4  # opcode (2B) + payload_size (2B)

# Packet IDs (must match src/common/Types.h)
LOGIN_REQ     = 1001
LOGIN_ACK     = 1002
ECHO_REQ      = 1003
ECHO_ACK      = 1004
HEARTBEAT_REQ = 1005
HEARTBEAT_ACK = 1006

OPCODE_NAMES = {
    LOGIN_REQ: "LOGIN_REQ",
    LOGIN_ACK: "LOGIN_ACK",
    ECHO_REQ: "ECHO_REQ",
    ECHO_ACK: "ECHO_ACK",
    HEARTBEAT_REQ: "HEARTBEAT_REQ",
    HEARTBEAT_ACK: "HEARTBEAT_ACK",
}


# ────────────────────────────────────────────
# Packet build / parse
# ────────────────────────────────────────────
def build_packet(opcode: int, payload: bytes = b"") -> bytes:
    """Build a packet: [opcode:2][size:2][payload:N] (little-endian)."""
    return struct.pack("<HH", opcode, len(payload)) + payload


def parse_packet(data: bytes) -> Optional[Tuple[int, bytes]]:
    """Parse a single packet from data. Returns (opcode, payload) or None."""
    if len(data) < PACKET_HEADER_SIZE:
        return None

    opcode, payload_size = struct.unpack("<HH", data[:PACKET_HEADER_SIZE])
    expected_size = PACKET_HEADER_SIZE + payload_size

    if len(data) < expected_size:
        return None

    payload = data[PACKET_HEADER_SIZE:expected_size]
    return opcode, payload


def opcode_name(opcode: int) -> str:
    return OPCODE_NAMES.get(opcode, f"UNKNOWN({opcode})")


# ────────────────────────────────────────────
# Connection helper
# ────────────────────────────────────────────
class Connection:
    """A simple TCP connection wrapper with packet send / receive."""

    def __init__(self, host: str, port: int, timeout: float = 5.0):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.sock: Optional[socket.socket] = None
        self.recv_buffer = b""

    def connect(self) -> None:
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(self.timeout)
        self.sock.connect((self.host, self.port))
        print(f"[connected] {self.host}:{self.port}")

    def close(self) -> None:
        if self.sock:
            try:
                self.sock.close()
            except OSError:
                pass
            self.sock = None
            print("[closed]")

    def send_packet(self, opcode: int, payload: bytes = b"") -> None:
        if not self.sock:
            raise RuntimeError("not connected")
        packet = build_packet(opcode, payload)
        self.sock.sendall(packet)
        print(f"[send] {opcode_name(opcode)} payload={payload.hex() or '(empty)'}")

    def recv_packet(self, timeout: Optional[float] = None) -> Optional[Tuple[int, bytes]]:
        """Receive one full packet. Returns None on timeout / disconnect."""
        if not self.sock:
            raise RuntimeError("not connected")

        if timeout is not None:
            self.sock.settimeout(timeout)

        # Read until we have a full packet in the buffer.
        while True:
            parsed = parse_packet(self.recv_buffer)
            if parsed is not None:
                opcode, payload = parsed
                consumed = PACKET_HEADER_SIZE + len(payload)
                self.recv_buffer = self.recv_buffer[consumed:]
                print(f"[recv] {opcode_name(opcode)} payload={payload.hex() or '(empty)'}")
                return opcode, payload

            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                print("[recv] timeout")
                return None

            if not chunk:
                print("[recv] connection closed by server")
                return None

            self.recv_buffer += chunk


# ────────────────────────────────────────────
# Scenarios
# ────────────────────────────────────────────
def scenario_login(host: str, port: int, account_id: int) -> None:
    """Send LOGIN_REQ and read LOGIN_ACK."""
    print(f"\n=== scenario: login (account_id={account_id}) ===")
    conn = Connection(host, port)
    try:
        conn.connect()
        payload = struct.pack("<Q", account_id)
        conn.send_packet(LOGIN_REQ, payload)

        result = conn.recv_packet()
        if result is None:
            print("[fail] no response")
            return

        opcode, payload = result
        if opcode != LOGIN_ACK or len(payload) < 9:
            print(f"[fail] unexpected response: opcode={opcode_name(opcode)}")
            return

        success = payload[0]
        user_id = struct.unpack("<Q", payload[1:9])[0]
        if success == 1:
            print(f"[ok] login success: user_id={user_id}")
        else:
            print(f"[fail] login rejected by server (user_id={user_id})")
    finally:
        conn.close()


def scenario_echo(host: str, port: int, message: str) -> None:
    """Send ECHO_REQ with message and verify ECHO_ACK matches."""
    print(f"\n=== scenario: echo (message='{message}') ===")
    conn = Connection(host, port)
    try:
        conn.connect()
        payload = message.encode("utf-8")
        conn.send_packet(ECHO_REQ, payload)

        result = conn.recv_packet()
        if result is None:
            print("[fail] no response")
            return

        opcode, recv_payload = result
        if opcode != ECHO_ACK:
            print(f"[fail] unexpected response: opcode={opcode_name(opcode)}")
            return

        if recv_payload == payload:
            print(f"[ok] echo matched: '{recv_payload.decode('utf-8', errors='replace')}'")
        else:
            print(f"[fail] echo mismatch: sent={payload!r} recv={recv_payload!r}")
    finally:
        conn.close()


def scenario_heartbeat(host: str, port: int) -> None:
    """Send HEARTBEAT_REQ and verify HEARTBEAT_ACK."""
    print("\n=== scenario: heartbeat ===")
    conn = Connection(host, port)
    try:
        conn.connect()
        conn.send_packet(HEARTBEAT_REQ)

        result = conn.recv_packet()
        if result is None:
            print("[fail] no response")
            return

        opcode, _ = result
        if opcode == HEARTBEAT_ACK:
            print("[ok] heartbeat ack received")
        else:
            print(f"[fail] unexpected response: opcode={opcode_name(opcode)}")
    finally:
        conn.close()


def scenario_timeout(host: str, port: int, account_id: int, wait_seconds: int = 60) -> None:
    """Login and then idle to verify server-side timeout disconnect."""
    print(f"\n=== scenario: timeout (account_id={account_id}, wait={wait_seconds}s) ===")
    print("Server should close the connection after the heartbeat timeout (default 30s).")
    conn = Connection(host, port, timeout=wait_seconds + 5)
    try:
        conn.connect()
        payload = struct.pack("<Q", account_id)
        conn.send_packet(LOGIN_REQ, payload)

        # Read login ack first.
        result = conn.recv_packet()
        if result is None:
            print("[fail] no login ack")
            return

        print(f"Now idling for up to {wait_seconds} seconds, waiting for server disconnect...")
        start = time.time()
        # Try to recv. The server should close the connection on timeout.
        result = conn.recv_packet(timeout=wait_seconds)
        elapsed = time.time() - start

        if result is None:
            print(f"[ok] connection closed by server after {elapsed:.1f}s")
        else:
            opcode, _ = result
            print(f"[fail] unexpected packet during idle: {opcode_name(opcode)}")
    finally:
        conn.close()


def scenario_duplicate(host: str, port: int, account_id: int) -> None:
    """Open two connections with the same account_id to verify duplicate rejection."""
    print(f"\n=== scenario: duplicate login (account_id={account_id}) ===")
    conn_a = Connection(host, port)
    conn_b = Connection(host, port)
    try:
        # First login should succeed.
        print("\n--- connection A (first login) ---")
        conn_a.connect()
        payload = struct.pack("<Q", account_id)
        conn_a.send_packet(LOGIN_REQ, payload)
        result_a = conn_a.recv_packet()
        if result_a is None:
            print("[fail] no response on conn A")
            return
        success_a = result_a[1][0] if len(result_a[1]) >= 1 else 0
        print(f"conn A login result={success_a} (expected 1)")

        # Second login with the same account_id should be rejected.
        print("\n--- connection B (duplicate login) ---")
        conn_b.connect()
        conn_b.send_packet(LOGIN_REQ, payload)
        result_b = conn_b.recv_packet()
        if result_b is None:
            print("[fail] no response on conn B")
            return
        success_b = result_b[1][0] if len(result_b[1]) >= 1 else 0
        print(f"conn B login result={success_b} (expected 0)")

        if success_a == 1 and success_b == 0:
            print("\n[ok] duplicate rejection works as expected")
        else:
            print("\n[fail] unexpected results")
    finally:
        conn_a.close()
        conn_b.close()


def scenario_interactive(host: str, port: int) -> None:
    """Interactive mode: pick packet types manually."""
    print("\n=== scenario: interactive ===")
    print("Commands:")
    print("  login <account_id>   - send LOGIN_REQ")
    print("  echo <message>       - send ECHO_REQ")
    print("  heartbeat            - send HEARTBEAT_REQ")
    print("  recv [timeout]       - try to receive a packet (default timeout 1s)")
    print("  quit                 - close connection and exit")

    conn = Connection(host, port)
    try:
        conn.connect()
        while True:
            try:
                line = input("> ").strip()
            except EOFError:
                break

            if not line:
                continue

            parts = line.split(maxsplit=1)
            cmd = parts[0].lower()
            arg = parts[1] if len(parts) > 1 else ""

            if cmd == "quit":
                break
            elif cmd == "login":
                try:
                    account_id = int(arg)
                except ValueError:
                    print("usage: login <account_id>")
                    continue
                conn.send_packet(LOGIN_REQ, struct.pack("<Q", account_id))
                conn.recv_packet(timeout=2.0)
            elif cmd == "echo":
                conn.send_packet(ECHO_REQ, arg.encode("utf-8"))
                conn.recv_packet(timeout=2.0)
            elif cmd == "heartbeat":
                conn.send_packet(HEARTBEAT_REQ)
                conn.recv_packet(timeout=2.0)
            elif cmd == "recv":
                try:
                    t = float(arg) if arg else 1.0
                except ValueError:
                    t = 1.0
                conn.recv_packet(timeout=t)
            else:
                print(f"unknown command: {cmd}")
    finally:
        conn.close()


# ────────────────────────────────────────────
# CLI
# ────────────────────────────────────────────
def main() -> int:
    parser = argparse.ArgumentParser(description="Game Server Sample - Test Client")
    parser.add_argument("--host", default=DEFAULT_HOST, help="server host (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="server port (default: 7777)")

    sub = parser.add_subparsers(dest="scenario", required=True)

    p_login = sub.add_parser("login", help="single login")
    p_login.add_argument("--account", type=int, default=1000, help="account_id")

    p_echo = sub.add_parser("echo", help="send echo packet")
    p_echo.add_argument("--message", default="hello", help="message to echo")

    sub.add_parser("heartbeat", help="send heartbeat")

    p_timeout = sub.add_parser("timeout", help="login then idle to test server timeout")
    p_timeout.add_argument("--account", type=int, default=1000, help="account_id")
    p_timeout.add_argument("--wait", type=int, default=60, help="seconds to wait")

    p_dup = sub.add_parser("duplicate", help="test duplicate login rejection")
    p_dup.add_argument("--account", type=int, default=1000, help="account_id")

    sub.add_parser("interactive", help="interactive mode")

    args = parser.parse_args()

    try:
        if args.scenario == "login":
            scenario_login(args.host, args.port, args.account)
        elif args.scenario == "echo":
            scenario_echo(args.host, args.port, args.message)
        elif args.scenario == "heartbeat":
            scenario_heartbeat(args.host, args.port)
        elif args.scenario == "timeout":
            scenario_timeout(args.host, args.port, args.account, args.wait)
        elif args.scenario == "duplicate":
            scenario_duplicate(args.host, args.port, args.account)
        elif args.scenario == "interactive":
            scenario_interactive(args.host, args.port)
    except ConnectionRefusedError:
        print(f"[error] connection refused — is the server running on {args.host}:{args.port}?")
        return 1
    except KeyboardInterrupt:
        print("\n[interrupted]")
        return 130

    return 0


if __name__ == "__main__":
    sys.exit(main())