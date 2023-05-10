import paho.mqtt.client as mqtt
from web_api import *


registered_organization_id = '6413ebf956917f74591468fa'
registered_room_id = '6413ebf956917f74591468fd'
admin_user_id = '64157d12fd022ac2c90c0a8b'

production_base_url = 'https://dacn-backend.vercel.app/'
local_base_url = 'http://localhost:3333/'

prev_user = ""
prev_stranger = ""



def get_user_id(input_string):
	result = input_string.split("-")
	return result[0]


def post_access(message):
	token = get_admin_token()['data']['token']
	message = get_user_id(message)
	now = datetime.now().isoformat()
	data = {
	    "organization_id": registered_organization_id,
	    "room_id": registered_room_id,
	    "user_id": message,
	    "is_guest": json.dumps(False),
	    "accessed_time": now
	}
	print("Post_access")
	create_access_event(token, data);
		


# Define callback functions for MQTT events
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker!")
        client.subscribe("authentication")
        client.subscribe("in")
        client.subscribe("out")
        client.subscribe("total")
    else:
        print("Failed to connect to MQTT broker.")
        
    
def on_message(client, userdata, msg):
	global prev_user
	token = get_admin_token()['data']['token']
	if msg.topic == "authentication":
		message = msg.payload.decode()
		if prev_user != message:
			prev_user = message
			if message != "New Person":
				arr = message.split("-")
				if arr[1] == "abnomal":
					data = {
						'organization_id': registered_organization_id,
						'room_id': registered_room_id,
						'abnormal_type_id': AbnormalTypeId.STRANGER,
						'note': 'Generated from python!'
					}
					file_path = "/home/jp51/workspace/face_recognition_tensorRT/abnomal_images/"
					file_path = file_path + message
					file_path = file_path + ".jpg"
					files = [('event_images', open(file_path, 'rb'))]
					raise_sample_abnormal_event(token, data, files)
				
				else:
					#print(f"token: {token}")
					message = get_user_id(message)
					now = datetime.now().isoformat()
					data = {
						"organization_id": registered_organization_id,
						"room_id": registered_room_id,
						"user_id": message,
						"is_guest": json.dumps(False),
						"accessed_time": now
					}
					#print(f"data: {json.dumps(data)}")
					create_access_event(token, data)
					#print(f"Name: {msg.payload.decode()}")
					#post_access(msg.payload.decode())
			else:
				print("Abnomal Event")
			
		
		
	elif msg.topic == "total":
		print(f"total:   {msg.payload.decode()}")
	elif msg.topic == "in":
		print(f"in:   {msg.payload.decode()}")
	elif msg.topic == "out":
		print(f"out: {msg.payload.decode()}")




# Create MQTT client object
client = mqtt.Client()

# Set callback functions
client.on_connect = on_connect
client.on_message = on_message

# Connect to MQTT broker
client.connect("localhost")

# Start the MQTT client loop to listen for incoming messages
client.loop_forever()

