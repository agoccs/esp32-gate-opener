from pathlib import Path
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


# SETTINGS

SCRIPT_DIR = Path(__file__).resolve().parent

INPUT_FILE = SCRIPT_DIR / "signal_data.txt"
OUTPUT_DIR = SCRIPT_DIR / "rxb60_analysis"

# Exclude measurements, if there is any:
# (button, remote controller, measurement number)
EXCLUDED_MEASUREMENTS = {
    (2, 1, 1),
}

SIGNAL_CODES = [1781, 1782, 1783]

PROTOCOL_ID = 11
BIT_COUNT = 12

# Protocol 11:
# sync = {36, 1}
# 0 = {1, 2}
# 1 = {2, 1}
SYNC_FACTOR = 36


# HELPER FUNCTIONS

def to_binary(value, bit_count=12):
    """Convert a decimal value to a fixed-width binary string."""
    return format(int(value), f"0{bit_count}b")


def parse_data_line(line):
    """
    DATA sor feldolgozása.

    Formátum:
    DATA,button,remote,sample,value,bitlength,protocol,delay,raw1,raw2,...
    """

    fields = line.strip().split(",")

    if len(fields) < 9:
        return None

    if fields[0] != "DATA":
        return None

    try:
        button = int(fields[1])
        remote = int(fields[2])
        sample = int(fields[3])
        value = int(fields[4])
        bitlength = int(fields[5])
        protocol = int(fields[6])
        delay = int(fields[7])

        raw_timings = []

        for x in fields[8:]:
            if x.strip() == "":
                continue

            try:
                raw_timings.append(int(x))
            except ValueError:
                break

        return {
            "button": button,
            "remote": remote,
            "sample": sample,
            "value": value,
            "bitlength": bitlength,
            "protocol": protocol,
            "delay": delay,
            "raw_timings": raw_timings,
        }

    except ValueError:
        return None


def load_measurements():
    """Mérési DATA sorok betöltése."""

    if not INPUT_FILE.exists():
        raise FileNotFoundError(
            f"Nem található a mérési fájl:\n{INPUT_FILE}"
        )

    measurements = []

    with open(INPUT_FILE, "r", encoding="utf-8-sig") as f:

        for line in f:

            if line.startswith("DATA,"):

                measurement = parse_data_line(line)

                if measurement is not None:
                    measurements.append(measurement)

    return measurements


def mark_valid_measurements(measurements):
    """Hibás mérések megjelölése."""

    for measurement in measurements:

        key = (
            measurement["button"],
            measurement["remote"],
            measurement["sample"],
        )

        measurement["valid"] = key not in EXCLUDED_MEASUREMENTS

    return measurements


# PROCESSING RAW DATA

def extract_data_timings(measurement):
    """
    Protocol 11 raw adatstruktúra.

    RCSwitch receiveProtocol():

        timings[0] = hosszú sync szakasz
        timings[1] = sync másik része
        timings[2:] = adat

    12 bit esetén:

        timings[2:26] = 24 időérték
                     = 12 db (high, low) pár

    """

    raw_timings = measurement["raw_timings"]

    if len(raw_timings) < 26:
        return None

    data_timings = raw_timings[2:26]

    if len(data_timings) != 24:
        return None

    return data_timings


def classify_duration(duration, delay):
    """
    Classify a pulse duration as short or long.

    For Protocol 11:
        short ≈ 1 × delay
        long  ≈ 2 × delay
    """

    distance_short = abs(duration - delay)
    distance_long = abs(duration - 2 * delay)

    if distance_short <= distance_long:
        return "short"

    return "long"

def valid_protocol_pair(first, second, delay, tolerance=0.60):
    """
    Pulse pair verification according to Protocol 11.

    0 = 1T + 2T
    1 = 2T + 1T

    Only pairs falling within the protocol's tolerance window are accepted.
    """

    tol = delay * tolerance

    zero_valid = (
        abs(first - delay) < tol
        and abs(second - 2 * delay) < tol
    )

    one_valid = (
        abs(first - 2 * delay) < tol
        and abs(second - delay) < tol
    )

    if zero_valid:
        return 0, first, second

    if one_valid:
        return 1, second, first

    return None

def decode_measured_bits(data_timings, delay):
    """
    A tényleges mért impulzuspárokból bitminta előállítása.

    Protocol 11:

        0 = short + long
        1 = long + short
    """

    bits = []
    pairs = []

    for i in range(0, len(data_timings), 2):

        first = data_timings[i]
        second = data_timings[i + 1]

        first_type = classify_duration(first, delay)
        second_type = classify_duration(second, delay)

        pairs.append(
            (
                first,
                second,
                first_type,
                second_type,
            )
        )

        if first_type == "short" and second_type == "long":
            bits.append("0")

        elif first_type == "long" and second_type == "short":
            bits.append("1")

        else:
            bits.append("?")

    return "".join(bits), pairs


def analyze_timing_data(measurements):
    """
    Minden méréshez hozzáadja a tényleges short/long
    impulzusokat és a biteket.
    """

    for m in measurements:

        raw_timings = m["raw_timings"]

        if len(raw_timings) >= 26:

            # RCSwitch Protocol 11 delay:
            # raw_timings[0] / 36
            measured_delay = raw_timings[0] / SYNC_FACTOR

            m["calculated_delay"] = measured_delay

            data_timings = extract_data_timings(m)

            m["data_timings"] = data_timings

            decoded_bits, pairs = decode_measured_bits(
                data_timings,
                measured_delay
            )

            m["measured_binary"] = decoded_bits

            m["pairs"] = pairs

            short_values = []
            long_values = []

            for pair in pairs:

                t1, t2, type1, type2 = pair

                if type1 == "short":
                    short_values.append(t1)
                else:
                    long_values.append(t1)

                if type2 == "short":
                    short_values.append(t2)
                else:
                    long_values.append(t2)

            m["short_values"] = short_values
            m["long_values"] = long_values

        else:

            m["calculated_delay"] = np.nan
            m["data_timings"] = None
            m["measured_binary"] = None
            m["pairs"] = []
            m["short_values"] = []
            m["long_values"] = []

    return measurements


# CLEAN CSV

def save_clean_csv(measurements):

    rows = []

    for m in measurements:

        rows.append({
            "button": m["button"],
            "remote": m["remote"],
            "sample": m["sample"],
            "value": m["value"],
            "binary": to_binary(m["value"], BIT_COUNT),
            "bitlength": m["bitlength"],
            "protocol": m["protocol"],
            "delay_us": m["delay"],
            "calculated_delay_us": m["calculated_delay"],
            "valid": m["valid"],
            "measured_binary": m["measured_binary"],
        })

    df = pd.DataFrame(rows)

    path = OUTPUT_DIR / "measurements_clean.csv"

    df.to_csv(
        path,
        index=False,
        encoding="utf-8-sig"
    )

    return path


# OPENING CODES

def analyze_codes(measurements):

    valid = [
        m for m in measurements
        if m["valid"]
    ]

    print()
    print("=" * 70)
    print("OPENING_CODES")
    print("=" * 70)

    for code in SIGNAL_CODES:

        group = [
            m for m in valid
            if m["value"] == code
        ]

        delays = [
            m["delay"]
            for m in group
        ]

        buttons = sorted(
            set(m["button"] for m in group)
        )

        remotes = sorted(
            set(m["remote"] for m in group)
        )

        print()
        print(f"Kód: {code}")
        print(f"Bináris: {to_binary(code)}")
        print(f"Mérések: {len(group)}")

        if delays:

            print(
                f"Delay: "
                f"{np.mean(delays):.2f} ± "
                f"{np.std(delays, ddof=1):.2f} µs"
            )

            print(
                f"Delay tartomány: "
                f"{min(delays)}–{max(delays)} µs"
            )

        print(
            f"Gomb címkék: "
            f"{', '.join(map(str, buttons))}"
        )

        print(
            f"Távirányítók: "
            f"{', '.join(map(str, remotes))}"
        )


# BUTTON -> CODE CONSISTENCY

def print_code_consistency(measurements):

    valid = [
        m for m in measurements
        if m["valid"]
    ]

    print()
    print("=" * 70)
    print("BUTTON -> CODE CONSISTENCY")
    print("=" * 70)

    rows = []

    for button in sorted(
        set(m["button"] for m in valid)
    ):

        group = [
            m for m in valid
            if m["button"] == button
        ]

        counts = (
            pd.Series(
                [m["value"] for m in group]
            )
            .value_counts()
            .sort_index()
        )

        for value, count in counts.items():

            rows.append({
                "button": button,
                "value": value,
                "count": count,
            })

    df = pd.DataFrame(rows)

    print(df.to_string(index=False))

    print()
    print("Várható fő hozzárendelés:")

    for button, code in zip(
        [1, 2, 3],
        SIGNAL_CODES
    ):

        print(
            f"Gomb {button} → "
            f"{code} "
            f"({to_binary(code)})"
        )


# DELAY STATISTICS

def calculate_delay_statistics(measurements):

    valid = [
        m for m in measurements
        if m["valid"]
    ]

    rows = []

    for code in SIGNAL_CODES:

        for remote_id in [1, 2]:

            group = [
                m for m in valid
                if m["value"] == code
                and m["remote"] == remote_id
            ]

            delays = [
                m["delay"]
                for m in group
            ]

            if not delays:
                continue

            rows.append({
                "code": code,
                "remote": remote_id,
                "n": len(delays),
                "mean": np.mean(delays),
                "std": (
                    np.std(delays, ddof=1)
                    if len(delays) > 1
                    else 0
                ),
                "median": np.median(delays),
                "min": np.min(delays),
                "max": np.max(delays),
            })

    return pd.DataFrame(rows)


# DELAY FIGURE

def plot_delay(measurements):

    df = calculate_delay_statistics(measurements)

    if df.empty:
        return

    labels = [
    f"Code {row.code}\nRemote {row.remote}"
    for _, row in df.iterrows()
    ]

    means = df["mean"].values
    stds = df["std"].values

    x = np.arange(len(labels))

    plt.figure(figsize=(12, 7))

    plt.errorbar(
        x,
        means,
        yerr=stds,
        fmt="o",
        capsize=7,
        markersize=8,
        linewidth=2,
    )

    plt.xticks(
        x,
        labels
    )

    plt.ylabel("Delay [µs]")
    plt.xlabel("Signal code and remote")

    plt.title(
        "Delay values: mean and standard deviation"
    )

    plt.grid(
        axis="y",
        alpha=0.25
    )

    plt.tight_layout()

    path = OUTPUT_DIR / "01_delay_distribution.png"

    plt.savefig(
        path,
        dpi=200
    )

    plt.close()

    print(f"Figure saved: {path}")


# SHORT / LONG IMPULSE STATISTICS

def calculate_pulse_statistics(measurements):

    valid = [m for m in measurements if m["valid"]]

    statistics = {}

    for code in SIGNAL_CODES:

        short_values = []
        long_values = []
        rejected_pairs = 0

        group = [
            m for m in valid
            if m["value"] == code
        ]

        for m in group:

            data = m["data_timings"]

            if data is None:
                continue

            delay = m["calculated_delay"]

            for i in range(0, 24, 2):

                first = data[i]
                second = data[i + 1]

                decoded = valid_protocol_pair(
                    first,
                    second,
                    delay
                )

                if decoded is None:
                    rejected_pairs += 1
                    continue

                bit, short_value, long_value = decoded

                short_values.append(short_value)
                long_values.append(long_value)

        def stats(values):

            if len(values) == 0:
                return None

            return {
                "n": len(values),
                "mean": np.mean(values),
                "std": np.std(values, ddof=1),
                "median": np.median(values),
                "min": np.min(values),
                "max": np.max(values),
            }

        statistics[code] = {
            "short": stats(short_values),
            "long": stats(long_values),
            "rejected": rejected_pairs,
        }

    return statistics


# SHORT / LONG FIGURE

def plot_short_long(measurements):

    stats = calculate_pulse_statistics(measurements)

    x = np.arange(len(SIGNAL_CODES))

    short_mean = []
    short_std = []

    long_mean = []
    long_std = []

    for code in SIGNAL_CODES:

        short_mean.append(
            stats[code]["short"]["mean"]
        )

        short_std.append(
            stats[code]["short"]["std"]
        )

        long_mean.append(
            stats[code]["long"]["mean"]
        )

        long_std.append(
            stats[code]["long"]["std"]
        )

    plt.figure(figsize=(10, 6))

    offset = 0.08

    plt.errorbar(
        x - offset,
        short_mean,
        yerr=short_std,
        fmt="o",
        capsize=7,
        markersize=8,
        linewidth=2,
        label="Short pulse"
    )

    plt.errorbar(
        x + offset,
        long_mean,
        yerr=long_std,
        fmt="o",
        capsize=7,
        markersize=8,
        linewidth=2,
        label="Long pulse"
    )

    plt.xticks(
        x,
        [str(c) for c in SIGNAL_CODES]
    )

    plt.xlabel("Signal code")
    plt.ylabel("Pulse duration [µs]")

    plt.title("Short and long pulse duration")

    plt.grid(
        axis="y",
        alpha=0.25
    )

    plt.legend()

    plt.tight_layout()

    path = OUTPUT_DIR / "02_short_long_pulses.png"

    plt.savefig(path, dpi=200)
    plt.close()

    print(f"Figure mentve: {path}")

    return stats


# TIPICAL FRAME TIMING

def plot_typical_frames(measurements):

    valid_measurements = [
        m for m in measurements
        if m["valid"]
        and m["data_timings"] is not None
        and len(m["data_timings"]) == 24
    ]

    fig, axes = plt.subplots(
        3,
        1,
        figsize=(14, 9),
        sharex=True
    )

    for ax, code in zip(axes, SIGNAL_CODES):

        group = [
            m for m in valid_measurements
            if m["value"] == code
        ]

        if not group:
            continue


        timing_matrix = np.array([
            m["data_timings"]
            for m in group
        ])

        median_timings = np.median(
            timing_matrix,
            axis=0
        )


        edges = np.concatenate(
            ([0], np.cumsum(median_timings))
        )

        # Protocol 11 inverted:
        levels = np.array([
            i % 2
            for i in range(24)
        ])

        ax.stairs(
            levels,
            edges,
            linewidth=2
        )


        bits = to_binary(code)

        for bit_index in range(12):

            start = edges[2 * bit_index]
            end = edges[2 * bit_index + 2]

            center = (start + end) / 2

            if bit_index > 0:
                ax.axvline(
                    start,
                    linestyle="--",
                    alpha=0.25,
                    linewidth=1
                )

            ax.text(
                center,
                1.12,
                bits[bit_index],
                ha="center",
                va="bottom",
                fontsize=13,
                fontweight="bold"
            )

            ax.text(
                center,
                -0.12,
                str(bit_index + 1),
                ha="center",
                va="top",
                fontsize=9
            )


        ax.set_ylim(
            -0.25,
            1.35
        )

        ax.set_yticks([0, 1])

        ax.set_ylabel("Logic level")

        ax.set_title(f"{code} = {bits}")

        ax.grid(
            axis="x",
            alpha=0.15
        )

    axes[-1].set_xlabel(
        "Time [µs]"
    )

    fig.suptitle("Typical signal frame timing")

    plt.tight_layout()

    path = OUTPUT_DIR / "03_typical_frames.png"

    plt.savefig(
        path,
        dpi=200,
        bbox_inches="tight"
    )

    plt.close()

    print(
        f"Figure saved: {path}"
    )


# BIT PATTERN

def plot_bit_patterns(measurements):

    bits_matrix = np.array([
        [
            int(bit)
            for bit in to_binary(code)
        ]
        for code in SIGNAL_CODES
    ])

    fig, ax = plt.subplots(
        figsize=(12, 4.5)
    )


    from matplotlib.colors import ListedColormap, BoundaryNorm

    cmap = ListedColormap([
        "#0B1F4B",   # 0
        "#F2C300"    # 1
    ])

    norm = BoundaryNorm(
        [-0.5, 0.5, 1.5],
        cmap.N
    )

    ax.imshow(
        bits_matrix,
        cmap=cmap,
        norm=norm,
        aspect="auto"
    )



    for row in range(3):

        for col in range(12):

            bit = bits_matrix[row, col]

            text_color = (
                "white"
                if bit == 0
                else "black"
            )

            ax.text(
                col,
                row,
                str(bit),
                ha="center",
                va="center",
                fontsize=16,
                fontweight="bold",
                color=text_color
            )

    ax.set_xticks(
        np.arange(12)
    )

    ax.set_xticklabels(
        [
            f"{i}"
            for i in range(1, 13)
        ]
    )

    ax.set_yticks(
        np.arange(3)
    )

    ax.set_yticklabels(
        [
            str(code)
            for code in SIGNAL_CODES
        ]
    )

    ax.set_xlabel("Bit position")

    ax.set_ylabel("Signal code")

    ax.set_title("Measured 12-bit signal patterns")


    ax.set_xticks(
        np.arange(-0.5, 12, 1),
        minor=True
    )

    ax.set_yticks(
        np.arange(-0.5, 3, 1),
        minor=True
    )

    ax.grid(
        which="minor",
        color="white",
        linewidth=2
    )

    ax.tick_params(
        which="minor",
        bottom=False,
        left=False
    )

    plt.tight_layout()

    path = OUTPUT_DIR / "04_bit_patterns.png"

    plt.savefig(
        path,
        dpi=200,
        bbox_inches="tight"
    )

    plt.close()

    print(
        f"Figure saved: {path}"
    )


# PROTOCOL SUMMARY

def create_protocol_summary(
    measurements,
    pulse_stats
):

    valid = [
        m for m in measurements
        if m["valid"]
    ]

    delays = [
        m["delay"]
        for m in valid
    ]

    mean_delay = np.mean(delays)

    rows = [
        ["Érvényes mérések", len(valid)],
        ["Kizárt mérések", len(measurements) - len(valid)],
        ["Protokoll", PROTOCOL_ID],
        ["Bit hossz", BIT_COUNT],
        ["Közös delay átlaga [µs]", mean_delay],
        ["Bit 0 kódolása", "rövid + hosszú"],
        ["Bit 1 kódolása", "hosszú + rövid"],
        ["1781 bináris", to_binary(1781)],
        ["1782 bináris", to_binary(1782)],
        ["1783 bináris", to_binary(1783)],
    ]

    for code in SIGNAL_CODES:

        short = pulse_stats[code]["short"]
        long = pulse_stats[code]["long"]

        ratio = (
            long["mean"]
            /
            short["mean"]
        )

        rows.extend([
            [
                f"{code} short pulse mean [µs]",
                short["mean"]
            ],
            [
                f"{code} short pulse standard deviation [µs]",
                short["std"]
            ],
            [
                f"{code} short pulse median [µs]",
                short["median"]
            ],
            [
                f"{code} long pulse mean [µs]",
                long["mean"]
            ],
            [
                f"{code} long pulse standard deviation [µs]",
                long["std"]
            ],
            [
                f"{code} long pulse median [µs]",
                long["median"]
            ],
            [
                f"{code} long/short ratio",
                ratio
            ],
        ])

    summary = pd.DataFrame(
        rows,
        columns=["parameter", "value"]
    )

    path = OUTPUT_DIR / "protocol_summary.csv"

    summary.to_csv(
        path,
        index=False,
        encoding="utf-8-sig"
    )

    return path


# KONZOL STATISTICS

def print_pulse_statistics(pulse_stats):

    print()
    print("=" * 70)
    print("IMPULZUS TIMING")
    print("=" * 70)

    for code in SIGNAL_CODES:

        short = pulse_stats[code]["short"]
        long = pulse_stats[code]["long"]
        rejected = pulse_stats[code]["rejected"]

        print()
        print(f"Code: {code}")

        print("  Short pulse:")
        print(
            f"    N:       {short['n']}"
        )
        print(
            f"    Mean:   {short['mean']:.2f} µs"
        )
        print(
            f"    Standard deviation:  {short['std']:.2f} µs"
        )
        print(
            f"    Median:  {short['median']:.2f} µs"
        )
        print(
            f"    Range: "
            f"{short['min']:.0f}–"
            f"{short['max']:.0f} µs"
        )

        print("  Long pulse:")
        print(
            f"    N:       {long['n']}"
        )
        print(
            f"    Mean:   {long['mean']:.2f} µs"
        )
        print(
            f"    Standard deviation:  {long['std']:.2f} µs"
        )
        print(
            f"    Median:  {long['median']:.2f} µs"
        )
        print(
            f"    Range: "
            f"{long['min']:.0f}–"
            f"{long['max']:.0f} µs"
        )

        ratio = (
            long["mean"]
            /
            short["mean"]
        )

        print(
            f"  Long/short ratio: "
            f"{ratio:.3f}"
        )

        print(
            f"  Rejected pulse pairs: "
            f"{rejected}"
        )

    print()
    print("Bit 0: short + long")
    print("Bit 1: long + short")


# MAIN

def main():

    OUTPUT_DIR.mkdir(
        exist_ok=True
    )

    print("=" * 70)
    print("RXB60 / ESP32 RF MEASUREMENT DATA ANALYSIS")
    print("=" * 70)


    measurements = load_measurements()

    print()
    print(
        f"DATA length: "
        f"{len(measurements)}"
    )



    measurements = mark_valid_measurements(
        measurements
    )

    excluded_count = sum(
        not m["valid"]
        for m in measurements
    )

    print(
        f"Excluded measurement: "
        f"{excluded_count}"
    )


    measurements = analyze_timing_data(
        measurements
    )


    csv_path = save_clean_csv(
        measurements
    )

    print()
    print(
        f"CSV saved: {csv_path}"
    )

    # CODES
    analyze_codes(
        measurements
    )

    print_code_consistency(
        measurements
    )


    pulse_stats = plot_short_long(
       measurements
    )

    print_pulse_statistics(
      pulse_stats
    )



    plot_delay(
      measurements
    )

    plot_typical_frames(
      measurements
    )

    plot_bit_patterns(
        measurements
    )


    # SUMMARY CSV

    summary_path = create_protocol_summary(
        measurements,
        pulse_stats
    )

    print()
    print(
        f"Protocol summary saved: "
        f"{summary_path}"
    )

    # FINAL SUMMARY

    valid_count = sum(
        m["valid"]
        for m in measurements
    )

    print()
    print("=" * 70)
    print("FINAL SUMMARY")
    print("=" * 70)

    print(
        f"Valid measurement: {valid_count}"
    )

    for code in SIGNAL_CODES:

        count = sum(
            m["valid"] and
            m["value"] == code
            for m in measurements
        )

        print(
            f"{code} = "
            f"{to_binary(code)} "
            f"({count} measurement)"
        )

    print()
    print(
        "Final analysis:"
    )

    print(
        f"  {OUTPUT_DIR}"
    )

    print()
    print(
        "Done."
    )


if __name__ == "__main__":
    main()