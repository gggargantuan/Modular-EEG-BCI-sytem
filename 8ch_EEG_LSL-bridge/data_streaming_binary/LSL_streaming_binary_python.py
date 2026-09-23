import serial
import struct
from pylsl import StreamInfo, StreamOutlet

# ==========================================
# USER SETTINGS
# ==========================================

PORT = "COM9" #change after checking!
BAUD = 500000
FS = 250

# ==========================================

SYNC1 = 0xAA
SYNC2 = 0x55

PACKET_SIZE = 38

# ==========================================

ser = serial.Serial(
    PORT,
    BAUD,
    timeout=0.05
)

# ==========================================
# Create LSL Stream
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

    c.append_child_value(
        "label",
        f"CH{ch+1}"
    )

    c.append_child_value(
        "type",
        "EEG"
    )

    c.append_child_value(
        "unit",
        "counts"
    )

outlet = StreamOutlet(info)

print("=================================")
print("ADS131M08 -> LSL Bridge")
print("Waiting for packets...")
print("=================================")

# ==========================================

expected_counter = None
lost_packets = 0
received_packets = 0

# ==========================================

while True:

    # Sync search

    b = ser.read(1)

    if len(b) == 0:
        continue

    if b[0] != SYNC1:
        continue

    b = ser.read(1)

    if len(b) == 0:
        continue

    if b[0] != SYNC2:
        continue

    payload = ser.read(36)

    if len(payload) != 36:
        continue

    counter = struct.unpack_from(
        '<I',
        payload,
        0
    )[0]

    sample = struct.unpack_from(
        '<8i',
        payload,
        4
    )

    # Packet loss detection

    if expected_counter is not None:

        if counter != expected_counter:

            missed = counter - expected_counter

            if missed > 0:

                lost_packets += missed

                print(
                    f"Lost {missed} packet(s)"
                )

    expected_counter = counter + 1

    received_packets += 1

    # Send to LSL

    outlet.push_sample(
        [float(x) for x in sample]
    )

    # Statistics

    if received_packets % 1000 == 0:

        print(
            f"Packets={received_packets}  Lost={lost_packets}"
        )