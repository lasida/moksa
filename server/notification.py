#Notification Class Vocomon
import requests

class Notification:
  def __init__(self, jwt, device):
    self.jwt = jwt
    self.device = device

  def send_text( self, receiver, message ):
    message = {"to": receiver, "message": message, "reply_for": 0 }
    resp = requests.post('http://jawa.senderpad.com/messages/send-text', json=message, headers={ 'Authorization' : 'Bearer ' + self.jwt, 'device-key' : self.device, 'Content-Type':'application/json'})
    if resp.status_code != 201:
        print('POST /tasks/ {}'.format(resp.status_code))
    print('Success : '.format(resp.json()))
  
  def send_media( self, receiver, message, media_url, media_type ):
    message = {"to": receiver, "message": message, "media_url": media_url, "type": media_type,"reply_for": 0 }
    resp = requests.post('http://jawa.senderpad.com/messages/send-media', json=message, headers={ 'Authorization' : 'Bearer ' + self.jwt, 'device-key' : self.device, 'Content-Type':'application/json'})
    if resp.status_code != 201:
        print('POST /tasks/ {}'.format(resp.status_code))
    print('Success : '.format(resp.json()))
