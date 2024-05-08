from tkinter import *
from tkinter import filedialog as fd
from tkinter import ttk
from iMOPSERunner import RunIMOPSE
from paretoRunner import RunPareto
from matplotlibManager import RunDrawParetoFront
import asyncio
import infoManager as im

class SetupWindow():
   def __init__(self, loop: asyncio.BaseEventLoop) -> None:
      self.configFile = None
      self.problemFile = None
      self.outputDirectory = None
      self.problemName = None
      self.progress = None
      self.errorLabel = None
      self.thread = None
      self.terminateThreadEvent = None
      self.loop = loop
      self.tasks = []
      self.tasks.append(loop.create_task(self.MainLoop()))

   def __SelectConfigFile(self):
      self.configFile.set(fd.askopenfilename())

   def __SelectProblemFile(self):
      self.problemFile.set(fd.askopenfilename())

   def __SelectOutputDirectory(self):
      self.outputDirectory.set(fd.askdirectory())

   def __RunOptimization(self):
      if self.errorLabel is not None:
         self.errorLabel.grid_remove()
         self.errorLabel = None
      self.runButton["state"] = "disabled"
      self.progress.set(0)
      task, self.terminateThreadEvent = RunIMOPSE(self.loop
         , self.configFile.get()
         , self.problemFile.get()
         , self.problemName.get()
         , self.outputDirectory.get()
         , self.RunsCount.get()
         , self.__UpdateProgessBar
         , self.__OnError
         , self.__OnSuccess
      )
      self.tasks.append(task)

   def __RunMatplotlib(self):
      methodName, problemInstance = im.ReadInfo(self.outputDirectory.get())
      RunDrawParetoFront(self.loop
         , self.outputDirectory.get()
         , methodName
         , problemInstance
      )

   def __RunParetoRunner(self):
      RunPareto(self.outputDirectory.get())

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
      success = RunPareto(self.outputDirectory.get())
      if success:
         self.__RunMatplotlib()

   def __InitVariables(self):
      self.configFile = StringVar()
      self.problemFile = StringVar()
      self.outputDirectory = StringVar()
      self.problemName = StringVar()
      self.RunsCount = StringVar(value=1)
      self.progress = IntVar()

   def __OnClosing(self):
      if self.thread is not None:
         self.terminateThreadEvent.set()
      for task in self.tasks:
         task.cancel()
      self.loop.stop()
      self.Root.destroy()
      self.Root = None

   async def MainLoop(self):
      self.CreateGUI()
      while self.Root is not None:
         self.Root.update()
         await asyncio.sleep(1/120.)
         

   def CreateGUI(self):
      self.Root = Tk()
      self.Root.geometry("500x350")
      self.Root.protocol("WM_DELETE_WINDOW", self.__OnClosing)
      self.Root.title("iMOPSE Demo")

      self.__InitVariables()

      self.MainFrame = Frame(self.Root)
      self.MainFrame.pack(fill=BOTH, padx=10, pady=10)
      self.MainFrame.grid_columnconfigure(0, weight=4)
      self.MainFrame.grid_columnconfigure(1, weight=1)

      labelFrame = Frame(self.MainFrame)
      labelFrame.grid(row=0, column=0, sticky='nesw')
      label = Label(labelFrame, text="Method config file:")
      label.pack(side=LEFT)
      self.MethodEntry = Entry(self.MainFrame, width=60, textvariable=self.configFile)
      self.MethodEntry.grid(row=1, column=0, sticky='ew')
      methodButton = Button(self.MainFrame, text="Select file", command=self.__SelectConfigFile)
      methodButton.grid(row=1, column=1, sticky='ew', padx=10)

      labelProblemFrame = Frame(self.MainFrame)
      labelProblemFrame.grid(row=2, column=0, sticky='nesw')
      labelProblem = Label(labelProblemFrame, text="Problem instance file:")
      labelProblem.pack(side=LEFT)
      self.ProblemEntry = Entry(self.MainFrame, width=60, textvariable=self.problemFile)
      self.ProblemEntry.grid(row=3, column=0, sticky='ew')
      problemButton = Button(self.MainFrame, text="Select file", command=self.__SelectProblemFile)
      problemButton.grid(row=3, column=1, sticky='ew', padx=10)

      labelOutputFrame = Frame(self.MainFrame)
      labelOutputFrame.grid(row=4, column=0, sticky='nesw')
      labelOutput = Label(labelOutputFrame, text="Output directory:")
      labelOutput.pack(side=LEFT)
      self.OutputEntry = Entry(self.MainFrame, width=60, textvariable=self.outputDirectory)
      self.OutputEntry.grid(row=5, column=0, sticky='ew')
      OutputButton = Button(self.MainFrame, text="Select directory", command=self.__SelectOutputDirectory)
      OutputButton.grid(row=5, column=1, sticky='ew', padx=10)

      labelProblemNameFrame = Frame(self.MainFrame)
      labelProblemNameFrame.grid(row=6, column=0, sticky='nesw')
      labelProblemName = Label(labelProblemNameFrame, text="Problem name:")
      labelProblemName.pack(side=LEFT)
      self.ProblemNameEntry = ttk.Combobox(self.MainFrame, width=60,values=["ECVRPTW"] , textvariable=self.problemName)
      self.ProblemNameEntry.grid(row=7, column=0, sticky='ew')

      labelRunsCountFrame = Frame(self.MainFrame)
      labelRunsCountFrame.grid(row=8, column=0, sticky='nesw')
      labelRunsCount = Label(labelRunsCountFrame, text="Runs count:")
      labelRunsCount.pack(side=LEFT)
      self.RunsCountEntry = Entry(self.MainFrame, width=60, textvariable=self.RunsCount)
      self.RunsCountEntry.grid(row=9, column=0, sticky='ew')

      self.runButton = Button(self.MainFrame, text="Run optimization", command=self.__RunOptimization)
      self.runButton.grid(row=10, pady=10)
      showData = Button(self.MainFrame, text="Show data", command=self.__RunMatplotlib)
      showData.grid(row=10, column=1, padx=10, sticky='ew')
      showData = Button(self.MainFrame, text="Run quality check", command=self.__RunParetoRunner)
      showData.grid(row=11, column=1, padx=10, sticky='ew')

      self.Progressbar = ttk.Progressbar(self.MainFrame, variable=self.progress)
      self.Progressbar.grid(row=12, columnspan=2, pady=10, sticky='ew')

      return self.Root