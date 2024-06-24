from strategies.ECVRPTW.BulkParetoDrawer import BulkParetoDrawer
from strategies.ECVRPTW.ParetoDrawer import ParetoDrawer
from strategies.Drawable import Drawable

from strategies.imopse.StandardRunner import StandardRunner
from strategies.imopse.MultiConfigRunner import MulticonfigRunner
from strategies.Runner import Runner

import fileManager as fm
import os

def ResolveDrawMethod(problem: str, instancePath: str | None = None, methodPath: str | None = None, singleObjective: bool = False) -> Drawable:
   useMulticonfig = False
   if len(methodPath) > 0:
      with open(methodPath) as methodFile:
         if methodFile.readline().startswith("#MULTICONFIG"):
            useMulticonfig = True
   match problem:
      case "ECVRPTW":
         if len(methodPath) > 0 and not useMulticonfig:
            methodName, problemInstance, time = fm.ReadInfo(os.path.join(os.getcwd(), 'results', fm.ReadMethodName(methodPath)), fm.ReadInstanceName(instancePath))         
            return ParetoDrawer(fm.GetParetoOutputFolder(fm.ReadInstanceName(instancePath))
               , methodName
               , problemInstance
               , time
               , singleObjective)
         else:
            return BulkParetoDrawer(fm.GetParetoOutputFolder(fm.ReadInstanceName(instancePath))
               , os.path.join(os.getcwd(), 'results')
               , fm.ReadInstanceName(instancePath)
               , singleObjective      
            )
   return None

def ResolveRunnerMethod(methodPath: str) -> Runner:
   useMulticonfig = False
   configs = []
   with open(methodPath) as methodFile:
      if methodFile.readline().startswith("#MULTICONFIG"):
         useMulticonfig = True
      for line in methodFile.readlines():
         configs.append(line)
   
   if useMulticonfig:
      print("Using multiconfig runner")
      return MulticonfigRunner(configs)
   else:
      print("Using standard runner")
      return StandardRunner(methodPath)