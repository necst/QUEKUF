from math import ceil

import numpy as np
from utils import toric_code_x_logicals, toric_code_x_stabilisers
import pymatching
import sys


def toDec(syn):
    return sum(syn[i] * 2 ** i for i in range(len(syn)))


np.set_printoptions(linewidth = 1000)
np.set_printoptions(threshold = sys.maxsize)

if len(sys.argv) != 4:
	print("Usage: {} D p [T|F]".format(sys.argv[0]))
	sys.exit(1)

DecoderClass = pymatching.Matching
L = int(sys.argv[1])
error_rate = float(sys.argv[2])
amount = 200
parity = toric_code_x_stabilisers(L)
logicals = toric_code_x_logicals(L)
if sys.argv[3] == "T":
    f = open('Decoder_Dataset.txt', 'w')
else:
    f = open('Real_Decoder_Dataset.txt', 'w')
oldSTDout = sys.stdout
sys.stdout = f
print(logicals.todense())
syndec=[]

avgSynCount = 0
i = 0
while i < 200:
    noise = np.random.binomial(1, error_rate, 2 * L * L)
    syndrome = parity @ noise % 2
    if(sys.argv[3] == "T"):
        if(np.any(syndrome)):
            avgSynCount += (syndrome == 1).sum()
            i = i + 1
            print(syndrome)
            actuals = logicals @ noise % 2
            print(actuals)
    else:
        print(syndrome)
        actuals = logicals @ noise % 2
        print(actuals)
        i = i + 1

sys.stdout = oldSTDout

print("Recommended amount of grow units is " + str(ceil(avgSynCount/i)))




