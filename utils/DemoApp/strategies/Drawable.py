from abc import ABC, abstractmethod

class Drawable(ABC):
   @abstractmethod
   def Draw(self):
      pass