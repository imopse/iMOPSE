import matplotlib
from strategies.Drawable import Drawable
matplotlib.rcParams["toolbar"] = "toolmanager"
from matplotlib.backend_bases import PickEvent
from matplotlib.backend_tools import ToolBase
import matplotlib.pyplot as plt
import csv
import asyncio
import os.path
import numpy as np

FIGSIZE = 8
VEHICLESEPERATOR=2147483647
ARROWSIZE = 1
MARKERSIZE = 25
ANNOTATIONOFFSETX = 0.5
ANNOTATIONOFFSETY = 0.5

class ExportTool(ToolBase):
   #image= <- Provide path to png

   def __init__(self, *args, methodName, instanceName, time
                  , TPFS, MPFS, HV, HVSTD, IGD, IGDSTD, PFS, PFSSTD, purity
                  , MinDistance, AverageDistance, MaxDistance, DistanceSTD
                  , MinTime, AverageTime, MaxTime, TimeSTD
                  , **kwarg):
      super().__init__(*args, **kwarg)
      self.methodName = methodName
      self.instanceName = instanceName
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
         writer.writerow([self.instanceName, self.methodName, self.time
            , self.TPFS, self.MPFS, self.HV, self.HVSTD, self.IGD, self.IGDSTD, self.PFS, self.PFSSTD, self.purity
            , self.MinDistance, self.AverageDistance, self.MaxDistance, self.DistanceSTD
            , self.MinTime, self.AverageTime, self.MaxTime, self.TimeSTD]      
         )

class ParetoDrawer(Drawable):
   def __init__(self
         , dataDirectory: str
         , methodName: str
         , instanceName: str
         , time: float | None
         , singleObjective: bool
      ) -> None:
      self.DataDirectory: str = dataDirectory
      self.MethodName: str = methodName
      self.InstanceName: str = instanceName
      self.npData = None
      self.LastPickedIndex = None
      self.paretofig = None
      self.eventId = 0
      self.time = time
      self.singleObjective = singleObjective

   def Draw(self):
      #Read parreto front data
      with open(os.path.join(self.DataDirectory, self.MethodName, 'results.csv')) as resultsCsv:
         reader = csv.reader(resultsCsv, delimiter=';')
         data = []
         for row in reader:
            data.append(np.array(row).astype(np.float32))
         self.npData = np.array(data)
      
      #Read true parreto front data
      with open(os.path.join(self.DataDirectory, 'true_pareto_front_approximation.csv')) as trueParretoCsv:
         reader = csv.reader(trueParretoCsv, delimiter=';')
         trueData = []
         for row in reader:
            trueData.append(np.array(row).astype(np.float32))
         trueData = np.array(trueData)

      #Read quality data
      with open(os.path.join(self.DataDirectory, 'quality.txt')) as qualityTxt:
         for line in qualityTxt.readlines():
            methodName = line.split(';')[0]
            if line.startswith('TPFS:'):
               tpfs = int(line.split(':')[1])
            if methodName == self.MethodName:
               qualitiesText = line.split(';')[1:]
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

      #Set style
      plt.style.use('_mpl-gallery')

      #Draw Scatter plot
      self.paretofig = plt.figure()
      self.ax = self.paretofig.add_subplot()

      scatterColors = []
      truePointsCount = 0.
      for i, row in enumerate(self.npData):
         found = False
         for j, trueDataInfo in enumerate(trueData):
            if trueDataInfo[0] == row[0] and trueDataInfo[1] == row[1]:
               scatterColors.append([0, 0, 0])
               truePointsCount = truePointsCount + 1.
               found = True
               break
         if not found:
            scatterColors.append([0, 0, 1])
      purity = truePointsCount/(np.size(trueData)/2)

      self.sc = self.ax.scatter(self.npData[:, 0], self.npData[:, 1], picker=True, pickradius=5, c=scatterColors)
      #distanceText = f"Distance: Best: {np.amin(self.npData[:, 0]):0.2f} Average: {np.average(self.npData[:, 0]):0.2f} Worst: {np.amax(self.npData[:, 0]):0.2f} Std: {np.std(self.npData[:, 0]):0.2f}"
      #timeText = f"Time: Best: {np.amin(self.npData[:, 1]):0.2f} Average: {np.average(self.npData[:, 1]):0.2f} Worst: {np.amax(self.npData[:, 1]):0.2f} Std: {np.std(self.npData[:, 1]):0.2f}"
      #paretoFrontText = f"True pareto front approximation\nTPFS: {tpfs} MPFS: {mpfs:0.6f} HV: {hv:0.6f}\nIGD: {igd:0.6f} PFS: {pfs:0.6f} Purity: {purity:0.3f}"
      self.ax.set_title(f"Metoda: {self.MethodName} Instancja: {self.InstanceName}")# Average time: {self.time:0.2f}\n{distanceText}\n{timeText}\n{paretoFrontText}")

      #Draw True Pareto Scatter plot
      # axTrue.scatter(trueData[:, 0], trueData[:, 1], color="black")
      # axTrue.set_title(f"True pareto front approximation\nTPFS: {tpfs} MPFS: {mpfs:0.6f} HV: {hv:0.6f}\nIGD: {igd:0.6f} PFS: {pfs:0.6f} Purity: {purity:0.6f}")
      # axTrue.set_xlabel("Distance")
      # axTrue.set_ylabel("Time")

      #Name axis
      self.ax.set_xlabel("Distance")
      self.ax.set_ylabel("Time")

      #Setup OnClick event
      self.eventId = self.paretofig.canvas.mpl_connect('pick_event', self.__OnPick)

      #Setup figure width
      self.paretofig.set_figwidth(FIGSIZE)
      self.paretofig.set_figheight(FIGSIZE)
      self.paretofig.tight_layout()

      #Add export button
      tm = self.paretofig.canvas.manager.toolmanager
      tm.add_tool("export", ExportTool
                  , methodName=self.MethodName, instanceName=self.InstanceName, time=self.time
                  , TPFS=tpfs, MPFS=mpfs, HV=hv, HVSTD=hvStd, IGD=igd, IGDSTD=igdStd, PFS=pfs, PFSSTD=pfsStd, purity=purity
                  , MinDistance=np.amin(self.npData[:, 0]), AverageDistance=np.average(self.npData[:, 0]), MaxDistance=np.amax(self.npData[:, 0]), DistanceSTD=np.std(self.npData[:, 0])
                  , MinTime=np.amin(self.npData[:, 1]), AverageTime=np.average(self.npData[:, 1]), MaxTime=np.amax(self.npData[:, 1]), TimeSTD=np.std(self.npData[:, 1]))
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
