from strategies.Runner import Runner
from iMOPSERunner import iMOPSERunner
import asyncio
import typing as type

class StandardRunner(Runner):
   def __init__(self, methodConfigFileName) -> None:
      self.methodConfigFileName = methodConfigFileName

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
      runner = iMOPSERunner()
      task = loop.create_task(runner.Run(self.methodConfigFileName
         , problemInstanceFileName
         , problemName
         , outputDirectory
         , runCount
         , onProgressUpdated
         , onError
         , onSuccess
         , parrarel))
      return task, runner, 1