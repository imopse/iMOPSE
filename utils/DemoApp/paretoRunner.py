import os
import subprocess as proc
from typing import *
import csv
import shutil

def __CreateFiles(outputDirectory):
   os.mkdir(os.path.join(outputDirectory, "config"))
   with open(os.path.join(outputDirectory, "config", "config.txt"), mode='w') as configFile:
      configFile.write(outputDirectory)

def RunPareto(outputDirectory):
   print("Starting pareto analyzer")
   __CreateFiles(outputDirectory)
   process = proc.Popen(["./resources/paretoAnalyzer.exe"
         , os.path.join(outputDirectory, "config", "config.txt")
         , ""
         , outputDirectory
         ]
      , stdout=proc.PIPE
      , stderr=proc.PIPE
   )

   process.wait()
   process.stdout.readline()
   tpfs: str = str(process.stdout.readline())
   qualities: List[str] = str(process.stdout.readline()).split(';')

   __MergeData(outputDirectory)

   with open(os.path.join(outputDirectory, "quality.txt"), mode='w') as qualityTxt:
      qualityTxt.write(tpfs.split(':')[1].split("\\r")[0] + '\n')
      qualityTxt.write(qualities[2].split(':')[1] + '\n')
      qualityTxt.write(qualities[4] .split(':')[1] + '\n')

   return process.returncode == 0

def __MergeData(outputDirectory):
   datas = []
   results = []
   for path, subdirs, files in os.walk(outputDirectory):
      for name in files:
         if name == "data.csv":
            with open(os.path.join(path, name), mode='r') as dataFile:
               dataCsv = csv.reader(dataFile, delimiter=';')
               for data in dataCsv:
                  datas.append(data)
            with open(os.path.join(path, "results.csv"), mode='r') as resultFile:
               resultsCsv = csv.reader(resultFile, delimiter=';')
               for result in resultsCsv:
                  results.append(result)
   indexesToRemove = []
   for i in range(0, len(results)):
      for j in range(i+1, len(results)):
         if results[i][0] == results[j][0] and results[i][1] == results[j][1]:
            print("are equal", results[i][0], results[i][1], "index", i)
            indexesToRemove.append(i)
   
   for index in sorted(indexesToRemove, reverse=True):
      del datas[index]
      del results[index]

   with open(os.path.join(outputDirectory, "data.csv"), mode='w', newline='') as dataFile:
      dataCsvWriter = csv.writer(dataFile, delimiter=';')
      for data in datas:
         dataCsvWriter.writerow(data)
   
   with open(os.path.join(outputDirectory, "results.csv"), mode='w', newline='') as resultsFile:
      resultsCsvWriter = csv.writer(resultsFile, delimiter=';')
      for result in results:
         resultsCsvWriter.writerow(result)

   shutil.copyfile(os.path.join(outputDirectory, "run_0", "points.csv"), os.path.join(outputDirectory, "points.csv"))
