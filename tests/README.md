# Test Client

A Python test client for the game server sample.

Requires Python 3.7+ (standard library only — no pip install needed).

## Usage

Make sure the server is running first:

```bash
./build/game_server 7777
```

Then in another terminal:

### Single login
```bash
python3 tests/client.py login --account 1000
```

Expected: `LOGIN_ACK` with `user_id=1`.

### Echo
```bash
python3 tests/client.py echo --message "hello"
```

Expected: `ECHO_ACK` with the same message echoed back.

### Heartbeat
```bash
python3 tests/client.py heartbeat
```

Expected: `HEARTBEAT_ACK`.

### Timeout
```bash
python3 tests/client.py timeout --account 1000
```

Login then idle. The server should close the connection after roughly 30 seconds (heartbeat timeout).

### Duplicate login
```bash
python3 tests/client.py duplicate --account 1000
```

Two connections with the same `account_id`. First should succeed (`result=1`), second should be rejected (`result=0`).

### Interactive mode
```bash
python3 tests/client.py interactive
```

Manual commands:
- `login <account_id>` — send LOGIN_REQ
- `echo <message>` — send ECHO_REQ
- `heartbeat` — send HEARTBEAT_REQ
- `recv [timeout]` — try to receive a packet
- `quit` — close and exit

### Load test bot

Simulates N concurrent virtual users. Each logs in, then sends periodic `MOVE_REQ` packets; the server runs a brute-force AOI query per request and replies with the nearby-user count.

Run the server with the metrics build first so it prints per-second stats:

```bash
cmake --preset macos-debug-metrics
cmake --build --preset macos-debug-metrics
./build-metrics/game_server 7777
```

Then run the bot:

```bash
python3 tests/loadbot.py --conns 1000 --rate 5 --duration 60 --ramp 100
```

Bot options:
- `--conns <n>` — number of concurrent connections (default 100)
- `--rate <n>` — MOVE requests per second per user (default 5)
- `--duration <s>` — how long each user sends after connecting (default 60)
- `--ramp <n>` — connections opened per second, to avoid accept-backlog overflow (default 100; 0 = all at once)
- `--map <size>` — coordinate range for random moves (default 100)

The bot reports connection success, actual send rate, and round-trip latency percentiles. Compare its send rate against the server's `[stats]` output: if the server's `jobs/s` keeps up and the queue stays near zero, the server has headroom; a growing queue means the single logic thread is saturating.

## Options (client.py)

- `--host <ip>` — server host (default `127.0.0.1`)
- `--port <port>` — server port (default `7777`)