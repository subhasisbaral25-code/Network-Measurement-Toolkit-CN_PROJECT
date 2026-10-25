import csv
import os
import matplotlib.pyplot as plt
import numpy as np

def generate_bufferbloat_graph():
    csv_path = os.path.join("data", "bufferbloat_results.csv")
    out_png = os.path.join("data", "bufferbloat_comparison.png")

    if not os.path.exists(csv_path):
        print(f"[!] Error: {csv_path} not found. Run run_experiment.py first.")
        return

    destinations = []
    idle_rtts = []
    loaded_rtts = []

    with open(csv_path, mode="r", newline="") as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            destinations.append(row["Destination"])
            idle_rtts.append(float(row["Idle_RTT_ms"]))
            loaded_rtts.append(float(row["Loaded_RTT_ms"]))

    if not destinations:
        print("[!] Error: CSV file is empty.")
        return

    x = np.arange(len(destinations))
    width = 0.36

    fig, ax = plt.subplots(figsize=(11, 5.5))
    ax.bar(x - width / 2, idle_rtts, width, label="Idle RTT (ms)", color="skyblue")
    ax.bar(x + width / 2, loaded_rtts, width, label="Loaded RTT (ms)", color="salmon")

    ax.set_ylabel("Round Trip Time (ms)")
    ax.set_title("10-Destination Bufferbloat Comparison (Idle vs. Loaded RTT)")
    ax.set_xticks(x)
    ax.set_xticklabels(destinations, rotation=25, ha="right")
    ax.legend(loc="upper left")
    ax.grid(axis="y", linestyle="--", alpha=0.5)

    os.makedirs("data", exist_ok=True)
    plt.tight_layout()
    plt.savefig(out_png, dpi=150)
    plt.close()

    print(f"[+] Graph successfully generated and saved to: {out_png}")

if __name__ == "__main__":
    generate_bufferbloat_graph()