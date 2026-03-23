#!/usr/bin/env python3
import argparse
import csv
import queue
import socket
import statistics
import threading
import time
from pathlib import Path


def percentile(values, p):
    if not values:
        return 0.0
    ordered = sorted(values)
    idx = int(round((len(ordered) - 1) * p))
    return ordered[idx]


def build_request(mode, idx, payload_size):
    if mode == "ping":
        return b"ping\n", b"pong\n"

    prefix = f"client-{idx}-"
    body = (prefix + "x" * max(0, payload_size - len(prefix))).encode()
    if payload_size <= 0:
        body = b"x"
    return b"echo " + body + b"\n", body + b"\n"


def worker(host, port, mode, messages_per_client, payload_size, timeout, results, idx):
    payload, expected = build_request(mode, idx, payload_size)

    latencies_ms = []
    start = time.perf_counter()
    ok = 0

    try:
        sock = socket.create_connection((host, port), timeout=timeout)
        sock.settimeout(timeout)

        for _ in range(messages_per_client):
            t0 = time.perf_counter()
            sock.sendall(payload)

            remaining = len(expected)
            chunks = []
            while remaining > 0:
                data = sock.recv(remaining)
                if not data:
                    raise RuntimeError("connection closed early")
                chunks.append(data)
                remaining -= len(data)

            echoed = b"".join(chunks)
            if echoed != expected:
                raise RuntimeError(f"mismatch: {echoed!r} != {expected!r}")

            latencies_ms.append((time.perf_counter() - t0) * 1000.0)
            ok += 1

        sock.close()
        elapsed = time.perf_counter() - start
        results.put(("ok", idx, ok, elapsed, latencies_ms))
    except Exception as exc:
        elapsed = time.perf_counter() - start
        results.put(("err", idx, repr(exc), elapsed, latencies_ms))


def run_once(host, port, mode, clients, messages, size, timeout):
    results = queue.Queue()
    threads = []
    start = time.perf_counter()

    for i in range(clients):
        t = threading.Thread(
            target=worker,
            args=(host, port, mode, messages, size, timeout, results, i),
        )
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

    total_elapsed = time.perf_counter() - start
    ok_clients = 0
    err_clients = 0
    total_ok_messages = 0
    all_latencies = []
    failures = []

    while not results.empty():
        kind, idx, value, elapsed, latencies_ms = results.get()
        if kind == "ok":
            ok_clients += 1
            total_ok_messages += value
            all_latencies.extend(latencies_ms)
        else:
            err_clients += 1
            failures.append((idx, value, elapsed))
            all_latencies.extend(latencies_ms)

    throughput = total_ok_messages / total_elapsed if total_elapsed > 0 else 0.0

    return {
        "clients": clients,
        "mode": mode,
        "messages": messages,
        "size": size,
        "successful_clients": ok_clients,
        "failed_clients": err_clients,
        "successful_messages": total_ok_messages,
        "total_time_sec": total_elapsed,
        "throughput_msg_per_sec": throughput,
        "p50_ms": statistics.median(all_latencies) if all_latencies else 0.0,
        "p95_ms": percentile(all_latencies, 0.95),
        "p99_ms": percentile(all_latencies, 0.99),
        "failures": failures,
    }


def append_csv(path, row):
    file_exists = path.exists()
    with path.open("a", newline="") as f:
        writer = csv.writer(f)
        if not file_exists:
            writer.writerow(
                [
                    "clients",
                    "mode",
                    "messages",
                    "size",
                    "successful_clients",
                    "failed_clients",
                    "successful_messages",
                    "total_time_sec",
                    "throughput_msg_per_sec",
                    "p50_ms",
                    "p95_ms",
                    "p99_ms",
                ]
            )
        writer.writerow(
            [
                row["clients"],
                row["mode"],
                row["messages"],
                row["size"],
                row["successful_clients"],
                row["failed_clients"],
                row["successful_messages"],
                f"{row['total_time_sec']:.6f}",
                f"{row['throughput_msg_per_sec']:.2f}",
                f"{row['p50_ms']:.3f}",
                f"{row['p95_ms']:.3f}",
                f"{row['p99_ms']:.3f}",
            ]
        )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=10001)
    parser.add_argument("--mode", choices=["ping", "echo"], default="echo")
    parser.add_argument("--clients", type=int, default=100)
    parser.add_argument("--messages", type=int, default=100)
    parser.add_argument("--size", type=int, default=128)
    parser.add_argument("--timeout", type=float, default=5.0)
    parser.add_argument("--csv", default="bench.csv")
    args = parser.parse_args()

    result = run_once(
        args.host,
        args.port,
        args.mode,
        args.clients,
        args.messages,
        args.size,
        args.timeout,
    )

    print(f"clients={result['clients']}")
    print(f"mode={result['mode']}")
    print(f"messages={result['messages']}")
    print(f"size={result['size']}")
    print(f"successful_clients={result['successful_clients']}")
    print(f"failed_clients={result['failed_clients']}")
    print(f"successful_messages={result['successful_messages']}")
    print(f"total_time_sec={result['total_time_sec']:.6f}")
    print(f"throughput_msg_per_sec={result['throughput_msg_per_sec']:.2f}")
    print(f"p50_ms={result['p50_ms']:.3f}")
    print(f"p95_ms={result['p95_ms']:.3f}")
    print(f"p99_ms={result['p99_ms']:.3f}")

    if result["failures"]:
        print("failures:")
        for idx, err, elapsed in result["failures"][:20]:
            print(f"  client={idx} err={err} elapsed={elapsed:.3f}s")

    append_csv(Path(args.csv), result)

    if result["failed_clients"] > 0:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
