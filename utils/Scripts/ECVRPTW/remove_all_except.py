import os
import csv

OUTPUT_PATH_ROOT = ""
KEEP = "NTGA2_ALNS"

for path, subdirectories, files in os.walk(OUTPUT_PATH_ROOT):
   for subdirectory in subdirectories:
      with open(os.path.join(OUTPUT_PATH_ROOT, subdirectory, 'config.txt'), mode='w', newline='') as resultsFile:        
         resultsFile.writelines(["C:/Users/adria/source/repos/iMOPSE_public/utils/DemoApp/results/NTGA2_ALNS"])