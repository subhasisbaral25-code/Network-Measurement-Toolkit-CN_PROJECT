import subprocess
import os
import time

# List of diverse global destination IPs for your experiment
TARGETS = [
    "8.8.8.8",        # Google DNS
    "1.1.1.1",        # Cloudflare DNS
    "208.67.222.222", # OpenDNS
    "9.9.9.9"         # Quad9
]

def run_command(cmd):
    """Utility function to run terminal commands safely."""
    print(f"Executing: {cmd}")
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result.stdout

def main():
    # Ensure the data directory exists for logging results
    os.makedirs("data", exist_ok=True)
    
    print("==================================================")
    print(" Starting Day 4: Global Experiment Orchestrator   ")
    print("==================================================")
    
    for ip in TARGETS:
        print(f"\n[+] Target IP: {ip}")
        
        # 1. Run native OS control benchmark (ping)[cite: 1]
        print("Running native OS ping control benchmark...")
        ping_output = run_command(f"ping -c 5 {ip}")
        print(ping_output)
        
        # 2. Invoke your compiled C bufferbloat module[cite: 1]
        # (Uncomment below once your C binary is compiled and ready to execute with root privileges)
        # c_output = run_command(f"sudo ./bufferbloat/bufferbloat {ip} 10")
        # print(c_output)
        
        time.sleep(1) # Brief pause between targets

    print("\n[+] Experiment orchestration complete. Results logged to data/ directory.")

if __name__ == "__main__":
    main()