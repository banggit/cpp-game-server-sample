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

## Options

- `--host <ip>` — server host (default `127.0.0.1`)
- `--port <port>` — server port (default `7777`)