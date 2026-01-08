#!/usr/bin/env python3
"""
Probe BitTorrent DHT bootstrap nodes with a KRPC ping over UDP.
Reports nodes that respond within the timeout.
"""
import random
import socket
import struct
import time
from typing import List, Tuple

# Default hostnames (resolved to IPs before probing). These are commonly cited DHT bootstraps; verify locally.
BOOTSTRAP_HOSTS: List[Tuple[str, int]] = [
    ("router.bittorrent.com", 6881),
    ("dht.transmissionbt.com", 6881),
    ("router.utorrent.com", 6881),
    ("router.bitcomet.com", 6881),
    ("dht.libtorrent.org", 25401),
    ("v4.router.mega.co.nz", 6881),
    ("open.demonii.com", 1337),
    ("dht.aelitis.com", 6881),
    ("dht.wifi.pps.unizar.es", 6881),
    ("router.magnets.im", 6881),
    ("dht.ipv6tracker.org", 6881),
    ("dht.tfl.su", 6881),
]

# Optional static IPv4s (bypass DNS). Update with freshly verified seeds.
STATIC_IPS: List[Tuple[str, int]] = [
    ("67.215.246.10", 6881),   # router.bittorrent.com (historical)
    ("82.221.103.244", 6881),  # dht.transmissionbt.com (historical)
    ("212.83.137.66", 6881),   # community seed (verify)
    ("212.47.229.2", 6881),    # community seed (verify)
    ("5.79.83.193", 6881),     # community seed (verify)
]

TIMEOUT = 1.5  # seconds
RETRIES = 2


def random_node_id() -> bytes:
    return bytes(random.randint(0, 255) for _ in range(20))


def build_ping(txid: bytes, node_id: bytes) -> bytes:
    # d1:ad2:id20:<nodeid>e1:q4:ping1:t2:<txid>1:y1:qe
    return (
        b"d1:ad2:id20:" + node_id + b"e1:q4:ping1:t" + txid + b"1:y1:qe"
    )


def probe(addr: Tuple[str, int]) -> bool:
    node_id = random_node_id()
    txid = struct.pack("!H", random.randint(0, 0xFFFF))
    payload = build_ping(txid, node_id)

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.settimeout(TIMEOUT)
        for _ in range(RETRIES):
            try:
                s.sendto(payload, addr)
                data, _ = s.recvfrom(512)
                # Minimal check: response contains the same txid
                if txid in data:
                    return True
            except (socket.timeout, OSError):
                continue
    return False


def main():
    responsive = []

    addrs: List[Tuple[str, int]] = []

    # Resolve hostnames to IPv4 and add to list
    for host, port in BOOTSTRAP_HOSTS:
        try:
            infos = socket.getaddrinfo(host, port, socket.AF_INET, socket.SOCK_DGRAM)
            for info in infos:
                ip = info[4][0]
                addrs.append((ip, port))
        except socket.gaierror:
            # DNS failed; skip
            continue

    # Add static IPs
    addrs.extend(STATIC_IPS)

    # Deduplicate while preserving order
    seen = set()
    unique_addrs = []
    for ip, port in addrs:
        key = (ip, port)
        if key in seen:
            continue
        seen.add(key)
        unique_addrs.append((ip, port))

    print("Probing bootstrap nodes (IPv4, DNS-resolved + static)...")
    for ip, port in unique_addrs:
        start = time.time()
        ok = probe((ip, port))
        elapsed = (time.time() - start) * 1000
        status = "OK" if ok else "NO-REPLY"
        print(f"{ip}:{port:<5} {status:9} ({elapsed:5.0f} ms)")
        if ok:
            responsive.append((ip, port))

    print("\nResponsive nodes:")
    for host, port in responsive:
        print(f"  {host}:{port}")

    if not responsive:
        print("  (none)")


if __name__ == "__main__":
    main()
