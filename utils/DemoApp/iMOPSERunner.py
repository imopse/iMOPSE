import subprocess as proc
import threading as t
import typing as type
import asyncio.subprocess
import infoManager as im
import time

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
   task = loop.create_task(Runner().Run(methodConfigFileName
      , problemInstanceFileName
      , problemName
      , outputDirectory
      , runCount
      , onProgressUpdated
      , onError
      , onSuccess
      , terminateEvent))
   return task, terminateEvent

class Runner():
   def __init__(self) -> None:
      self.terminated = False
      self.loop = None

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

   async def __readLine(self, process: asyncio.subprocess.Process, onProgressUpdated: type.Callable[[int], None]):
      while process.returncode is None:
         data = None
         try:
            data = await asyncio.wait_for(process.stdout.readline(), timeout=0.1)
            onProgressUpdated(int(data))
         except:
            if data != None and data != b'':
               print(data)
         try:
            await asyncio.wait_for(process.communicate(), timeout=0.1)
         except:
            pass

   async def __CheckTerminate(self, process: asyncio.subprocess.Process, terminateEvent: t.Event):
      while process.returncode is None:
         if terminateEvent.is_set():
            self.terminated = True
            process.terminate()
            return
         await asyncio.sleep(0.1)

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
      startTime = time.time()
      process = await asyncio.create_subprocess_exec("./resources/imopse.exe"
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

      checkTerminate = asyncio.create_task(self.__CheckTerminate(process, terminateEvent))
      readLines = asyncio.create_task(self.__readLine(process, onProgressUpdated))
      await asyncio.wait([checkTerminate
         , readLines
         ]
         , return_when=asyncio.FIRST_COMPLETED)

      if self.terminated:
         readLines.cancel()
         return
      else:
         checkTerminate.cancel()
      onProgressUpdated(100)
      endTime = time.time()
      print("Return code: ", process.returncode)
      if process.returncode != 0:
         try:
            print("Trying to read stderr...")
            msg = await asyncio.wait_for(process.stderr.readline(), timeout=0.1)
            if msg == b'':
               print("Trying to read stdout...")
               msg = await asyncio.wait_for(process.stdout.readline(), timeout=0.1)
               onError(process.returncode, msg)
            onError(process.returncode, msg)
         except:
            print("Trying to read stdout...")
            try:
               onError(process.returncode, await asyncio.wait_for(process.stdout.readline(), timeout=0.1))
            except:
               pass        
      else:
         im.SaveInfo(outputDirectory, methodConfigFileName, problemInstanceFileName, endTime - startTime)
         onSuccess()
