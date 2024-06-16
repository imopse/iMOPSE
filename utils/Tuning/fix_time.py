import os
import numpy as np

OUTPUT_PATH_ROOT = "C:/Users/adria/source/repos/iMOPSE_public/utils/DemoApp/results/MOEAD/info"
SKIP=1

for path, subdirectories, files in os.walk(OUTPUT_PATH_ROOT):
   for subdirectory in subdirectories:
      times = open(os.path.join(OUTPUT_PATH_ROOT, subdirectory, 'time.txt')).readline().strip()[:-1].split(';')
      #open(os.path.join(OUTPUT_PATH_ROOT, subdirectory, 'time.txt'), mode='w').writelines([';'.join(times[30*SKIP:30*(SKIP+1)]) + ';'])
      info = open(os.path.join(OUTPUT_PATH_ROOT, subdirectory, 'info.txt')).readlines()
      print(times)
      info[2] = np.average(np.array(times, dtype=np.float32))
      print(info)
      open(os.path.join(OUTPUT_PATH_ROOT, subdirectory, 'info.txt'), mode='w').writelines([info[0], info[1], str(info[2])])