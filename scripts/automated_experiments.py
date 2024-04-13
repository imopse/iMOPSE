import subprocess
import threading
import argparse

path_to_executable = "./imopse" # Input correct path

configurations_to_run = [
    "../../configurations/methods/NTGA2/NTGA2_ORIGINAL.cfg MSRCPSP_TA2 ../../configurations/problems/MSRCPSP/Regular/100_5_20_9_D3.def ../experiments/NTGA2/ 10",
    "../../configurations/methods/NSGAII/NSGAII_MSRCPSP.cfg MSRCPSP_TA2 ../../configurations/problems/MSRCPSP/Regular/100_5_20_9_D3.def ../experiments/NSGAII/ 10"
] # Input configurations to run

def run_executable(config_string, silent):
    command = f"{path_to_executable} {config_string}"

    print(f"Starting task for configuration '{config_string}'...")

    # Run the command and capture the output
    result = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    # Check if there was an error
    if result.returncode != 0 or not silent:
        print("\033[90m" + result.stdout.rstrip() + "\033[0m")

    if silent and result.returncode == 0:
        print("Output is suppressed due to silent mode.")

    print(f"Task for configuration '{config_string}' ended.")

def main(configurations, silent):
    threads = []

    for config in configurations:
        thread = threading.Thread(target=run_executable, args=(config, silent))
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run iMOPSE with configurations')
    parser.add_argument('-s', '--silent', action='store_true', help='Run in silent mode')
    args = parser.parse_args()

    main(configurations_to_run, args.silent)

