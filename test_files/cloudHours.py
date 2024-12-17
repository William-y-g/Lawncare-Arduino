from oauthlib.oauth2 import BackendApplicationClient
from requests_oauthlib import OAuth2Session
from os import access
import iot_api_client as iot
from iot_api_client.rest import ApiException
from iot_api_client.configuration import Configuration
import datetime
import calendar

#keys and ids
client_i = "hjFGo5O1SEgmXxgrYlNN2B51BaAy6m8m"
client_s = "mv1ikOOC1Q7EU9ZO6GOBmORAIik7xPKvdOsezr1BeIs7eJni8kluqGLyIzKIY8wF"
thing_id = "7c97ff41-1e72-46ac-aca0-e6562b9dc4d6"
device_id = "3b0e96d1-1e80-4f5d-83e9-b4850f3f98e7"

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

ctime = datetime.datetime.now().timestamp() #float value (epoch)
#do offset for timezone

print(ctime)

cppltime = datetime.datetime.now()
print(cppltime)

#november 19 12 AM
#formula: EPOCH TIME - 6 hours
#start = 1731996000 - 21600

start = 1
end = 23

#november 19 11:59 PM
#end = 	1732082340 - 21600

setVal1 = {"value": start}
setVal2 = {"value": end}

#upload values

p_begin = "c8ae92a6-59f6-45c4-995d-63429b78b74f"
p_end = "266d0fee-726f-4bac-95cc-a42e5a19b4ce"

try:
    api_response = api.properties_v2_publish(thing_id, p_begin, setVal1)
    api_response2 = api.properties_v2_publish(thing_id, p_end, setVal2)
    print(api_response)
    print(api_response2)
except ApiException as e:
    print("Got an exception: {}".format(e))