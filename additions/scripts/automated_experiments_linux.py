import subprocess
import threading
import argparse

def run_executable(config_string, silent):
    command = f"./build/imopse {config_string}"
    print(f"Starting task for configuration '{config_string}'...")

    # Run the command and capture the output
    result = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    # Check if there was an error
    if result.returncode != 0 or not silent:
        # Print the output in grey color, strip trailing newlines
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
    parser = argparse.ArgumentParser(description='Run ./build/imopse with configurations')
    parser.add_argument('-s', '--silent', action='store_true', help='Run in silent mode')
    args = parser.parse_args()

    # Experiment configurations
    configurations = [
        "configurations/methods/GA/GA_MSRCPSP_2d.cfg MSRCPSP_TO_A configurations/problems/MSRCPSP/200_10_50_9.def experiments/MSRCPSP_TO/GA/ 10",
        "configurations/methods/DEGR/DE_MSRCPSP_2d.cfg MSRCPSP_TO_A configurations/problems/MSRCPSP/200_10_50_9.def experiments/MSRCPSP_TO/DE/ 10",
        "configurations/methods/SA/SA_MSRCPSP_2d.cfg MSRCPSP_TO_A configurations/problems/MSRCPSP/200_10_50_9.def experiments/MSRCPSP_TO/SA/ 10",
        "configurations/methods/TS/TS_MSRCPSP_2d.cfg MSRCPSP_TO_A configurations/problems/MSRCPSP/200_10_50_9.def experiments/MSRCPSP_TO/TS/ 10",
        "configurations/methods/PSO/PSO_MSRCPSP_2d.cfg MSRCPSP_TO_A configurations/problems/MSRCPSP/200_10_50_9.def experiments/MSRCPSP_TO/PSO/ 10",
    ]

    main(configurations, args.silent)