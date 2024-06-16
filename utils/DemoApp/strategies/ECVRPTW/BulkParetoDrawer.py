import matplotlib
from matplotlib.collections import PatchCollection
matplotlib.rcParams["toolbar"] = "toolmanager"
from matplotlib.backend_bases import PickEvent
from matplotlib.backend_tools import ToolBase
import matplotlib.pyplot as plt
import csv
import asyncio
import os.path
import numpy as np
import numpy.typing as npt
from strategies.Drawable import Drawable
import typing as t


FIGSIZE = 8
VEHICLESEPERATOR=2147483647
ARROWSIZE = 1
MARKERSIZE = 25
ANNOTATIONOFFSETX = 0.5
ANNOTATIONOFFSETY = 0.5

class Metrics():
   def __init__(self, time
                  , TPFS, MPFS, HV, HVSTD, IGD, IGDSTD, PFS, PFSSTD, purity
                  , MinDistance, AverageDistance, MaxDistance, DistanceSTD
                  , MinTime, AverageTime, MaxTime, TimeSTD):
      self.time = time
      self.TPFS = TPFS
      self.MPFS = MPFS
      self.HV = HV
      self.HVSTD = HVSTD
      self.IGD = IGD
      self.IGDSTD = IGDSTD
      self.PFS = PFS
      self.PFSSTD = PFSSTD
      self.purity = purity
      self.MinDistance = MinDistance
      self.AverageDistance = AverageDistance
      self.MaxDistance = MaxDistance
      self.DistanceSTD = DistanceSTD
      self.MinTime = MinTime
      self.AverageTime = AverageTime
      self.MaxTime = MaxTime
      self.TimeSTD = TimeSTD

class MethodResult():
   def __init__(self, methodName: str, instanceName: str, color: npt.NDArray[np.floating]):
      self.methodName = methodName
      self.instanceName = instanceName
      self.metrics: Metrics
      self.data: npt.ArrayLike
      self.color: npt.NDArray[np.floating] = color
      self.scatterColorArray: t.List


class ExportTool(ToolBase):
   image=r"C:\\Users\\adria\\source\\repos\\iMOPSE_public\\utils\\DemoApp\\assets\\ios_share_24dp.png"

   def __init__(self, *args, methodResults: t.Dict[str, MethodResult] , **kwarg):
      super().__init__(*args, **kwarg)
      self.methodResults = methodResults

   def trigger(self, sender, event, data=None):
      if not os.path.exists(os.path.join(os.getcwd(), "export.csv")):
         with open(os.path.join(os.getcwd(), "export.csv"), mode="w", newline='', encoding='UTF-8') as csvFile:
            writer = csv.writer(csvFile, delimiter=';', quotechar='"')
            writer.writerow(["Nazwa instancji", "Metoda", "Średni czas wykonania"
               , "TPFS", "MPFS", "HV", "HV błąd standardowy", "IGD", "IGD błąd standardowy", "PFS", "PFS błąd standardowy", "Purity"
               , "Minimalny dystans", "Średni dystans", "Maksymalny dystans", "Dystans błąd standardowy"
               , "Minimalny czas", "Średni czas", "Maksymalny czas", "Czas błąd standardowy"]      
            )
      with open(os.path.join(os.getcwd(), "export.csv"), mode="a+", newline='') as csvFile:
         writer = csv.writer(csvFile, delimiter=';', quotechar='"')
         for i, metodName in enumerate(self.methodResults):
            result = self.methodResults[metodName]
            writer.writerow([result.instanceName, result.methodName, result.metrics.time
               , result.metrics.TPFS, result.metrics.MPFS, result.metrics.HV, result.metrics.HVSTD, result.metrics.IGD, result.metrics.IGDSTD, result.metrics.PFS, result.metrics.PFSSTD, result.metrics.purity
               , result.metrics.MinDistance, result.metrics.AverageDistance, result.metrics.MaxDistance, result.metrics.DistanceSTD
               , result.metrics.MinTime, result.metrics.AverageTime, result.metrics.MaxTime, result.metrics.TimeSTD]      
            )

class BulkParetoDrawer(Drawable):
   def __init__(self
         , paretoDirectory: str
         , resultDirectory: str
         , instanceName: str
         , singleObjective: bool
      ):
      self.paretoDirectory: str = paretoDirectory
      self.resultDirectory: str = resultDirectory
      self.InstanceName: str = instanceName
      self.methodResults: t.Dict[str, MethodResult] = {}
      self.LastPickedIndex = None
      self.paretofig = None
      self.eventId = 0
      self.singleObjective = singleObjective

   def Draw(self):
      colorGenerator = self.__GetPredefinedColor()
      for path, subdirectories, files in os.walk(self.paretoDirectory):
         for file in files:
            if file.endswith('_merged.csv'):    
               methodName = '_'.join(file.split("_")[:-1])
               #Read parreto front data
               with open(os.path.join(self.paretoDirectory,  f'{methodName}_merged.csv')) as resultsCsv:
                  reader = csv.reader(resultsCsv, delimiter=';')
                  data = []
                  for row in reader:
                     data.append(np.array(row).astype(np.float32))
                  self.methodResults[methodName] = MethodResult(methodName, self.InstanceName, next(colorGenerator))
                  self.methodResults[methodName].data = np.array(data)

      #Read true parreto front data
      with open(os.path.join(self.paretoDirectory, 'true_pareto_front_approximation.csv')) as trueParretoCsv:
         reader = csv.reader(trueParretoCsv, delimiter=';')
         trueData = []
         for row in reader:
            trueData.append(np.array(row).astype(np.float32))
         trueData = np.array(trueData)

      #Read quality data
      with open(os.path.join(self.paretoDirectory, 'quality.txt')) as qualityTxt:
         for line in qualityTxt.readlines():
            methodName = line.split(';')[0]     
            if line.startswith('TPFS:'):
               tpfs = int(line.split(':')[1])
            elif not methodName.startswith('---') and line.strip():
               qualitiesText = line.split(';')[1:]
               methodName = line.split(';')[0]
               infoLines = open(os.path.join(self.resultDirectory, methodName, 'info', self.InstanceName, 'info.txt')).readlines()
               time = float(infoLines[2])
               qualities = []
               for quality in qualitiesText:
                  splitedQulity = quality.split(":")
                  if len(splitedQulity) == 2:
                     qualities.append(splitedQulity[1])
               mpfs = float(qualities[1])
               hv = float(qualities[3])
               hvStd = float(qualities[4])
               try:
                  igd = float(qualities[7])
               except:
                  igd = float('inf')
               try:
                  igdStd = float(qualities[8])
               except:
                  igdStd = float('inf')
               pfs = float(qualities[9])
               pfsStd = float(qualities[10])
               scatterColors = []
               truePointsCount = 0.
               methodResult = self.methodResults[methodName]
               for i, row in enumerate(methodResult.data):
                  found = False
                  for j, trueDataInfo in enumerate(trueData):
                     if trueDataInfo[0] == row[0] and trueDataInfo[1] == row[1]:
                        scatterColors.append([0, 0, 0])
                        truePointsCount = truePointsCount + 1.
                        found = True
                        break
                  if not found:
                     scatterColors.append(methodResult.color)
               methodResult.scatterColorArray = scatterColors
               purity = truePointsCount/(np.size(trueData)/2)
               if len(methodResult.data) == 0:
                  methodResult.metrics = Metrics(time, 0, 0, 0, 0, 1, 0, 0, 0, 0
                     , 0, 0, 0, 0
                     , 0, 0, 0, 0
                     )
               else:
                  methodResult.metrics = Metrics(time, tpfs, mpfs, hv, hvStd, igd, igdStd, pfs, pfsStd, purity
                     , np.amin(methodResult.data[:, 0]), np.average(methodResult.data[:, 0]), np.amax(methodResult.data[:, 0]), np.std(methodResult.data[:, 0])
                     , np.amin(methodResult.data[:, 1]), np.average(methodResult.data[:, 1]), np.amax(methodResult.data[:, 1]), np.std(methodResult.data[:, 1])
                     )

      #Set style
      plt.style.use('_mpl-gallery')

      #Draw Scatter plot
      self.paretofig = plt.figure()
      self.ax = self.paretofig.add_subplot()
      markerGenerator = self.__GetPredefinedMarker()
      for i, metodName in enumerate(self.methodResults):
         if len(self.methodResults[metodName].data) == 0:
            self.ax.scatter([]
               , []
               , label=metodName
               , marker=next(markerGenerator)
               , picker=True
               , pickradius=5
               , c=[]
            )
         else:
            self.sc = self.ax.scatter(self.methodResults[metodName].data[:, 0]
               , self.methodResults[metodName].data[:, 1]
               , label=metodName
               , marker=next(markerGenerator)
               , picker=True
               , pickradius=5
               , c=self.methodResults[metodName].scatterColorArray
            )
      self.ax.set_title(f"Instancja: {self.InstanceName}")

      self.ax.legend()

      #Name axis
      self.ax.set_xlabel("Dystans")
      self.ax.set_ylabel("Czas")

      #Setup OnClick event
      # self.eventId = self.paretofig.canvas.mpl_connect('pick_event', self.__OnPick)

      #Setup figure width
      self.paretofig.set_figwidth(FIGSIZE)
      self.paretofig.set_figheight(FIGSIZE)
      self.paretofig.tight_layout()

      #Add export button
      tm = self.paretofig.canvas.manager.toolmanager
      tm.add_tool("export", ExportTool, methodResults=self.methodResults)
      self.paretofig.canvas.manager.toolbar.add_tool(tm.get_tool("export"), "toolgroup")

      #Show
      plt.show()
      
   def __OnPick(self, event: PickEvent):
      if event.artist.axes != self.ax:
         return
      self.LastPickedColor = [*self.sc._facecolors[event.ind[0],:]]
      self.LastPickedIndex = event.ind[0]
      self.sc._facecolors[event.ind[0],:] = (0, 1, 0, 1)
      self.paretofig.canvas.draw()
      self.__DrawGenotype(event.ind[0])
      #Disable OnClick event
      self.paretofig.canvas.mpl_disconnect(self.eventId)

   def __OnClose(self, event):
      self.sc._facecolors[self.LastPickedIndex,:] = (self.LastPickedColor[0], self.LastPickedColor[1], self.LastPickedColor[2], self.LastPickedColor[3])
      self.paretofig.canvas.draw()
      self.LastPickedIndex = None
      #Reconnect OnClick event
      self.eventId = self.paretofig.canvas.mpl_connect('pick_event', self.__OnPick)

   def __GetColor(self):
      return np.random.choice(range(256), size=3)/255.

   def __GetPredefinedColor(self):
      colorArray = [[1, 0, 0], [0, 1, 0], [0, 0, 1], [0.6, 0.6, 0], [0, 1, 1], [1, 0, 1], [0.5, 0.7, 0.5]]
      i = 0
      while True:
         for color in colorArray:
            if i == 10:
               return color
            yield color
            i = i+1

   def __GetPredefinedMarker(self):
      markerArray = ["o", "v", "s", "D", "x", "*", "2"]
      i = 0
      while True:
         for marker in markerArray:
            if i == 10:
               return marker
            yield marker
            i = i+1

   def __DrawQuiver(self, ax, x_data, y_data, color):
      u = np.diff(x_data)
      v = np.diff(y_data)
      pos_x = x_data[:-1] + u/2
      pos_y = y_data[:-1] + v/2
      norm = np.sqrt(u**2+v**2) 
      ax.quiver(pos_x
         , pos_y
         , u/norm
         , v/norm
         , angles="xy"
         , zorder=5
         , pivot="mid"
         , color=color
         , width = 0.002
         , headwidth=5
      )

   def __GenerateDataPointsForScatter(self, points):
      colors=[]
      size=[]
      for pointType in points[:, 2:]:
         if pointType == 'd':
            colors.append([0, 0, 0])
            size.append(MARKERSIZE * 2.5)
         elif pointType == 'f':
            colors.append([0, 1, 0])
            size.append(MARKERSIZE)
         else:
            colors.append([0, 0, 1])
            size.append(MARKERSIZE)
      return np.array(colors), np.array(size)

   def __DrawGenotype(self, index: int):
      #Read Data
      with open(os.path.join(self.DataDirectory, self.MethodName, 'data.csv')) as dataCsv:
         reader = csv.reader(dataCsv, delimiter=';')
         rows = []
         for row in reader:
            rows.append(row)
         row = rows[index]
      #Read points
      with open(os.path.join(self.DataDirectory, 'points.csv')) as pointsCsv:
         reader = csv.reader(pointsCsv, delimiter=';')
         pointsRead = []
         for point in reader:
            pointsRead.append(point)
         pointsRead = np.array(pointsRead)
      #Convert points and row to int
      points = np.array(pointsRead[:, :-1]).astype(np.int32)
      row = np.array(row).astype(np.int32)

      #Create plot
      fig, ax = plt.subplots()
      ax.set_title(f"Distance: {self.npData[index][0]:.2f} Time: {self.npData[index][1]:.2f}")
      ax.set_xlabel("x")
      ax.set_ylabel("y")
      fig.set_figwidth(FIGSIZE)
      fig.set_figheight(FIGSIZE)

      #Setup data
      x_data = []
      y_data = []
      colors, sizes = self.__GenerateDataPointsForScatter(pointsRead)
      ax.scatter(points[:,0], points[:,1], c=colors, s=sizes)
      for i, point in enumerate(points):
         multiplier = 1
         if i == 0:
            multiplier = -1
         ax.annotate(i, (point[0] + multiplier * ANNOTATIONOFFSETX, point[1] + ANNOTATIONOFFSETY))
      color = self.__GetColor()
      tolereance = 1
      for pointIndex in row:
         if pointIndex != VEHICLESEPERATOR and pointIndex != 0 and pointIndex != 1:
            x_data.append(points[pointIndex, 0])
            y_data.append(points[pointIndex, 1])
         elif (pointIndex == 0 or pointIndex == 1) and tolereance == 0:
            x_data.append(points[0, 0])
            y_data.append(points[0, 1])
            ax.plot(np.array(x_data), np.array(y_data), color=color)
            self.__DrawQuiver(ax, x_data, y_data, color)
            color = self.__GetColor()
            x_data = []
            y_data = []
            x_data.append(points[0, 0])
            y_data.append(points[0, 1])
            tolereance = 1
         elif (pointIndex == 0 or pointIndex == 1) and tolereance > 0:
            tolereance = tolereance - 1
            x_data.append(points[pointIndex, 0])
            y_data.append(points[pointIndex, 1])
         else:
            ax.plot(np.array(x_data), np.array(y_data), color=color)
            self.__DrawQuiver(ax, x_data, y_data, color)
            color = self.__GetColor()
            x_data = []
            y_data = []
            tolereance = 1
      ax.plot(np.array(x_data), np.array(y_data), color=color)
      self.__DrawQuiver(ax, x_data, y_data, color)
      fig.canvas.mpl_connect('close_event', self.__OnClose)
      fig.tight_layout()
      plt.show()
