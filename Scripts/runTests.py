import os
import subprocess
import re

import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit
from matplotlib import rcParams
from sklearn.metrics import r2_score

def cubic_curve(x, a, b, c, d):
    return a * np.array(x)**3 + b * np.array(x)**2 + c * np.array(x) + d

def adjusted_r_squared(y_true, y_pred, num_predictors):
    n = len(y_true)
    r_squared = r2_score(y_true, y_pred)
    return 1 - ((1 - r_squared) * (n - 1) / (n - num_predictors - 1))

def run_bash_command(command, y_values):
    try:
        # Run the bash command and capture the output
        result = subprocess.run(command, shell=True, check=True, text=True, capture_output=True)

        lines = result.stdout.splitlines()  
        avg_decoding_line = [line for line in lines if "Decode AVG" in line]  
        avg_ccs_line = [line for line in lines if "Clock Cycles AVG" in line]  
        stddev_line = [line for line in lines if "Standard Deviation" in line]  
        accuracy_line = [line for line in lines if "Correct/Total_Decodes" in line] 

        avg_decoding = re.findall(r'\d+\.\d+', avg_decoding_line[0])
        avg_ccs = re.findall(r'\d+\.\d+', avg_ccs_line[0])
        stddev = re.findall(r'\d+\.\d+', stddev_line[0])
        accuracy_n = re.findall(r'\d+', accuracy_line[0])

        y_values.append(float(avg_ccs[0]))

        accuracy = float(accuracy_n[0]) / float(accuracy_n[1])

        print(avg_decoding[0], end ="\t\t")
        print(avg_ccs[0], end ="\t")
        print(stddev[0], end ="\t")
        print(accuracy)

        # Print any errors, if they occur
        if result.stderr:
            print("Error output:")
            print(result.stderr)
    except subprocess.CalledProcessError as e:
        # Handle any errors raised by the command
        print(f"Error: Command '{e.cmd}' returned non-zero exit status {e.returncode}")

script_folder = os.path.join(os.getcwd())
d3_folder = os.path.join(os.getcwd(), "..", "Build", "D3")
d4_folder = os.path.join(os.getcwd(), "..", "Build", "D4")
d5_folder = os.path.join(os.getcwd(), "..", "Build", "D5")
d6_folder = os.path.join(os.getcwd(), "..", "Build", "D6")
d7_folder = os.path.join(os.getcwd(), "..", "Build", "D7")
d8_folder = os.path.join(os.getcwd(), "..", "Build", "D8")

x_values = [3, 4, 5, 6, 7, 8]
x_fit = np.linspace(min(x_values), max(x_values), 6)
y_values = []

print("--------------------------------------------------------------------------------------")
print("\t\t\tQUEKUF Result Evaluation")
print("--------------------------------------------------------------------------------------")
print("d\tAvg. Decoding time\tCCs\t\tStdDev (CCs)\tAccuracy")

# Tests with code distance 3
# Change folder
print("3", end ="\t")
os.chdir(d3_folder)
# Define command
command_path = "./QUEKUF QUEKUF.xclbin Decoder_dataset.txt"
# Run the command
run_bash_command(command_path, y_values)

# Tests with code distance 4
print("4", end ="\t")
os.chdir(d4_folder)
run_bash_command(command_path, y_values)

# Tests with code distance 5
print("5", end ="\t")
os.chdir(d5_folder)
run_bash_command(command_path, y_values)

# Tests with code distance 6
print("6", end ="\t")
os.chdir(d6_folder)
run_bash_command(command_path, y_values)

# Tests with code distance 7
print("7", end ="\t")
os.chdir(d7_folder)
run_bash_command(command_path, y_values)

# Tests with code distance 8
print("8", end ="\t")
os.chdir(d8_folder)
run_bash_command(command_path, y_values)

# print(y_values)
os.chdir(script_folder)

# Fit the cubic curve to the data
params_cubic, covariance_cubic = curve_fit(cubic_curve, x_values, y_values)
errors_cubic = np.sqrt(np.diag(covariance_cubic))

# Compute fitted y values 
y_fit_cubic = cubic_curve(x_values, *params_cubic)

# Compute Mean Square Error (MSE) 
mse_cubic = np.mean((y_values - y_fit_cubic)**2)
print(f"Mean Square Error (MSE) for Cubic Curve: {mse_cubic}")

r_squared = r2_score(y_values, y_fit_cubic)
print(f"r-squared for Cubic Curve: {r_squared}")

adjusted_r_squared_cubic = adjusted_r_squared(y_values, y_fit_cubic, len(params_cubic))
print(f"Adjusted r-squared for Cubic Curve: {adjusted_r_squared_cubic}")

# Plotting the data 
y_fit_cubic = cubic_curve(x_fit, *params_cubic)

fig, ax = plt.subplots(figsize=(8, 4))

ax.plot(x_fit, y_fit_cubic, linestyle='dotted', color='#d7191c', label='Fitted Curve')
ax.scatter(x_values, y_values, marker='o', color='#2c7bb6', label='CCs Measurements')

ax.set_xlabel('Code Distance', fontsize=11)
ax.set_ylabel('CCs for Decoding', fontsize=11)
ax.set_ylim(200, 4000)
ax.legend(fontsize=8, loc='upper center', bbox_to_anchor=(0.5, -0.15), ncol=3)

# Save the plot as a PDF file
plt.savefig('cc-scaling.pdf', bbox_inches='tight')
