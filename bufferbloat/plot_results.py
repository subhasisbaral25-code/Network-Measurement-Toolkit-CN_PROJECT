import matplotlib.pyplot as plt
import pandas as pd
import os

def plot_bufferbloat():
    csv_path = "data/bufferbloat_results.csv"
    
    # Ensure the CSV results file exists before attempting to plot
    if not os.path.exists(csv_path):
        print(f"[-] Data file {csv_path} not found. Please run your experiment first.")
        return

    # Load logged CSV metrics into a DataFrame
    df = pd.read_csv(csv_path, header=None, names=["Target", "Idle_RTT", "Loaded_RTT", "Delta"])
    
    plt.figure(figsize=(10, 6))
    
    # Generate comparative bar chart for Idle RTT vs Loaded RTT
    x = range(len(df))
    plt.bar([i - 0.2 for i in x], df["Idle_RTT"], width=0.4, label="Idle RTT (ms)", color="skyblue")
    plt.bar([i + 0.2 for i in x], df["Loaded_RTT"], width=0.4, label="Loaded RTT (ms)", color="salmon")
    
    plt.xlabel("Global Destination IPs")
    plt.ylabel("Round Trip Time (ms)")
    plt.title("Bufferbloat Analysis: Idle RTT vs Saturated Load RTT")
    plt.xticks(x, df["Target"], rotation=25, ha="right")
    plt.legend()
    plt.tight_layout()
    
    # Save the output visualization plot to the data directory
    os.makedirs("data", exist_ok=True)
    plot_path = "data/bufferbloat_comparison.png"
    plt.savefig(plot_path)
    print(f"[+] Plot successfully generated and saved to {plot_path}")
    plt.show()

if __name__ == "__main__":
    plot_bufferbloat()