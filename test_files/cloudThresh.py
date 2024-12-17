from oauthlib.oauth2 import BackendApplicationClient
from requests_oauthlib import OAuth2Session
from os import access
import iot_api_client as iot
from iot_api_client.rest import ApiException
from iot_api_client.configuration import Configuration

#keys and ids
client_i = "hjFGo5O1SEgmXxgrYlNN2B51BaAy6m8m"
client_s = "mv1ikOOC1Q7EU9ZO6GOBmORAIik7xPKvdOsezr1BeIs7eJni8kluqGLyIzKIY8wF"
thing_id = "7c97ff41-1e72-46ac-aca0-e6562b9dc4d6"
device_id = "3b0e96d1-1e80-4f5d-83e9-b4850f3f98e7"
p_id1 = "f6a80de6-f994-4ff0-bf0f-79eb0752cfa0"
p_id2 = "ccc6285f-e3f9-4742-97d9-8b6571fed0d2"

#generate token
oauth_client = BackendApplicationClient(client_id=client_i)
token_url = "https://api2.arduino.cc/iot/v1/clients/token"

oauth = OAuth2Session(client=oauth_client)

token = oauth.fetch_token(
    token_url=token_url,
    client_id=client_i,
    client_secret=client_s,
    include_client_id=True,
    audience="https://api2.arduino.cc/iot",
    method='POST'
)

#set up access to cloud
access_token = token.get("access_token")
client_config = Configuration(host="https://api2.arduino.cc")
client_config.access_token = access_token
client = iot.ApiClient(client_config)
api = iot.PropertiesV2Api(client)

#values to upload
setVal1 = {"value": 100}
setVal2 = {"value": 100}

#upload values
try:
    api_response = api.properties_v2_publish(thing_id, p_id1, setVal1)
    api_response = api.properties_v2_publish(thing_id, p_id2, setVal2)
    print(api_response)
except ApiException as e:
    print("Got an exception: {}".format(e)) 