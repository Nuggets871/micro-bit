#!/usr/bin/env python3

import argparse
import json
import socket
import sys
from dataclasses import dataclass, asdict
from typing import Optional, Set, Tuple

try:
    import serial
except ImportError:
    print("pyserial manquant. Installer avec: pip install pyserial", file=sys.stderr)
    sys.exit(1)


DisplayOrder = str
ClientAddress = Tuple[str, int]


@dataclass
class SensorData:
    sequence: int
    temperature: int
    light: int
    humidity: int
    pressure: int


@dataclass
class ConfigAck:
    sequence: int
    order: DisplayOrder


def is_valid_display_order(order: str) -> bool:
    if not order or len(order) > 4:
        return False

    allowed = set("TLHP")
    seen = set()
    for char in order:
        if char not in allowed or char in seen:
            return False
        seen.add(char)

    return True


def parse_data_line(line: str) -> Optional[SensorData]:
    parts = line.split("|")
    if len(parts) != 6 or parts[0] != "DATA":
        return None

    try:
        return SensorData(
            sequence=int(parts[1]),
            temperature=int(parts[2]),
            light=int(parts[3]),
            humidity=int(parts[4]),
            pressure=int(parts[5]),
        )
    except ValueError:
        return None


def parse_config_ack_line(line: str) -> Optional[ConfigAck]:
    parts = line.split("|")
    if len(parts) != 3 or parts[0] != "CFG-ACK":
        return None

    try:
        sequence = int(parts[1])
    except ValueError:
        return None

    order = parts[2]
    if not is_valid_display_order(order):
        return None

    return ConfigAck(sequence=sequence, order=order)


class GatewayServer:
    def __init__(self, serial_port: str, baudrate: int, udp_port: int):
        self.serial = serial.Serial(serial_port, baudrate=baudrate, timeout=0.1)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("0.0.0.0", udp_port))
        self.sock.setblocking(False)
        self.latest_data: Optional[SensorData] = None
        self.latest_config_ack: Optional[ConfigAck] = None
        self.subscribers: Set[ClientAddress] = set()

    def send_json(self, payload: dict, addr: ClientAddress) -> None:
        self.sock.sendto(json.dumps(payload).encode("utf-8"), addr)

    def broadcast_json(self, payload: dict) -> None:
        dead = []
        for subscriber in self.subscribers:
            try:
                self.send_json(payload, subscriber)
            except OSError:
                dead.append(subscriber)

        for subscriber in dead:
            self.subscribers.discard(subscriber)

    def handle_serial_line(self, line: str) -> None:
        data = parse_data_line(line)
        if data is not None:
            self.latest_data = data
            self.broadcast_json({"type": "data", **asdict(data)})
            return

        config_ack = parse_config_ack_line(line)
        if config_ack is not None:
            self.latest_config_ack = config_ack
            self.broadcast_json({"type": "config_ack", **asdict(config_ack)})
            return

        if line.startswith("CFG-ERR|"):
            self.broadcast_json({"type": "config_error", "message": line})

    def send_config_to_microbit(self, order: str) -> None:
        self.serial.write(f"CFG|{order}\n".encode("utf-8"))

    def handle_udp_message(self, message: str, addr: ClientAddress) -> None:
        normalized = message.strip()

        if normalized in {"subscribe()", "SUBSCRIBE"}:
            self.subscribers.add(addr)
            self.send_json({"type": "subscribed"}, addr)
            return

        if normalized in {"unsubscribe()", "UNSUBSCRIBE"}:
            self.subscribers.discard(addr)
            self.send_json({"type": "unsubscribed"}, addr)
            return

        if normalized in {"getValues()", "GET_VALUES"}:
            if self.latest_data is None:
                self.send_json({"type": "error", "message": "no_data_yet"}, addr)
            else:
                self.send_json({"type": "data", **asdict(self.latest_data)}, addr)
            self.subscribers.add(addr)
            return

        if normalized in {"getStatus()", "GET_STATUS"}:
            payload = {
                "type": "status",
                "latest_data": asdict(self.latest_data) if self.latest_data else None,
                "latest_config_ack": asdict(self.latest_config_ack) if self.latest_config_ack else None,
                "subscribers": len(self.subscribers),
            }
            self.send_json(payload, addr)
            self.subscribers.add(addr)
            return

        if normalized.startswith("CFG|"):
            order = normalized[4:]
        else:
            order = normalized

        if is_valid_display_order(order):
            self.send_config_to_microbit(order)
            self.send_json({"type": "config_sent", "order": order}, addr)
            self.subscribers.add(addr)
            return

        self.send_json({"type": "error", "message": "unsupported_command"}, addr)

    def loop(self) -> None:
        print("Gateway server started")
        while True:
            line = self.serial.readline().decode("utf-8", errors="ignore").strip()
            if line:
                print(f"SERIAL> {line}")
                self.handle_serial_line(line)

            try:
                payload, addr = self.sock.recvfrom(4096)
            except BlockingIOError:
                continue

            message = payload.decode("utf-8", errors="ignore")
            print(f"UDP<{addr[0]}:{addr[1]}> {message.strip()}")
            self.handle_udp_message(message, addr)


def main() -> int:
    parser = argparse.ArgumentParser(description="UDP <-> UART gateway for the micro:bit IoT project")
    parser.add_argument("--serial-port", required=True, help="Serial device, e.g. /dev/cu.usbmodem14102")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--udp-port", type=int, default=10000)
    args = parser.parse_args()

    server = GatewayServer(args.serial_port, args.baudrate, args.udp_port)
    server.loop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())