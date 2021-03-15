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
    self.versi_dua( path, chip, date, time, app )

  def versi_satu(self, path, chip, date, time, app ):
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

  def versi_dua(self, path, chip, date, time, app ):
    with app.app_context():
      print("***************** Estimation 2.0.0 Masking Color *****************")
      print(path)
      self.raw = cv2.imread(path)

      # Convert RGB to GRAYSCALE
      self.colorspace = cv2.cvtColor(self.raw, cv2.COLOR_BGR2RGB)

      container_color_start = (0, 0, 0)
      container_color_end = (157,157,157)
      mask = cv2.inRange(self.colorspace, container_color_start, container_color_end)
      self.masking = cv2.bitwise_and(self.raw, self.raw, mask=mask)

      _, self.binary = cv2.threshold(self.masking, 0, 255, cv2.THRESH_BINARY_INV) 
      self.imageWidth = self.binary.shape[0] #width
      self.imageHeight = self.binary.shape[1] #height
      self.imageResolution = self.imageWidth * self.imageHeight

      self.samuna = cv2.cvtColor(self.binary, cv2.COLOR_BGR2GRAY)
      self.blackPixel = self.imageResolution - cv2.countNonZero(self.samuna)

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