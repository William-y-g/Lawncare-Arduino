from oauthlib.oauth2 import BackendApplicationClient
from requests_oauthlib import OAuth2Session
from os import access
import iot_api_client as iot
from iot_api_client.rest import ApiException
from iot_api_client.configuration import Configuration
import datetime
import calendar
import random
import time
import math

'''Turn arduino on and connect it to internet and have it undergo normal operations then run this script'''
'''This script is the one correlated with testResults.txt'''

#keys and ids
client_i = "hjFGo5O1SEgmXxgrYlNN2B51BaAy6m8m"
client_s = "mv1ikOOC1Q7EU9ZO6GOBmORAIik7xPKvdOsezr1BeIs7eJni8kluqGLyIzKIY8wF"
thing_id = "7c97ff41-1e72-46ac-aca0-e6562b9dc4d6"
device_id = "3b0e96d1-1e80-4f5d-83e9-b4850f3f98e7"

valves = ["38288b67-f2d2-4a43-9e85-8e46e194fa12", "e8eee459-a4e7-490a-a19d-a1680b004a4f"]
soilpids = ["37ca0716-d635-4e27-8a17-f0bd98feb1fc", "04598216-6bad-44f6-ae1d-77ad31dfad6a", "0775e5d0-5343-41b4-910e-3e2b0521e902", "fe3c947d-ec2d-4f2a-bb65-588c3217cfec", "3267393f-ba83-4920-a473-223c4a2567fb", "1c6e8e2e-1fc7-4f63-b674-4a636cb29dde"]

modepid = "2b1208ef-b048-4986-a8dc-7a62cec2e0bb"

#generate token
oauth_client = BackendApplicationClient(client_id=client_i)
token_url = "https://api2.arduino.cc/iot/v1/clients/token"

oauth = OAuth2Session(client=oauth_client)

token = oauth.fetch_token(
    token_url=token_url,
    client_id=client_i,
    client_secret=client_s,
    include_client_id=True,
    audience="https://api2.arduino.cc/iot"
    #method='POST'
)

#set up access to cloud
access_token = token.get("access_token")
client_config = Configuration(host="https://api2.arduino.cc")
client_config.access_token = access_token
client = iot.ApiClient(client_config)
api = iot.PropertiesV2Api(client)


def pushCloud(tid, val, pid):
    try:
        api_response = api.properties_v2_publish(tid, pid, val)
        time.sleep(1)
        return api_response
    except ApiException as e:
        print("Got an exception: {}".format(e))
        
def getCloud(tid, pid):
    try:
        api_response = api.properties_v2_show(tid, pid)
        #time.sleep(3)
        return api_response.last_value
    except ApiException as e:
        print("Got an exception: {}".format(e))
        
#valves
valveVals = []
zoneStatus = ['2', '2']  #set to default, please make sure they're default before calling this test
runSoil = False
runStatus = False

'''Soil moisture -> valve testing'''

soilVals = []
threshold = 50
#calculates what the expected valve output should be
def soilHelper():       
    
    #part 1, see if each zone requires watering     
    count1 = 0
    count2 = 0
    print("Zone 1: " + zoneStatus[0])
    for val in soilVals[0:3]:
        print(val)
        if (val >= threshold): #arbitrary threshold
            count1 += 1
            
    print("Zone 2: " + zoneStatus[1])
    for val in soilVals[3:6]:
        print(val)
        if (val >= threshold): #arbitrary threshold
            count2 += 1
            
    #part 2, turning valves on/off    
    
    #if a manual override for turning a zone on
    for v in range(0,2):
        if (zoneStatus[v] == '1'):
            for n in range(0,2):#turn off all other zones
                if (zoneStatus[n] == '1'):
                    zoneStatus[n] = '2' 
                valveVals[n] = False
            zoneStatus[v] = '1'
            valveVals[v] = True
            return
                
    #zone 1
    if (zoneStatus[0] != '3'):
        if (count1 >= 2):
            if (valveVals[1] == True): #if zone 2 is already on, turn it off
                valveVals[1] = False
            valveVals[0] = True
            return
        else:
            valveVals[0] = False
    else:
        valveVals[0] = False
    
    #zone 2
    if (zoneStatus[1] != '3'):
        if (count2 >= 2):
            valveVals[1] = True
            return
        else: #if zone 2 should be off
            valveVals[1] = False
    else:
        valveVals[1] = False
    
    
def soilTest():
    
    if (not runSoil):
        return
    
    f = open("testResults.txt", "w")
    
    cValue = random.randint(0,100)
    
    #instantiate the soilpid values for speed purposes
    print("getting initial soil vals")
    for s in soilpids:
        soilVals.append(getCloud(thing_id, s))
        
    print("getting initial valve vals")
    for v in valves:
        valveVals.append(getCloud(thing_id, v))
    
    for i in range (0,100):
        print("iteration: " + str(i))
        cRandom = random.randint(0,5) #choose random sensor to update
            
        cSensor = soilpids[cRandom]
        if (cValue > 50): #ensures (mostly) alternation of input (this test becomes rly slow without this bc lots of same result)
            cValue = random.randint(0,50)
        else:
            cValue = random.randint(50,100)
        
        #update local list of soil sensor vals
        soilVals[cRandom] = cValue
        
        #if we want to also randomize the setting of a particular zone
        if (runStatus):
            manualControl()
        
        #calculate valve status after cValue is updated (emulates soil sensor sending data to arduino)
        soilHelper()
            
        #push new sensor value to cloud
        valueWrapper = {"value": cValue}
        print("setting: " + str(cRandom) + " to: " + str(cValue))
        pushCloud(thing_id, valueWrapper, cSensor)
        
        #obtain valve status from the cloud after arduino has processed new sensor datas
        count = 0
        while (True):
            cloudResult1 = getCloud(thing_id, valves[0])
            cloudResult2 = getCloud(thing_id, valves[1])
            
            if (cloudResult1 == valveVals[0] and cloudResult2 == valveVals[1]):
                result = "PASS"
                break
            else:
                result = "FAIL"
                count += 1
                if (count > 5):
                    break
                time.sleep(2)
            
        zone1 = zoneStatus[0]
        zone2 = zoneStatus[1]
        
        f.write("Setting sensor " + str(cRandom + 1) + " to " + str(cValue) + ". Zones are: (" + zone1 + ", " + zone2 + "). Result should be: (" + str(valveVals[0]) + ", " + str(valveVals[1])  + ") got: (" + str(cloudResult1) + ", " + str(cloudResult2)  + ") " + result + "\n") 
  
    f.close()

#for testing manual control inputs          
def manualControl():
    pick = random.randint(0,10)
    if (pick == 10): #all automatic
        value = "+"
        valueWrapper = {"value": "+"}
        for n in range(0,2):
            zoneStatus[n] = '2'
    elif (pick == 9): #all off
        value = "-"
        valueWrapper = {"value": "-"}
        for n in range(0,2):
            zoneStatus[n] = '3'
    else: #edit manual zone
        cVal = random.randint(1,2)
        cVal2 = random.randint(1,3)
        value = str(cVal) + " " + str(cVal2)
        valueWrapper = {"value": value}
        zoneStatus[cVal - 1] = str(cVal2)

    pushCloud(thing_id, valueWrapper, modepid)
            
            
'''Here call the test u wanna run'''
runSoil = True
runStatus = True

'''do not change'''
soilTest()