#!/usr/bin/env python3
"""
Master server for Корсары ГПК multiplayer mod (net.dll protocol).

Protocol (reverse-engineered from net.dll + PROGRAM/GPK/net):
  UDP datagram = [CRC16 LE (2)] [flags/index word LE (2)] [payload...]
    CRC16 = CRC-16/ARC (poly 0xA001, init 0) over everything after the first 2 bytes
    flags/index: bit15 = guaranteed delivery, low 15 bits = packet index (0 for master traffic)
  payload = messages: [NC_MASTERSERVER=13][subcode][fields...]
    all integers little-endian, 24-bit = 3 bytes LE

  client -> master
    CONNECT      (0): [ver:byte]
    GETSERVERS   (1): [start:24]
    DISCONNECT   (5)
  game server -> master (port of the server = UDP source port of the datagram)
    ADDSERVER    (3): [ver:byte]
    UPDATESERVER (2): [ver:byte]   (ver optional, old scripts send nothing)
    DELSERVER    (4)
  master -> client
    CONNECT    reply: [13][0][count:24]
    GETSERVERS reply: [13][1][start:24][n:byte] n * ([ip:4 bytes a.b.c.d][port:2 bytes BIG-endian])

Only the Python standard library is used.
"""
import argparse
import asyncio
import logging
import signal
import socket
import struct
import time
from collections import defaultdict

NC_MASTERSERVER = 13
SUB_CONNECT, SUB_GETSERVERS, SUB_UPDATE, SUB_ADD, SUB_DEL, SUB_DISCONNECT = 0, 1, 2, 3, 4, 5

SERVER_TTL = 150          # sec without ADD/UPDATE -> server removed (game sends UPDATE every 60 s)
CLIENT_TTL = 120          # sec lifetime of a client's list snapshot
PAGE_SIZE = 20            # servers per GETSERVERS reply (reply stays ~130 bytes)
MAX_SERVERS_PER_IP = 8
MAX_SERVERS_TOTAL = 2000
MAX_PACKET = 1024         # game receives with a 1024 byte buffer
RATE_BURST = 40           # packets per source IP: bucket size
RATE_PER_SEC = 15         # packets per source IP: refill rate

log = logging.getLogger("master")


# --- CRC16 exactly as net.dll computes it (nibble table at 0x100980c8, CRC-16/ARC) -------------
_TAB = [0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
        0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400]


def crc16(data: bytes) -> int:
    crc = 0
    for b in data:
        t = _TAB[b & 0xF] ^ _TAB[crc & 0xF] ^ ((crc >> 4) & 0xFFF)
        crc = _TAB[b >> 4] ^ _TAB[t & 0xF] ^ ((t >> 4) & 0xFFF)
    return crc


def build_packet(payload: bytes) -> bytes:
    body = struct.pack("<H", 0) + payload          # flags/index = 0
    return struct.pack("<H", crc16(body)) + body


def parse_packet(data: bytes):
    if len(data) < 5 or len(data) > MAX_PACKET:
        return None
    crc, = struct.unpack_from("<H", data, 0)
    if crc != crc16(data[2:]):
        return None
    return data[4:]                                  # payload


class Reader:
    def __init__(self, b): self.b, self.i = b, 0
    def left(self): return len(self.b) - self.i
    def byte(self):
        v = self.b[self.i]; self.i += 1; return v
    def u24(self):
        v = self.b[self.i] | self.b[self.i + 1] << 8 | self.b[self.i + 2] << 16
        self.i += 3; return v


class Master(asyncio.DatagramProtocol):
    def __init__(self):
        self.servers = {}      # (ip, port) -> {"ver": int|None, "seen": float, "added": float}
        self.clients = {}      # (ip, port) -> {"list": [(ip, port)], "seen": float}
        self.buckets = defaultdict(lambda: [RATE_BURST, time.monotonic()])
        self.transport = None
        self.stats = defaultdict(int)

    # -- plumbing ------------------------------------------------------------------------------
    def connection_made(self, transport):
        self.transport = transport

    def send(self, addr, payload):
        self.transport.sendto(build_packet(payload), addr)

    def allowed(self, ip):
        b = self.buckets[ip]
        now = time.monotonic()
        b[0] = min(RATE_BURST, b[0] + (now - b[1]) * RATE_PER_SEC)
        b[1] = now
        if b[0] < 1:
            return False
        b[0] -= 1
        return True

    def datagram_received(self, data, addr):
        ip = addr[0]
        if not self.allowed(ip):
            self.stats["ratelimited"] += 1
            return
        payload = parse_packet(data)
        if payload is None:
            self.stats["bad_packets"] += 1
            return
        r = Reader(payload)
        try:
            while r.left() > 0:
                if r.byte() != NC_MASTERSERVER:
                    break                            # not ours, drop the rest
                self.handle(addr, r)
        except IndexError:
            self.stats["truncated"] += 1

    # -- handlers ------------------------------------------------------------------------------
    def handle(self, addr, r):
        sub = r.byte()
        if sub == SUB_CONNECT:
            ver = r.byte() if r.left() else None
            self.on_connect(addr, ver)
        elif sub == SUB_GETSERVERS:
            self.on_getservers(addr, r.u24())
        elif sub == SUB_DISCONNECT:
            self.clients.pop(addr, None)
        elif sub in (SUB_ADD, SUB_UPDATE):
            ver = r.byte() if r.left() else None
            self.on_add(addr, ver, sub == SUB_ADD)
        elif sub == SUB_DEL:
            if self.servers.pop(addr, None):
                log.info("server removed %s:%d", *addr)
        else:
            self.stats["unknown_sub"] += 1

    def visible_list(self, ver):
        return [a for a, s in sorted(self.servers.items(), key=lambda kv: kv[1]["added"])
                if ver is None or s["ver"] is None or s["ver"] == ver]

    def on_connect(self, addr, ver):
        lst = self.visible_list(ver)
        self.clients[addr] = {"list": lst, "seen": time.monotonic()}
        self.stats["connects"] += 1
        self.send(addr, bytes([NC_MASTERSERVER, SUB_CONNECT]) + struct.pack("<I", len(lst))[:3])
        log.debug("client %s:%d connect ver=%s -> %d servers", addr[0], addr[1], ver, len(lst))

    def on_getservers(self, addr, start):
        c = self.clients.get(addr)
        if c is None:                                # CONNECT lost - serve a fresh snapshot
            c = self.clients[addr] = {"list": self.visible_list(None), "seen": 0}
        c["seen"] = time.monotonic()
        chunk = c["list"][start:start + PAGE_SIZE]
        out = bytearray([NC_MASTERSERVER, SUB_GETSERVERS])
        out += struct.pack("<I", start)[:3]
        out.append(len(chunk))
        for ip, port in chunk:
            out += socket.inet_aton(ip) + struct.pack(">H", port)
        self.send(addr, bytes(out))
        self.stats["getservers"] += 1

    def on_add(self, addr, ver, is_add):
        now = time.monotonic()
        s = self.servers.get(addr)
        if s:
            s["seen"] = now
            if ver is not None:
                s["ver"] = ver
            return
        if sum(1 for a in self.servers if a[0] == addr[0]) >= MAX_SERVERS_PER_IP \
                or len(self.servers) >= MAX_SERVERS_TOTAL:
            self.stats["rejected_add"] += 1
            return
        self.servers[addr] = {"ver": ver, "seen": now, "added": now}
        log.info("server %s %s:%d ver=%s (total %d)",
                 "added" if is_add else "re-registered by update", addr[0], addr[1], ver, len(self.servers))

    # -- housekeeping --------------------------------------------------------------------------
    def cleanup(self):
        now = time.monotonic()
        for a in [a for a, s in self.servers.items() if now - s["seen"] > SERVER_TTL]:
            del self.servers[a]
            log.info("server expired %s:%d", *a)
        for a in [a for a, c in self.clients.items() if now - c["seen"] > CLIENT_TTL]:
            del self.clients[a]
        for ip in [ip for ip, b in self.buckets.items() if now - b[1] > 60]:
            del self.buckets[ip]

    def report(self):
        log.info("stats: servers=%d clients=%d %s", len(self.servers), len(self.clients), dict(self.stats))


async def main():
    ap = argparse.ArgumentParser(description="GPK multiplayer master server")
    ap.add_argument("--host", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=29029)
    ap.add_argument("--debug", action="store_true")
    args = ap.parse_args()
    logging.basicConfig(level=logging.DEBUG if args.debug else logging.INFO,
                        format="%(asctime)s %(levelname)s %(message)s")

    loop = asyncio.get_running_loop()
    master = Master()
    await loop.create_datagram_endpoint(lambda: master, local_addr=(args.host, args.port))
    log.info("master server listening on udp://%s:%d", args.host, args.port)

    stop = asyncio.Event()
    for sig in (signal.SIGINT, signal.SIGTERM):
        loop.add_signal_handler(sig, stop.set)

    tick = 0
    while not stop.is_set():
        try:
            await asyncio.wait_for(stop.wait(), timeout=10)
        except asyncio.TimeoutError:
            master.cleanup()
            tick += 1
            if tick % 30 == 0:
                master.report()
    log.info("shutting down")


if __name__ == "__main__":
    asyncio.run(main())
