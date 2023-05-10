import json

from bson import ObjectId
from pymongo import MongoClient
from datetime import datetime
import requests

registered_organization_id = '6413ebf956917f74591468fa'
registered_room_id = '6413ebf956917f74591468fd'
admin_user_id = '64157d12fd022ac2c90c0a8b'

production_base_url = 'https://dacn-backend.vercel.app/'
local_base_url = 'http://localhost:3333/'

class AbnormalTypeId:
    STRANGER = '64191579e00817dbbf4c6501'
    OVERCROWD = '64191579e00817dbbf4c6502'
    FIRE = '64191579e00817dbbf4c6503'
    OTHER = '64191579e00817dbbf4c6504'
    FAKE_FACE_REGISTER = '6430f9fb4cb19e5e6fad8a00'


def get_admin_token():
    url = production_base_url + 'auth/admin/login'
    user = {
        "email": "nam.vo@example2.com",
        "password": "Namvo123456789@"
    }
    response = requests.post(url=url, data=user).json()
    return response


def raise_sample_abnormal_event(token, data, files):
    # raise abnormal events:
    url = production_base_url + 'abnormal-event'
    # files = [('event_images', open('img1.png', 'rb'))]
    headers = {
        "Authorization": f"Bearer {token}",
    }
    respones = requests.post(url=url, data=data, headers=headers, files=files)
    print(respones.json())
    


def connect_to_mongoDB():
    client = MongoClient(
        "mongodb+srv://namhoai:kDGnC3IdjwLoyJ9m@cluster0.ibtpkkm.mongodb.net/?retryWrites=true&w=majority")
    return client


def create_room_status():
    client = connect_to_mongoDB()
    db = client["maindb"]
    roomsStatusCollection = db["rooms-status"]
    # insert some documents into the collection
    roomStatus = {
        "room_id": ObjectId("6413ebf956917f74591468fd"),
        "current_occupancy": 0,
        "total_visitor": 0,
        "total_abnormal_events": 0,
    }
    createRoomStatus = roomsStatusCollection.insert_one(roomStatus)


def create_access_event(token, data):
    # insert some documents into the collection
    url = production_base_url + 'access-event'
    headers = {
        "Authorization": f"Bearer {token}"
    }
    respones = requests.post(url=url, data=data, headers=headers)
    print(respones.json())
    


def get_room_status(token):
    url = f'http://localhost:3333/analytics/room-status/{registered_room_id}'
    headers = {
        "Authorization": f"Bearer {token}"
    }
    response = requests.get(url=url, headers=headers).json()
    print(response)


def get_visitors_by_days(token):
    url = f'http://localhost:3333/analytics/visitors-by-day/{registered_room_id}'
    headers = {
        "Authorization": f"Bearer {token}"
    }
    response = requests.get(url=url, headers=headers).json()
    print(response)


#if __name__ == '__main__':
    #token = get_admin_token()['data']['token']
    # create_access_event(token)
    # get_visitors_by_days(token)
    #raise_sample_abnormal_event(token)
