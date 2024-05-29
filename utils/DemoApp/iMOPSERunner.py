import subprocess as proc
import threading as t
import typing as type
import asyncio.subprocess
import fileManager as fm
import numpy as np
import shutil
import os

def RunIMOPSE(loop: asyncio.BaseEventLoop
   , methodConfigFileName
   , problemInstanceFileName
   , problemName
   , outputDirectory
   , runCount
   , onProgressUpdated: type.Callable[[int], None]
   , onError: type.Callable[[int, str], None]
   , onSuccess: type.Callable[[], None]
   , parrarel: bool):  
   runner = Runner()
   task = loop.create_task(runner.Run(methodConfigFileName
      , problemInstanceFileName
      , problemName
      , outputDirectory
      , runCount
      , onProgressUpdated
      , onError
      , onSuccess
      , parrarel))
   return task, runner

class Runner():
   def __init__(self) -> None:
      self.terminated = False
      self.loop = None
      self.processes: type.List[asyncio.subprocess.Process] = []
      self.readLines: type.List[asyncio.Task[None]] = []
      self.timeArray = []

   async def Run(self
      , methodConfigFileName
      , problemInstanceFileName
      , problemName
      , outputDirectory
      , runCount
      , onProgressUpdated: type.Callable[[int], None]
      , onError: type.Callable[[int, str], None]
      , onSuccess: type.Callable[[], None]
      , parrarel: bool):
      await self.__RunIMOPSE(methodConfigFileName
         , problemInstanceFileName
         , problemName
         , outputDirectory
         , runCount
         , onProgressUpdated
         , onError
         , onSuccess
         , parrarel)

   def CancelProcess(self):
      for readline in self.readLines:
         readline.cancel()
      for process in self.processes:
         if process.returncode is None:
            process.kill()

   async def __readLine(self, process: asyncio.subprocess.Process, onProgressUpdated: type.Callable[[int], None]):
      data = None
      while data != b'':
         try:
            data = await process.stdout.readline()
            if data.startswith(b'Finished'):
               self.timeArray.append(float(data.split(b' ')[2][:-4])/1000.)
            else:
               onProgressUpdated(int(data))
         except Exception as e:
            if data != None:
               print(data)

   async def __RunIMOPSE(self
      , methodConfigFileName
      , problemInstanceFileName
      , problemName
      , outputDirectory
      , runCount
      , onProgressUpdated: type.Callable[[int], None]
      , onError: type.Callable[[int, str], None]
      , onSuccess: type.Callable[[], None]
      , parrarel: bool):
      actualRunCount = runCount
      processCount=1
      if parrarel == True:
         processCount = runCount
         actualRunCount = 1
      try:
         for i in range(0, int(processCount)):
            print(f"Starting imopse {i}")
            process = await asyncio.create_subprocess_exec("./resources/imopse.exe"
               , *[methodConfigFileName
                  , problemName
                  , problemInstanceFileName
                  , os.path.join(outputDirectory, str(i))
                  , str(actualRunCount)
                  ]
               , stdout=proc.PIPE
               , stderr=proc.PIPE
               , stdin=proc.PIPE
            )
            self.processes.append(process)
            print(f"Creating tasks {i}")
            self.readLines.append(asyncio.create_task(self.__readLine(process, onProgressUpdated)))

         print("Waiting...")
         await asyncio.wait(self.readLines)
         print("Wait completed")
  
         onProgressUpdated(100)
         failedProcesses = 0
         for process in self.processes:
            print(f"Waiting for process {process}")
            try:
               await asyncio.wait_for(process.communicate(), timeout=0.1)
            except Exception as e:
               pass
            returnCode = await process.wait()
            print("Return code: ", returnCode)
            if returnCode != 0:
               failedProcesses = failedProcesses + 1            
               
         if failedProcesses > 0:
            onError(0, f"{failedProcesses} tasks failed")

         if failedProcesses != int(processCount):
            for i in range(0, int(processCount)):
               if parrarel == True:
                  outputPath = os.path.join(outputDirectory, f"run_{i+failedProcesses}")
                  counter = 0
                  while os.path.exists(outputPath):
                     outputPath = os.path.join(outputDirectory, f"run_{counter}")
                     counter = counter + 1
                  if self.processes[i].returncode == 0:
                     shutil.copytree(os.path.join(outputDirectory, str(i), "run_0"), outputPath)
               elif self.processes[i].returncode == 0:
                  shutil.copytree(os.path.join(outputDirectory, str(i)), outputDirectory)
               shutil.rmtree(os.path.join(outputDirectory, str(i)))
            timesRead = fm.ReadTime(fm.DirectoryBack(outputDirectory), fm.ReadInstanceName(problemInstanceFileName))
            if timesRead != None:
               self.timeArray.extend(timesRead)
            else:
               self.timeArray
            fm.SaveInfo(fm.DirectoryBack(outputDirectory), fm.ReadMethodName(methodConfigFileName), fm.ReadInstanceName(problemInstanceFileName), np.average(self.timeArray))
            fm.SaveTime(fm.DirectoryBack(outputDirectory), fm.ReadInstanceName(problemInstanceFileName), self.timeArray)
            onSuccess()
      except Exception as e:
         print(f"iMOPSE runner: {e}")
