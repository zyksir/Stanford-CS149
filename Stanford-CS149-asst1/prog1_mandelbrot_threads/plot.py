import os
import matplotlib.pyplot as plt

def analyze(filename):
    result = []
    with open(filename) as f:
        for line in f:
            if "speedup" not in line:
                continue
            line = line.strip().strip("(").strip(")").split()
            speedup = float(line[0][:-1])
            threadNum = int(line[3])
            result.append((threadNum, speedup))
    result.append((1, 1))  
    result = sorted(result)
    X, Y = zip(*result)
    return X, Y

for filename in ["output_view1", "output_view2"]:
    X, Y = analyze(filename)
    plt.plot(X, Y, label=filename.split("_")[-1])
plt.title('SpeedUp', fontsize=20)
plt.xlabel('Number of Threads')
plt.ylabel('speedup')
plt.grid(True)
# plt.show()
plt.legend()
plt.savefig("result.png")
