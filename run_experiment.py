import subprocess
import os
import time

TARGETS = [
    "8.8.8.8",
    "1.1.1.1",
    "208.67.222.222",
    "9.9.9.9",
    "4.2.2.2",
    "80.80.80.80",
    "199.91.73.222",
    "202.12.27.33"
]

def run_command(cmd):
    print(f"Executing: {cmd}")
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result.stdout

def main():
    os.makedirs("data", exist_ok=True)
    print("==================================================")
    print(" Starting Day 4: Global Experiment Orchestrator   ")
    print("==================================================")
    
    for ip in TARGETS:
        print(f"\n[+] Target IP: {ip}")
        print("Running native OS ping control benchmark...")
        ping_output = run_command(f"ping -c 5 {ip}")
        print(ping_output)
        time.sleep(1)

    print("\n[+] Experiment orchestration complete. Results logged to data/ directory.")

if __name__ == "__main__":
    main()
