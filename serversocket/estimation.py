import cv2
import os

class CapacityEstimation:
  raw = ""
  colorspace = ""
  binary = ""

  imageWidth = ""
  imageHeight = ""
  imageResolution = ""

  percentage = ""
  capacity = 0
  fileResult = ""

  def __init__(self, path, chip, date, time, app ):
    with app.app_context():
      print("***************** Estimation 1.0.0 *****************")
      print(path)
      self.raw = cv2.imread(path)

      # Convert RGB to GRAYSCALE
      self.colorspace = cv2.cvtColor(self.raw, cv2.COLOR_RGB2GRAY)

      # Segmentation using Threshold
      _, self.binary = cv2.threshold(self.colorspace, 30, 255, cv2.THRESH_BINARY_INV)

      self.imageWidth = self.binary.shape[0] #width
      self.imageHeight = self.binary.shape[1] #height
      self.imageResolution = self.imageWidth * self.imageHeight

      self.blackPixel = self.imageResolution - cv2.countNonZero(self.binary)

      # Counting Persentage
      self.percentage = self.blackPixel/self.imageResolution * 100
      self.capacity = 100 - round(self.percentage, 0)

      cv2.putText(self.binary, "{}{}{}".format('Kapasitas : ', self.capacity, '%'), (20, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (60, 80, 20), 2, cv2.LINE_AA)

      if not os.path.isdir("static/results/" + chip + '/' + date + '/'):
          os.makedirs("static/results/" + chip + '/' + date + '/', 755, exist_ok=True)

      self.fileResult = "static/results/" + chip + '/' + date + '/' + time + '-est.jpg'
      cv2.imwrite(self.fileResult, self.binary)

  def getCapacity( self ):
    return self.capacity
  
  def getFilename( self ):
    return self.fileResult