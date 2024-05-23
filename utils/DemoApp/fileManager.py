import os

def SaveInfo(directory: str, methodName: str, instanceName, time):
   print("Saving info...")
   try:
      if not os.path.isdir(os.path.join(directory, 'info', instanceName)):
         os.makedirs(os.path.join(directory, 'info', instanceName))
      with open(os.path.join(directory, 'info', instanceName, 'info.txt'), mode='w') as infoFile:
         infoFile.writelines([methodName + '\n', instanceName + '\n', str(time)])
         infoFile.close()
   except Exception as e:
      print("Saving failed...", e)
      return
   print("Save succeded..")

def DirectoryBack(directory: str):
   return os.path.dirname(directory)

def ReadInfo(directory: str, instanceName: str):
   print("Reading info...")
   with open(os.path.join(directory, 'info', instanceName, 'info.txt')) as infoFile:
      lines = infoFile.readlines()
   return lines[0].replace('\n', ''), lines[1].replace('\n', ''), float(lines[2])

def ReadMethodName(filePath: str):
   with open(filePath) as config:
      for line in config.readlines():
         if line.startswith("MethodName"):
            return line.split(' ')[1][:-1]
   return "NOMETHODNAME"

def ReadInstanceName(filePath: str):
   return os.path.basename(filePath).split('.')[0].replace('\n', '')

def WriteConfig(outputDirectory: str, instanceName: str):
   print("Writing config...")
   try:
      path = os.path.join(os.getcwd(), "config", instanceName)
      if not os.path.isdir(path):
         os.makedirs(path)
      lines = []
      if os.path.exists(os.path.join(os.getcwd(), "config", instanceName, "config.txt")):
         with open(os.path.join(os.getcwd(), "config", instanceName, "config.txt"), mode='r') as configFile:
            for line in configFile.read().splitlines():
               print(line)
               lines.append(line.replace('\n', '') + '\n')
      with open(os.path.join(os.getcwd(), "config", instanceName, "config.txt"), mode='w+') as configFile:
         configFile.writelines([*lines, outputDirectory])
   except Exception as e:
      print("Config save failed..", e)
      return
   print("Config written...")

def GetParetoOutputFolder(instanceName: str):
   return os.path.join(os.getcwd(), "result_pareto", instanceName).replace('\\', '/')