import cv2
# from main import app

class Estimation:
  raw = ""
  colorspace = ""
  img = ""
  result = ""
  binary = ""
  capacity = 0
  percentage = 0
  blackPixel = 0
  totalPixel = 0
  filename = ""

  def __init__(self, path, device_chip, device_date, device_time, app ):
    with app.app_context():
      print( "Path : " + path)
      self.path = path

      #Read Image
      self.raw = cv2.imread( self.path )

    #Convert RGB to GRAYSCALE
    # self.colorspace = cv2.cvtColor(self.raw, cv2.COLOR_RG   B2GRAY)

    # #Pisahkan Warna Kontainer dengan Warna Disekitar

    # #Crop Secara Bitwise -> Area Irisan = Object Timbunan

    # #Hitung Tingg

    # #Segmentation using Threshold
    # _, self.binary = cv2.threshold(self.colorspace, 30, 255, cv2.THRESH_BINARY_INV)

    # img_width = self.binary.shape[0]
    # img_height = self.binary.shape[1]
    # img_resolution = img_width * img_height

    # # print(self.binary)
    # self.blackPixel = img_resolution - cv2.countNonZero(self.binary);
    # # print(self.blackPixel)

    # #Counting Persentage
    # temp = self.blackPixel/img_resolution * 100
    # self.percentage = round( temp, 0 )
    # return self.percentage

    # cv2.putText(self.binary, "{}{}{}".format('Kapasitas : ', self.percentage, '%'), (20, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (60,80,20), 2, cv2.LINE_AA)
    # self.filename = "static/devices/" + device_chip + '/' + device_date + '/' + device_time + '-est.jpg'
    # cv2.imwrite(self.filename, self.binary) 

  def getFilename( self ):
    return self.filename
  
  def getEstimation( self ):
    return self.percentage