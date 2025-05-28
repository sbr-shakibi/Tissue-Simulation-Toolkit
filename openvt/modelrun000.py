import json
import subprocess
import os

stdout=subprocess.PIPE

def main():
    # Path to the JSON config and executable (assumed to be in the same directory)
    base_dir = os.path.dirname(os.path.abspath(__file__))
    json_path = os.path.join(base_dir, "input.json")
    executable_path = os.path.join(base_dir, "../bin/openvt-persistentrandomwalk-model000-tst")

    # Read JSON file
    try:
        with open(json_path, 'r') as f:
            config = json.load(f)
    except FileNotFoundError:
        print(f"Error: Config file not found at {json_path}")
        return
    except json.JSONDecodeError as e:
        print(f"Error: Failed to parse JSON config. {e}")
        return

    # Get number of simulations
    try:
        num_sims = config["sim"]["num_sims"]
    except KeyError:
        print("Error: 'num_sims' not found in JSON config under ['sim']['num_sims']")
        return

    # Run the executable for each simulation ID
    for sim_id in range(num_sims):
        print(f"Running simulation {sim_id + 1}/{num_sims} with ID {sim_id}...")
        try:
            subprocess.run(
                [executable_path, "openvt-persistentrandomwalk-model000.par", str(sim_id), "--platform minimal"], # argv[1] = parameterfile, argv[2] = id
                check=True
            )
        except subprocess.CalledProcessError as e:
            print(f"Simulation {sim_id} failed: {e}")
            continue

if __name__ == "__main__":
    main()
