import serial
import struct
import time
from pylsl import StreamInfo, StreamOutlet

# ==========================================
# USER SETTINGS
# ==========================================

PORT = "..." #(COM1,2,3,4...)
BAUD = 500000
FS = 250
WATCHDOG_TIMEOUT = 3.0  # seconds before serial restart is triggered

# ==========================================

SYNC1 = 0xAA
SYNC2 = 0x55
PACKET_SIZE = 38

# ==========================================
# Create LSL Stream (created ONCE, never restarted)
# ==========================================

info = StreamInfo(
    name='ADS131M08',
    type='EEG',
    channel_count=8,
    nominal_srate=FS,
    channel_format='float32',
    source_id='ADS131M08_001'
)

desc = info.desc()
channels = desc.append_child("channels")

for ch in range(8):
    c = channels.append_child("channel")
    c.append_child_value("label", f"CH{ch+1}")
    c.append_child_value("type", "EEG")
    c.append_child_value("unit", "counts")

outlet = StreamOutlet(info)

print("=================================")
print("ADS131M08 -> LSL Bridge")
print("Waiting for packets...")
print("=================================")

# ==========================================
# Serial open/close helper
# ==========================================

def open_serial():
    ser = serial.Serial(PORT, BAUD, timeout=0.1)  # short timeout so watchdog check isn't blocked
    ser.reset_input_buffer()
    return ser

# ==========================================
# Runtime state
# ==========================================

expected_counter = None
lost_packets = 0
received_packets = 0
restart_count = 0

ser = open_serial()
last_packet_time = time.time()

# ==========================================
# Main loop
# ==========================================

while True:

    # --- Watchdog check ---
    if time.time() - last_packet_time > WATCHDOG_TIMEOUT:
        restart_count += 1
        print(
            f"[WATCHDOG] No packets for {WATCHDOG_TIMEOUT}s — "
            f"restarting serial (restart #{restart_count})"
        )
        try:
            ser.close()
        except Exception:
            pass
        time.sleep(0.5)
        try:
            ser = open_serial()
        except serial.SerialException as e:
            print(f"[WATCHDOG] Serial reopen failed: {e} — retrying in 2s")
            time.sleep(2.0)
            last_packet_time = time.time()  # reset so we don't spam restarts
            continue

        expected_counter = None  # counter will reset on Arduino after reconnect
        last_packet_time = time.time()
        print("[WATCHDOG] Serial reopened, resuming...")
        continue

    # --- Sync byte 1 ---
    b = ser.read(1)

    if len(b) == 0:
        continue  # timeout, loop back — watchdog will catch persistent silence

    if b[0] != SYNC1:
        continue

    # --- Sync byte 2 ---
    b = ser.read(1)

    if len(b) == 0:
        continue

    if b[0] != SYNC2:
        continue

    # --- Payload ---
    payload = ser.read(36)

    if len(payload) != 36:
        continue

    counter = struct.unpack_from('<I', payload, 0)[0]
    sample = struct.unpack_from('<8i', payload, 4)

    last_packet_time = time.time()  # valid packet received — reset watchdog

    # --- Packet loss detection ---
    if expected_counter is not None:
        if counter != expected_counter:
            missed = (counter - expected_counter) & 0xFFFFFFFF
            if missed < 100000:  # ignore large jumps after restart
                lost_packets += missed
                print(f"Lost {missed} packet(s) — expected {expected_counter}, got {counter}")
            else:
                print(f"Counter reset detected (expected {expected_counter}, got {counter})")

    expected_counter = (counter + 1) & 0xFFFFFFFF
    received_packets += 1

    # --- Send to LSL ---
    outlet.push_sample([float(x) for x in sample])

    # --- Statistics ---
    if received_packets % 1000 == 0:
        print(f"Packets={received_packets}  Lost={lost_packets}  Restarts={restart_count}")