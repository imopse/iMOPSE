import os
import csv

OUTPUT_PATH_ROOT = "C:/Users/adria/source/repos/iMOPSE_public/utils/DemoApp/results"

for path, subdirectories, files in os.walk(OUTPUT_PATH_ROOT):
   for subdirectory in subdirectories:
      for configPath, configSubdirectories, configFies in os.walk(os.path.join(OUTPUT_PATH_ROOT, subdirectory)):
         for configDirectory in configSubdirectories:
            if configDirectory != 'info':
               for instancePath, instanceSubdirectories, instanceFiles in os.walk(os.path.join(OUTPUT_PATH_ROOT, subdirectory, configDirectory)):
                  for instanceDirectory in instanceSubdirectories:
                     dataToSave = []
                     with open(os.path.join(OUTPUT_PATH_ROOT, subdirectory, configDirectory, instanceDirectory, 'results.csv')) as resultsFile:                     
                           reader = csv.reader(resultsFile, delimiter=';')                   
                           for row in reader:
                              if len(row) == 3 and row[2] == '0':
                                 dataToSave.append(row[:-1])
                              elif len(row) == 2:
                                 dataToSave.append(row)
                     with open(os.path.join(OUTPUT_PATH_ROOT, subdirectory, configDirectory, instanceDirectory, 'results.csv'), mode='w', newline='') as outputFile:
                        writer = csv.writer(outputFile, delimiter=';')
                        for data in dataToSave:
                           writer.writerow(data)