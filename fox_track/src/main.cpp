#include <painlessMesh.h>
#include "../lib/stringUtils.cpp"

#define   MESH_PREFIX     "Not_a_botnet"
#define   MESH_PASSWORD   "password"
#define   MESH_PORT       5566

//Define these
unsigned long ackSeconds = 3;
uint32_t ackTimes = 20;

// Prototype
Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;
uint16_t channel = 6;
int32_t maxRttDelay = 0;
unsigned long initDelay = 15000000;
uint32_t acknowledgementInterval = 900000; //ms
std::vector<long> foundTargetNodeIds;
const char* mac;
const char delimiter = ' ';


void receivedCallback( uint32_t from, String &msg );
void rootInitialization();
void acknowledgeTargets();
void disableAck();
void readSerialCommand();
String prepCommandForMesh(const String &command);

//Async functions
Task rootInitializationTask(TASK_IMMEDIATE, TASK_ONCE, &rootInitialization, &userScheduler);
Task acknowledgementTask(TASK_SECOND * ackSeconds, ackTimes, &acknowledgeTargets, &userScheduler, false, NULL, &disableAck);
Task readSerialTask (TASK_IMMEDIATE, TASK_FOREVER, &readSerialCommand, &userScheduler);

void acknowledgeTargets(){

    if (!foundTargetNodeIds.empty()){


        StaticJsonDocument<1024> msg;
        // JsonObject& msg = jsonBuffer.createObject();
        for (auto nodeId : foundTargetNodeIds) // access by reference to avoid copying
        {
            msg["foundack"] = nodeId;
            String str;
            serializeJson(msg, str);
            serializeJsonPretty(msg, Serial);
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
    StaticJsonDocument<16> msg;
    msg["initialize"] = (maxRttDelay*2) + mesh.getNodeTime();

    String str;
    serializeJson(msg, str);
    mesh.sendBroadcast(str);

    // log to serial
    Serial.printf("c2 initialization sent: ");
    serializeJsonPretty(msg, Serial);
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
  userScheduler.addTask(readSerialTask);
  readSerialTask.enable();
  rootInitializationTask.enable();
}

void loop() {
  userScheduler.execute(); // it will run mesh scheduler as well
  mesh.update();
}

void receivedCallback( uint32_t from, String &msg ) {
  // Serial.printf("logServer: Received from %u msg=%s\n", from, msg.c_str());
  StaticJsonDocument<100> root;
  deserializeJson(root, msg);

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

void readSerialCommand (){
  // Serial.printf("reading serial");
  if (Serial.available() > 0) {
    // Serial.printf("serial open...\n");
    char command[16];
    command[Serial.readBytesUntil('\n', command, 15)] = '\0';
    const String commandStr = command;
    if (commandStr.startsWith("tar") || commandStr.startsWith("rem")){
    // if (strcmp(command, "test") == 0) {
       // mesh.sendBroadcast();
       String msg = prepCommandForMesh(command);
       mesh.sendBroadcast(msg);
       // Serial.printf("%s\n",command);
    }
  }
}

String prepCommandForMesh(const String &command){
  // size_t pos = 0;
  // String token;
  StaticJsonDocument<25> comMsg;
  // while ((pos = command.indexOf(delimiter)) != -1) {
  //     token = command.substring(0, pos);
  //
  //     command.remove(0, pos + delimiter.length());
  //   }
  String commandAlias = getValue(command, delimiter, 0);
  String commandValue = getValue(command, delimiter, 1);
  comMsg[commandAlias] = commandValue.c_str();

  //for debugging
  serializeJsonPretty(comMsg, Serial);
  // Serial.println();
  String jsonStr;
  serializeJson(comMsg, jsonStr);
  return jsonStr;
}
