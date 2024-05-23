import subprocess as proc
import threading as t
import typing as type
import asyncio.subprocess
import fileManager as fm
import numpy as np

def RunIMOPSE(loop: asyncio.BaseEventLoop
   , methodConfigFileName
   , problemInstanceFileName
   , problemName
   , outputDirectory
   , runCount
   , onProgressUpdated: type.Callable[[int], None]
   , onError: type.Callable[[int, str], None]
   , onSuccess: type.Callable[[], None]):  
   terminateEvent = t.Event()
   runner = Runner()
   task = loop.create_task(runner.Run(methodConfigFileName
      , problemInstanceFileName
      , problemName
      , outputDirectory
      , runCount
      , onProgressUpdated
      , onError
      , onSuccess
      , terminateEvent))
   return task, runner, terminateEvent

class Runner():
   def __init__(self) -> None:
      self.terminated = False
      self.loop = None
      self.process = None
      self.readLines = None
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
      , terminateEvent: t.Event):
      await self.__RunIMOPSE(methodConfigFileName
         , problemInstanceFileName
         , problemName
         , outputDirectory
         , runCount
         , onProgressUpdated
         , onError
         , onSuccess
         , terminateEvent)

   def CancelProcess(self):
      self.readLines.cancel()
      if self.process.returncode is None:
         self.process.kill()

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
      , terminateEvent: t.Event):
      print("Starting impose")
      self.process = await asyncio.create_subprocess_exec("./resources/imopse.exe"
         , *[methodConfigFileName
            , problemName
            , problemInstanceFileName
            , outputDirectory
            , runCount
            ]
         , stdout=proc.PIPE
         , stderr=proc.PIPE
         , stdin=proc.PIPE
      )

      print("Creating tasks")
      self.readLines = asyncio.create_task(self.__readLine(self.process, onProgressUpdated))
      await self.readLines
      try:
         await asyncio.wait_for(self.process.communicate(), timeout=0.1)
      except Exception as e:
         pass

      onProgressUpdated(100)
      returnCode = await self.process.wait()
      print("Return code: ", returnCode)
      if returnCode != 0:
         onError(returnCode, "ERROR")
      else:
         fm.SaveInfo(fm.DirectoryBack(outputDirectory), fm.ReadMethodName(methodConfigFileName), fm.ReadInstanceName(problemInstanceFileName), np.average(self.timeArray))
         onSuccess()
