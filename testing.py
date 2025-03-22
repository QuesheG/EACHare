import os
import platform
import subprocess
import shutil

network = [
        [0, 1, 0, 1, 0, 0, 0, 0], #node 0
        [1, 0, 1, 1, 0, 0, 0, 0], #node 1
        [0, 1, 0, 0, 1, 0, 0, 0], #node 2
        [1, 1, 0, 0, 1, 0, 0, 0], #node 3
        [0, 0, 1, 1, 0, 0, 1, 1], #node 4
        [0, 0, 0, 1, 0, 0, 1, 0], #node 5
        [0, 0, 0, 0, 1, 1, 0, 1], #node 6
        [0, 0, 0, 0, 1, 0, 1, 0] #node 7
    ]
f_peers = "peers.txt"

try: os.mkdir("testing")
except: pass
os.chdir("testing")

for i in range(8):
    try: os.mkdir("g0" + str(i))
    except: pass
    os.chdir("g0" + str(i))
    try: os.mkdir("files")
    except: pass
    for j in range(8):
        if network[i][j]:
            with open(f_peers, "a") as file:
                file.write("127.0.0.1:500" + str(j) + "\n")
    os.chdir("..")

os.chdir("..")
for i in range(8):
    if platform.system() == "Windows":
        shutil.copy(os.getcwd()+"/eachare.exe", os.getcwd()+"/testing/g0"+str(i))
        os.system("start ./testing/g0"+str(i)+"/eachare.exe 127.0.0.1:500"+str(i)+" ./testing/g0"+str(i)+"/peers.txt ./testing/g0"+str(i)+"/files")
    if platform.system() == "Linux":
        shutil.copy(os.getcwd()+"/eachare", os.getcwd()+"/testing/g0"+str(i))
        # os.system("start ./testing/g0"+str(i)+"/eachare.exe 127.0.0.1:500"+str(i)+" ./testing/g0"+str(i)+"/peers.txt ./testing/g0"+str(i)+"/files")
