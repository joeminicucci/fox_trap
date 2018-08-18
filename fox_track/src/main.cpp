#include <painlessMesh.h>

#define   MESH_PREFIX     "Not_a_botnet"
#define   MESH_PASSWORD   "password"
#define   MESH_PORT       5566

// Prototype
Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;
uint16_t channel = 6;
int32_t maxRttDelay = 0;
unsigned long initDelay = 15000000;
uint32_t acknowledgementInterval = 900000; //ms
std::vector<long> foundTargetNodeIds;
const char* mac;


void receivedCallback( uint32_t from, String &msg );
void rootInitialization();
void acknowledgeTargets();
void disableAck();
//Async functions
Task rootInitializationTask(TASK_IMMEDIATE, TASK_ONCE, &rootInitialization, &userScheduler);
Task acknowledgementTask(TASK_SECOND * 3, 20, &acknowledgeTargets, &userScheduler, false, NULL, &disableAck);


void acknowledgeTargets(){

    if (!foundTargetNodeIds.empty()){


        DynamicJsonBuffer jsonBuffer;
        JsonObject& msg = jsonBuffer.createObject();
        for (auto nodeId : foundTargetNodeIds) // access by reference to avoid copying
        {
            msg["foundack"] = nodeId;
            String str;
            msg.printTo(str);
            msg.printTo(Serial);
            Serial.printf("\n");
            mesh.sendBroadcast(str);
        }
    }

}

void disableAck()
{
    foundTargetNodeIds.clear();
}

void sendInitializationSignal()
{
    DynamicJsonBuffer jsonBuffer;
    JsonObject& msg = jsonBuffer.createObject();
    msg["initialize"] = (maxRttDelay*2) + mesh.getNodeTime();

    String str;
    msg.printTo(str);
    mesh.sendBroadcast(str);

    // log to serial
    Serial.printf("c2 initialization sent: ");
    msg.printTo(Serial);
    Serial.printf("\n");
}

void onNodeDelayReceivedCallback(uint32_t nodeId, int32_t delay)
{
    Serial.printf("ROOT:onNodeDelayReceivedCallback: %u, %i\n", nodeId, delay);
    maxRttDelay = (delay > maxRttDelay)
        ? delay
        : maxRttDelay;
}

void rootInitialization(){
    Serial.printf("ROOT:rootInitialization_BEGIN");

    mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP, channel);
    mesh.onReceive(&receivedCallback);
    //keep the topology from unnesecary re-organization
    mesh.setRoot(true);
    // This and all other mesh should ideally now the mesh contains a root
    mesh.setContainsRoot(true);
    mesh.onNewConnection([](size_t nodeId) {
        Serial.printf("New Connection %u\n", nodeId);
    });
    mesh.onDroppedConnection([](size_t nodeId) {
        Serial.printf("Dropped Connection %u\n", nodeId);
    });

    Serial.printf("ROOT:rootInitialization_END");
}

void setup() {
  Serial.begin(115200);
  userScheduler.addTask(rootInitializationTask);
  userScheduler.addTask(acknowledgementTask);
  rootInitializationTask.enable();
}

void loop() {
  userScheduler.execute(); // it will run mesh scheduler as well
  mesh.update();
}

void receivedCallback( uint32_t from, String &msg ) {
  // Serial.printf("logServer: Received from %u msg=%s\n", from, msg.c_str());
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(msg);
  if (root.containsKey("from"))
    {
      long trueFrom = root["found"];
      Serial.printf("logServer: Received from %u msg=%s\n", trueFrom, msg.c_str());
    }
  if (root.containsKey("found")) {
          long foundTargetNodeId = root["found"];
          int channel = root["chan"];
          signed rssi = root["rssi"];
          mac = root["mac"];
          Serial.printf("[FOUND] %d | %s | %i | %d \n", foundTargetNodeId, mac, channel, rssi);

        if(std::find(foundTargetNodeIds.begin(), foundTargetNodeIds.end(), foundTargetNodeId) == foundTargetNodeIds.end()) {
            foundTargetNodeIds.push_back(foundTargetNodeId);
          // signed rssi = root["rssi"];
          Serial.printf("[ADDED]NODE %u ADDED TARGET\n", foundTargetNodeId);
        }

        //checks if it is diabled and re-enables it. if it is out of iterations AND disabled, restart
        if( !acknowledgementTask.enableIfNot()){
            acknowledgementTask.restart();
        }
  }

  if (root.containsKey("fin_ack")){
      foundTargetNodeIds.erase(std::remove(foundTargetNodeIds.begin(), foundTargetNodeIds.end(), root["fin_ack"]), foundTargetNodeIds.end());
  }
}
