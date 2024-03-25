import numpy as np
from utils import toric_code_x_logicals, toric_code_x_stabilisers
import pymatching
import sys


def toDec(syn):
    return sum(syn[i] * 2 ** i for i in range(len(syn)))

n = len(sys.argv)
if(n < 2):
    print("Correct usage: python3 datasetGen.py <Code Length>")
    exit()


np.set_printoptions(linewidth = 1000)
np.set_printoptions(threshold = sys.maxsize)

DecoderClass = pymatching.Matching
L = int(sys.argv[1])
error_rate = 0.1
amount = 200
parity = toric_code_x_stabilisers(L)
logicals = toric_code_x_logicals(L)
f = open('Decoder_dataset.txt', 'w')
sys.stdout = f
print(logicals.todense())
syndec=[]

for i in range(amount):
    noise = np.random.binomial(1, error_rate, 2 * L * L)
    syndrome = parity @ noise % 2
    if(np.any(syndrome)):
        print(syndrome)
        actuals = logicals @ noise % 2
        print(actuals)
    else:
        i = i - 1


