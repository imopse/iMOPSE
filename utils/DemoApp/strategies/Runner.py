from abc import ABC, abstractmethod
import asyncio
import typing as type

class Runner(ABC):
   @abstractmethod
   def Run(self, loop: asyncio.BaseEventLoop
      , problemInstanceFileName
      , problemName
      , outputDirectory
      , runCount
      , onProgressUpdated: type.Callable[[int], None]
      , onError: type.Callable[[int, str], None]
      , onSuccess: type.Callable[[], None]
      , parrarel: bool):
      pass