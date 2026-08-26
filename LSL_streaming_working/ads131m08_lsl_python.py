# ads131m08_lsl.py

import serial
from pylsl import StreamInfo, StreamOutlet

PORT = "COM9"      # change this
BAUD = 500000
FS = 250

ser = serial.Serial(PORT, BAUD, timeout=1)

info = StreamInfo(
    name='ADS131M08',
    type='EEG',
    channel_count=8,
    nominal_srate=FS,
    channel_format='float32',
    source_id='ADS131M08_001'
)

channels = info.desc().append_child("channels")

for ch in range(8):
    channel = channels.append_child("channel")
    channel.append_child_value("label", f"CH{ch+1}")
    channel.append_child_value("unit", "counts")
    channel.append_child_value("type", "EEG")

outlet = StreamOutlet(info)

print("LSL stream created")
print("Waiting for ADS131M08 data...")

while True:
    try:
        line = ser.readline().decode("utf-8").strip()

        if not line:
            continue

        values = line.split(",")

        if len(values) != 8:
            continue

        sample = [float(v) for v in values]

        outlet.push_sample(sample)

    except KeyboardInterrupt:
        break

    except Exception:
        pass