import os
import subprocess
import csv
import re

DESTINATIONS = [
    "8.8.8.8",         # Google DNS
    "1.1.1.1",         # Cloudflare
    "208.67.222.222",  # OpenDNS
    "9.9.9.9",         # Quad9
    "4.2.2.2",         # Level3
    "80.80.80.80",     # Freenom
    "199.91.73.222",   # Verisign
    "202.12.27.33",    # APNIC
    "193.0.14.129",    # RIPE NCC
    "200.160.0.8"      # NIC.br
]

# Compile each teammate's untouched C file directly
MODULES = {
    "ping":        ["gcc", "ping.c", "-o", "ping_bin"],
    "traceroute":  ["gcc", "traceroute.c", "-o", "traceroute_bin"],
    "pmtu":        ["gcc", "pmtu.c", "-o", "pmtu_bin"],
    "bandwidth":   ["gcc", "bandwidth.c", "-o", "bandwidth_bin"],
    "bufferbloat": ["gcc", "new_bufferbloat.c", "-lpthread", "-o", "bufferbloat_bin"]
}

def compile_team_files():
    print("Compiling original team C files...")
    for name, cmd in MODULES.items():
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode == 0:
            print(f" [+] Compiled {cmd[1]} -> {cmd[-1]}")
        else:
            print(f" [!] Error compiling {cmd[1]}:\n{res.stderr}")

def run_binary(cmd, logfile):
    print(f"  -> Running: {' '.join(cmd)}")
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, timeout=90)
        out = res.stdout + "\n" + res.stderr
        logfile.write(f"\n--- {' '.join(cmd)} ---\n" + out)
        logfile.flush()
        return out
    except subprocess.TimeoutExpired:
        logfile.write(f"\n--- TIMEOUT: {' '.join(cmd)} ---\n")
        return ""

def main():
    os.makedirs("data", exist_ok=True)
    compile_team_files()

    csv_path = os.path.join("data", "bufferbloat_results.csv")
    log_path = os.path.join("data", "full_toolkit_experiment.log")

    with open(csv_path, "w", newline="") as csvfile, open(log_path, "w") as logfile:
        writer = csv.writer(csvfile)
        writer.writerow(["Destination", "Idle_RTT_ms", "Loaded_RTT_ms"])

        for idx, ip in enumerate(DESTINATIONS, 1):
            print(f"\n[{idx}/10] Testing Destination: {ip}")
            logfile.write(f"\n================ DESTINATION: {ip} ================\n")

            # 1. Call Rupen's ping.c binary directly
            run_binary(["sudo", "./ping_bin", ip], logfile)

            # 2. Call Subhasis's traceroute.c binary directly (ICMP and UDP modes)
            run_binary(["sudo", "./traceroute_bin", ip, "-i"], logfile)
            run_binary(["sudo", "./traceroute_bin", ip, "-u"], logfile)

            # 3. Call Diptalal's bandwidth.c binary directly
            run_binary(["sudo", "./bandwidth_bin", ip], logfile)

            # 4. Call Amlan's bufferbloat.c binary (which #includes pmtu.c and ping.c)
            bb_out = run_binary(["sudo", "./bufferbloat_bin", ip], logfile)

            idle_rtt, loaded_rtt = "0.00", "0.00"
            match = re.search(r"RESULT_CSV:[^,]+,([\d\.]+),([\d\.]+)", bb_out)
            if match:
                idle_rtt, loaded_rtt = match.group(1), match.group(2)

            writer.writerow([ip, idle_rtt, loaded_rtt])
            csvfile.flush()
            print(f"  [Result] {ip} -> Idle RTT: {idle_rtt} ms | Loaded RTT: {loaded_rtt} ms")

    if os.path.exists("new_plot_results.py"):
        subprocess.run(["python3", "new_plot_results.py"])

if __name__ == "__main__":
    main()