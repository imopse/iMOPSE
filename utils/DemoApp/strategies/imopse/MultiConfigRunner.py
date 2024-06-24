from strategies.Runner import Runner
from iMOPSERunner import iMOPSERunner
import asyncio
import typing as type

class MulticonfigRunner(Runner):
   def __init__(self, methodConfigFiles: type.List[str]) -> None:
      self.methodConfigFiles = methodConfigFiles

   def Run(self
      , loop: asyncio.BaseEventLoop
      , problemInstanceFileName
      , problemName
      , outputDirectory
      , runCount
      , onProgressUpdated: type.Callable[[int], None]
      , onError: type.Callable[[int, str], None]
      , onSuccess: type.Callable[[], None]
      , parrarel: bool):
      async def __Run(configFiles: type.List[str]):
         if len(configFiles) == 0:
            print("Multiconfig run ended")
            return
         else:
            print("Running config file: ", configFiles[0].strip())
            await runner.Run(configFiles[0].strip()
               , problemInstanceFileName
               , problemName
               , outputDirectory
               , runCount
               , onProgressUpdated
               , onError
               , onSuccess
               , parrarel)
            print(f"Running config {configFiles[0].strip()} ended")
            await __Run(configFiles[1:])
      runner = iMOPSERunner()
      task = loop.create_task(__Run(self.methodConfigFiles))
      return task, runner, len(self.methodConfigFiles)