#!/usr/bin/env python3
"""MOVE 부하 봇 — asyncio, 표준 라이브러리만.
사용: python3 loadbot.py --conns 100 --rate 5 --duration 60 --map 100
"""
import argparse, asyncio, random, struct, time

LOGIN_REQ, LOGIN_ACK = 1001, 1002
MOVE_REQ,  MOVE_ACK  = 1007, 1008
HDR = 4  # opcode(2) + size(2), little-endian


def build(opcode, payload=b""):
    return struct.pack("<HH", opcode, len(payload)) + payload


# ── 봇 전역 통계 (단일 이벤트 루프라 락 불필요) ──
class Stats:
    def __init__(self):
        self.sent = 0; self.acked = 0
        self.connected = 0
        self.fail_connect = 0    # open_connection 단계 실패
        self.fail_login = 0      # 연결은 됐으나 로그인 실패
        self.lat = []


S = Stats()


async def read_packet(reader):
    """헤더 4B 읽고 payload까지 완성해서 (opcode, payload) 반환."""
    hdr = await reader.readexactly(HDR)
    opcode, size = struct.unpack("<HH", hdr)
    payload = await reader.readexactly(size) if size else b""
    return opcode, payload


async def virtual_user(idx, host, port, rate, duration, map_size, ramp):
    # 램프업: 초당 ramp명씩 접속하도록 시작 시점을 분산
    if ramp > 0:
        await asyncio.sleep(idx / ramp)   # idx번째 유저는 idx/ramp초 뒤 접속
    try:
        reader, writer = await asyncio.open_connection(host, port)
    except Exception:
        S.fail_connect += 1
        return

    try:
        # 로그인 (account_id = 1000 + idx, 중복 회피)
        writer.write(build(LOGIN_REQ, struct.pack("<Q", 1000 + idx)))
        await writer.drain()
        op, pl = await read_packet(reader)
        if op != LOGIN_ACK or not pl or pl[0] != 1:
            S.fail_login += 1    # ← 로그인 단계
            return
        S.connected += 1

        interval = 1.0 / rate
        deadline = time.monotonic() + duration
        while time.monotonic() < deadline:
            x, y = random.uniform(0, map_size), random.uniform(0, map_size)
            t0 = time.monotonic()
            writer.write(build(MOVE_REQ, struct.pack("<ff", x, y)))
            await writer.drain()
            S.sent += 1
            op, pl = await read_packet(reader)          # MOVE_ACK 대기
            if op == MOVE_ACK:
                S.acked += 1
                S.lat.append((time.monotonic() - t0) * 1000)  # ms
            await asyncio.sleep(interval)
    except (asyncio.IncompleteReadError, ConnectionError):
        pass                                             # 서버가 끊음 — 집계에만 반영
    finally:
        writer.close()


async def main():
    print("[bot] starting")
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=7777)
    ap.add_argument("--conns", type=int, default=100)
    ap.add_argument("--rate", type=float, default=5)
    ap.add_argument("--duration", type=int, default=60)
    ap.add_argument("--map", type=float, default=100)
    ap.add_argument("--ramp", type=float, default=100,
                        help="connections per second during ramp-up (0=all at once)")
    a = ap.parse_args()
    ramp_time = a.conns / a.ramp if a.ramp > 0 else 0

    print(f"[bot] {a.conns} conns, {a.rate}/s each, {a.duration}s, "
          f"map {a.map}, ramp {a.ramp}/s (~{ramp_time:.1f}s to full)")

    t0 = time.monotonic()
    await asyncio.gather(*[virtual_user(i, a.host, a.port, a.rate, a.duration, a.map, a.ramp)
                           for i in range(a.conns)])
    elapsed = time.monotonic() - t0

    target = a.conns * a.rate
    lat = sorted(S.lat)

    def pct(p):
        return lat[int(len(lat) * p)] if lat else 0.0

    print(f"\n=== bot result ===")
    print(f"connected={S.connected}/{a.conns} "
          f"fail_connect={S.fail_connect} fail_login={S.fail_login}")
    print(f"sent={S.sent} acked={S.acked} over {elapsed:.1f}s")
    print(f"send rate: {S.sent / elapsed:.0f}/s (target {target:.0f}/s)")
    print(f"RTT ms: p50={pct(.5):.2f} p95={pct(.95):.2f} p99={pct(.99):.2f}")


if __name__ == "__main__":
    asyncio.run(main())