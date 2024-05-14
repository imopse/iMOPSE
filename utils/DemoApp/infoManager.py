import os

def SaveInfo(directory: str, methodNameFile: str, problemInstanceNameFile, time):
   print("Saving info...")
   try:
      with open(os.path.join(directory, 'info.txt'), mode='w') as infoFile:
         infoFile.writelines([os.path.basename(os.path.normpath(methodNameFile)) + '\n', os.path.basename(os.path.normpath(problemInstanceNameFile)) + '\n', str(time)])
         infoFile.close()
   except:
      print("Saving failed...")
   return

def ReadInfo(directory: str):
   print("Reading info...")
   with open(os.path.join(directory, 'info.txt')) as infoFile:
      lines = infoFile.readlines()
   return lines[0], lines[1], float(lines[2])