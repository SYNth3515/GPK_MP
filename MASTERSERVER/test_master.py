#!/usr/bin/env python3
"""Emulates game server + game client the way the scripts do, against a running master."""
import socket, struct, sys, time
sys.path.insert(0, ".")
from master_server import build_packet, parse_packet, crc16

HOST, PORT = (sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"), int(sys.argv[2]) if len(sys.argv) > 2 else 29029
VER = 104

def sock(bind_port=0):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("0.0.0.0", bind_port)); s.settimeout(2); return s

def msg(*parts): return build_packet(bytes(parts[0]) + b"".join(parts[1:]))

# 1) two "game servers" register (port = their source port)
srv = [sock(31001), sock(31002)]
srv[0].sendto(msg([13, 3, VER]), (HOST, PORT))
srv[1].sendto(msg([13, 3, VER]), (HOST, PORT))
srv[1].sendto(msg([13, 2, VER]), (HOST, PORT))   # heartbeat: must not create a duplicate entry
time.sleep(0.3)

# 2) client connects exactly like FG_ConnectToMasterServer
c = sock()
c.sendto(msg([13, 0, VER]), (HOST, PORT))
p = parse_packet(c.recvfrom(1024)[0]); assert p[0] == 13 and p[1] == 0, p
count = p[2] | p[3] << 8 | p[4] << 16
print("count =", count)

# 3) GETSERVERS like FG_MasterServer_GetServersFrameTimer / FG_MasterServer_GetServers
loaded, found = 0, []
while True:
    c.sendto(msg([13, 1], struct.pack("<I", loaded)[:3]), (HOST, PORT))
    p = parse_packet(c.recvfrom(1024)[0])
    assert p[0] == 13 and p[1] == 1
    last = p[2] | p[3] << 8 | p[4] << 16
    n = p[5]
    assert last == loaded
    if n == 0: break
    for i in range(n):
        o = 6 + i * 6
        ip = ".".join(map(str, p[o:o+4])); port = struct.unpack(">H", p[o+4:o+6])[0]   # ntohs(NMGetWord)
        found.append((ip, port)); loaded += 1
c.sendto(msg([13, 5]), (HOST, PORT))
print("servers:", found)
assert sorted(p for _, p in found) == [31001, 31002], found

# 4) bad CRC is ignored
bad = bytearray(msg([13, 0, VER])); bad[0] ^= 0xFF
c.sendto(bytes(bad), (HOST, PORT))
c.settimeout(0.5)
try: c.recvfrom(1024); print("FAIL: answered bad crc"); sys.exit(1)
except socket.timeout: print("bad crc ignored: ok")

# 5) DELSERVER
srv[0].sendto(msg([13, 4]), (HOST, PORT)); time.sleep(0.3)
c.settimeout(2)
c.sendto(msg([13, 0, VER]), (HOST, PORT))
p = parse_packet(c.recvfrom(1024)[0]); print("count after del =", p[2]); assert p[2] == 1
# 6) wrong version client does not see servers
c.sendto(msg([13, 0, 1]), (HOST, PORT))
p = parse_packet(c.recvfrom(1024)[0]); print("count for other version =", p[2]); assert p[2] == 0
print("ALL OK")
