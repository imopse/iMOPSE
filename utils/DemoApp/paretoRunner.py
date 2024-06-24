import os
import subprocess as proc
from typing import *
import csv
import shutil
import fileManager as fm

def RunPareto(instanceName: str):
   print("Starting pareto analyzer..")
   outputDirectory=fm.GetParetoOutputFolder(instanceName)
   if not os.path.exists(outputDirectory):
      os.makedirs(outputDirectory)
   process = proc.Popen(["./resources/paretoAnalyzer.exe"
         , os.path.join(os.getcwd(), "config", instanceName, "config.txt").replace('\\', '/')
         , instanceName
         , outputDirectory
         ]
      , stdout=proc.PIPE
      , stderr=proc.PIPE
   )

   process.wait()
   
   with open(os.path.join(outputDirectory, "quality.txt"), mode='w') as qualityTxt:
      while True:
         line = process.stdout.readline()
         if not line:
            break
         qualityTxt.write(line.decode())

   with open(os.path.join(os.getcwd(), "config", instanceName, "config.txt"), mode='r') as configFile:
      for line in configFile.readlines():
         line = line.replace('\n', '')
         datas = []
         results = []
         for path, subdirs, files in os.walk(os.path.join(line, instanceName)):
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
                  indexesToRemove.append(i)
                  break
         
         for index in sorted(indexesToRemove, reverse=True):
            del datas[index]
            del results[index]

         if not os.path.exists(os.path.join(outputDirectory, os.path.basename(line))):
            os.mkdir(os.path.join(outputDirectory, os.path.basename(line)))

         with open(os.path.join(outputDirectory, os.path.basename(line), "data.csv"), mode='w', newline='') as dataFile:
            dataCsvWriter = csv.writer(dataFile, delimiter=';')
            for data in datas:
               dataCsvWriter.writerow(data)
         
         with open(os.path.join(outputDirectory, os.path.basename(line), "results.csv"), mode='w', newline='') as resultsFile:
            resultsCsvWriter = csv.writer(resultsFile, delimiter=';')
            for result in results:
               resultsCsvWriter.writerow(result)

         shutil.copyfile(os.path.join(line, instanceName, "run_0", "points.csv"), os.path.join(outputDirectory, "points.csv"))

   print(f"Pareto analyzer completed work.. with status {process.returncode}")
   return process.returncode == 0


# def MergeFolders(outputDirectory, directoryFrom):
#    lastNumber = -1
#    for path, subdirs, files in os.walk(outputDirectory):
#       for name in subdirs:
#          if name.startswith('run'):
#             lastNumber = int(name.split('_')[1])
#    for path, subdirs, files in os.walk(directoryFrom):
#       for name in subdirs:
#          if name.startswith('run'):
#             lastNumber = lastNumber + 1
#             shutil.copytree(os.path.join(path, name), os.path.join(outputDirectory, f"run_{lastNumber}"))
#    RunPareto(outputDirectory)