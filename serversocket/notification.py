import requests

#Notification REST API
class Notification:

  def __init__(self, apikey ):
    self.jwt = apikey

  def send_text( self, receiver, message ):
    message = {"to": receiver, "message": message, "reply_for": 0 }
    result = requests.post('https://senderpad.com/route/v1/send/message/', json=message, headers={'Content-Type':'application/json'})
    if result.status_code != 201:
      print('POST {}'.format(result.status_code))
      print('Success : '.format(result.json()))
  
  def send_media( self, receiver, message, media_url, media_type ):
    message = {"to": receiver, "message": message, "media_url": media_url, "type": media_type,"reply_for": 0 }
    result = requests.post('https://senderpad.com/route/v1/send/message/', json=message, headers={'Content-Type':'application/json'})
    if result.status_code != 201:
      print('POST {}'.format(result.status_code))
      print('Success : '.format(result.json()))
