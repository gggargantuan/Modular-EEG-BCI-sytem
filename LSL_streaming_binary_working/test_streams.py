from pylsl import resolve_streams

print("Searching...")

streams = resolve_streams()

for s in streams:
    print(
        "Name:", s.name(),
        "Type:", s.type(),
        "Channels:", s.channel_count(),
        "Rate:", s.nominal_srate()
    )