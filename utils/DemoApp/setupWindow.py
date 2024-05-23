from tkinter import *
from tkinter import filedialog as fd
from tkinter import ttk
from iMOPSERunner import RunIMOPSE
from paretoRunner import RunPareto
from matplotlibManager import RunDrawParetoFront
import asyncio
import fileManager as fm
import os

class SetupWindow():
   def __init__(self, loop: asyncio.BaseEventLoop) -> None:
      self.configFile = None
      self.instanceFile = None
      self.problemName = None
      self.progress = None
      self.errorLabel = None
      self.thread = None
      self.terminateThreadEvent = None
      self.mergeDirectory = None
      self.loop = loop
      self.tasks = []
      self.runner = None

   def __SelectFile(self, parameter: StringVar):
      def __SelectFileFunc():
         parameter.set(fd.askopenfilename())
      return __SelectFileFunc

   def __RunOptimization(self):
      if self.errorLabel is not None:
         self.errorLabel.grid_remove()
         self.errorLabel = None
      self.runButton["state"] = "disabled"
      self.progress.set(0)
      task, self.runner, self.terminateThreadEvent = RunIMOPSE(self.loop
         , self.configFile.get()
         , self.instanceFile.get()
         , self.problemName.get()
         , self.__getDirectory()
         , self.RunsCount.get()
         , self.__UpdateProgessBar
         , self.__OnError
         , self.__OnSuccess
      )
      self.tasks.append(task)

   def __getDirectory(self):
      return os.path.join(os.getcwd(), 'results', fm.ReadMethodName(self.configFile.get()), fm.ReadInstanceName(self.instanceFile.get()))
   
   def __getDirectoryWithoutInstance(self):
      return os.path.join(os.getcwd(), 'results', fm.ReadMethodName(self.configFile.get()))

   def __RunMatplotlib(self):
      methodName, problemInstance, time = fm.ReadInfo(self.__getDirectoryWithoutInstance(), fm.ReadInstanceName(self.instanceFile.get()))
      RunDrawParetoFront(self.loop
         , fm.GetParetoOutputFolder(fm.ReadInstanceName(self.instanceFile.get()))
         , methodName
         , problemInstance
         , time
      )

   def __UpdateProgessBar(self, progress: int):
      self.progress.set(progress)

   def __OnError(self, exitCode: int, error: str):
      self.errorLabel = Label(self.MainFrame, text=error, foreground='Red')
      self.errorLabel.grid(row=10, columnspan=2, sticky='s')
      self.runButton["state"] = "active"
      self.tasks = [self.tasks[0]]

   def __OnSuccess(self):
      self.runButton["state"] = "active"
      self.tasks = [self.tasks[0]]
      fm.WriteConfig(self.__getDirectoryWithoutInstance(), fm.ReadInstanceName(self.instanceFile.get()))
      self.__RunPareto()

   def __RunPareto(self):
      RunPareto(fm.ReadInstanceName(self.instanceFile.get()))

   def __InitVariables(self):
      self.configFile = StringVar()
      self.instanceFile = StringVar()
      self.problemName = StringVar(value='ECVRPTW')
      self.RunsCount = StringVar(value=1)
      self.progress = IntVar()
      self.mergeDirectory = StringVar()

   def __OnClosing(self):
      if self.thread is not None:
         self.terminateThreadEvent.set()
      if self.runner is not None:
         self.runner.CancelProcess()
      for task in self.tasks:
         task.cancel()
      self.Root.destroy()
      self.Root = None

   def Run(self):
      task = self.loop.create_task(self.MainLoop())
      return task

   async def MainLoop(self):
      self.CreateGUI()
      while self.Root is not None:
         self.Root.update()
         await asyncio.sleep(1/120.)
         

   def CreateGUI(self):
      self.Root = Tk()
      self.Root.geometry("800x350")
      self.Root.protocol("WM_DELETE_WINDOW", self.__OnClosing)
      self.Root.title("iMOPSE Demo")

      self.__InitVariables()

      self.MainFrame = Frame(self.Root)
      self.MainFrame.pack(fill=BOTH, padx=10, pady=10)
      self.MainFrame.grid_columnconfigure(0, weight=3)
      self.MainFrame.grid_columnconfigure(1, weight=1)
      self.MainFrame.grid_columnconfigure(2, weight=1)

      labelFrame = Frame(self.MainFrame)
      labelFrame.grid(row=0, column=0, sticky='nesw')
      label = Label(labelFrame, text="Method config file:")
      label.pack(side=LEFT)
      self.MethodEntry = Entry(self.MainFrame, width=60, textvariable=self.configFile)
      self.MethodEntry.grid(row=1, column=0, sticky='ew')
      methodButton = Button(self.MainFrame, text="Select file", command=self.__SelectFile(self.configFile))
      methodButton.grid(row=1, column=1, columnspan=2, sticky='ew', padx=10)

      labelProblemFrame = Frame(self.MainFrame)
      labelProblemFrame.grid(row=2, column=0, sticky='nesw')
      labelProblem = Label(labelProblemFrame, text="Problem instance file:")
      labelProblem.pack(side=LEFT)
      self.ProblemEntry = Entry(self.MainFrame, width=60, textvariable=self.instanceFile)
      self.ProblemEntry.grid(row=3, column=0, sticky='ew')
      problemButton = Button(self.MainFrame, text="Select file", command=self.__SelectFile(self.instanceFile))
      problemButton.grid(row=3, column=1, columnspan=2, sticky='ew', padx=10)

      labelProblemNameFrame = Frame(self.MainFrame)
      labelProblemNameFrame.grid(row=6, column=0, columnspan=3, sticky='nesw')
      labelProblemName = Label(labelProblemNameFrame, text="Problem name:")
      labelProblemName.pack(side=LEFT)
      self.ProblemNameEntry = ttk.Combobox(self.MainFrame, width=60, values=["ECVRPTW"], textvariable=self.problemName)
      self.ProblemNameEntry.grid(row=7, column=0, columnspan=3, sticky='ew')

      labelRunsCountFrame = Frame(self.MainFrame)
      labelRunsCountFrame.grid(row=8, column=0, columnspan=3, sticky='nesw')
      labelRunsCount = Label(labelRunsCountFrame, text="Runs count:")
      labelRunsCount.pack(side=LEFT)
      self.RunsCountEntry = Entry(self.MainFrame, width=60, textvariable=self.RunsCount)
      self.RunsCountEntry.grid(row=9, column=0, columnspan=3, sticky='ew')

      self.runButton = Button(self.MainFrame, text="Run optimization", command=self.__RunOptimization)
      self.runButton.grid(row=10, column=1, sticky='ew')
      showData = Button(self.MainFrame, text="Show data", command=self.__RunMatplotlib)
      showData.grid(row=10, column=2, sticky='ew', pady=10)
      runParetoButton = Button(self.MainFrame, text="Run pareto analyzer", command=self.__RunPareto)
      runParetoButton.grid(row=11, column=1, sticky='ew', pady=10)

      self.Progressbar = ttk.Progressbar(self.MainFrame, variable=self.progress)
      self.Progressbar.grid(row=12, columnspan=3, pady=10, sticky='ew')
      textProgress = Label(self.MainFrame, textvariable=self.progress)
      textProgress.grid(row=12, columnspan=3)

      return self.Root