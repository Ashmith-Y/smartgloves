"""
BLE receiver for SmartGlove ESP32.
Scans, connects to 'Glove1', and streams IMU notifications.

Install dependency first:
    pip install bleak
"""

import asyncio
import struct
import sys

from bleak import BleakClient, BleakScanner
from bleak.backends.device import BLEDevice

GLOVE_NAME = "Glove1"
SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
CHAR_UUID    = "beb5483e-36e1-4688-b7f5-ea07361b26a8"


def imu_notification_handler(sender, data: bytearray) -> None:
    if len(data) < 24:
        print(f"[WARN] Short packet ({len(data)} bytes), skipping")
        return
    ax, ay, az, gx, gy, gz = struct.unpack_from("<6f", data)
    print(
        f"Accel: {ax:8.3f}, {ay:8.3f}, {az:8.3f}  |  "
        f"Gyro: {gx:8.3f}, {gy:8.3f}, {gz:8.3f}"
    )


async def scan_and_report() -> BLEDevice | None:
    print("Scanning for BLE devices (5 s)...\n")

    # bleak 3.x discover() returns list[BLEDevice]; advertisement data
    # (including RSSI) comes via the detection callback.
    seen: dict[str, tuple[BLEDevice, int]] = {}

    def _on_detect(device: BLEDevice, adv) -> None:
        rssi = getattr(adv, "rssi", None) or getattr(device, "rssi", -999) or -999
        seen[device.address] = (device, rssi)

    scanner = BleakScanner(detection_callback=_on_detect)
    await scanner.start()
    await asyncio.sleep(5.0)
    await scanner.stop()

    if not seen:
        print("No devices found.")
        return None

    print(f"{'Address':<20}  {'RSSI':>5}  Name")
    print("-" * 60)
    target = None
    for device, rssi in sorted(seen.values(), key=lambda x: x[1], reverse=True):
        name = device.name or "(unknown)"
        print(f"{device.address:<20}  {rssi:>5}  {name}")
        if device.name == GLOVE_NAME:
            target = device
    print()

    return target


async def run() -> None:
    target = await scan_and_report()

    if target is None:
        print(f"'{GLOVE_NAME}' not found in scan. Is it powered on and advertising?")
        sys.exit(1)

    print(f"Found '{GLOVE_NAME}' at {target.address}. Connecting...")

    async with BleakClient(target) as client:
        if not client.is_connected:
            print("Connection failed.")
            sys.exit(1)

        print(f"Connected. Subscribing to characteristic {CHAR_UUID}\n")
        await client.start_notify(CHAR_UUID, imu_notification_handler)

        print("Streaming IMU data — press Ctrl+C to stop.\n")
        try:
            while True:
                await asyncio.sleep(1)
        except KeyboardInterrupt:
            print("\nStopping...")
        finally:
            await client.stop_notify(CHAR_UUID)

    print("Disconnected.")


if __name__ == "__main__":
    asyncio.run(run())
