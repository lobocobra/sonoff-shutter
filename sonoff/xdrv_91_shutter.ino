/*
  xsns_98_shutter.ino - Ooperate your shutter

  Add-on by lobocobra

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/



// #ifdef shutter

/*********************************************************************************************\
   Translations
  \*********************************************************************************************/
// Translation of the new commands
// Commands Shutter
#define D_CMND_shutterPosDSec     "ShutterPosDSec"
#define D_CMND_shutStartDelayDSec "ShutStartDelayDSec"
#define D_CMND_shutLagUpwardsDSec "ShutLagUpwardsDSec"
#define D_CMND_shutterMinDSec     "ShutterMinDSec"
#define D_CMND_shutterMax_Sec     "ShutterMaxDSec"
#define D_CMND_shutterGoPercent   "ShutterGoPercent"
#define D_SHUT_PERCENT1           "Position Shutter 1 in %"
#define D_SHUT_PERCENT2           "Position Shutter 2 in %"
#define D_SHUTTER1                "SHUTTER1"
#define D_SHUTTER2                "SHUTTER2"

/*********************************************************************************************\
   Variables
  \*********************************************************************************************/

enum ShutterCommands { CMND_shutterPosDSec, CMND_shutStartDelayDSec, CMND_shutLagUpwardsDSec, CMND_shutterMinDSec, CMND_shutterMaxDSec, CMND_shutterGoPercent };
const char kShutterCommands[] PROGMEM = D_CMND_shutterPosDSec "|" D_CMND_shutStartDelayDSec "|" D_CMND_shutLagUpwardsDSec "|" D_CMND_shutterMinDSec "|" D_CMND_shutterMax_Sec "|" D_CMND_shutterGoPercent;

const char JSON_SHUTTER_PERCENT[] PROGMEM = "%s,\"%s\":%d";

// Global variables
int shutDeciSecMem[2] = {0L}; // counts up the time all 1/10 seconds 0=endless timer, 1=save for device1 2=save for device2
unsigned long shutDeciSecCounter = 0L;         // a counter to calculate how long the shutter did run, might be replaced by UTC
boolean MS100 = false;





/*********************************************************************************************\
    Procedures / Functions
  \*********************************************************************************************/
void ShutterInit() {
  for (byte i = 0; i < 2; i++)  {
    //eliminate impossible values
    if ( Settings.shutterPosMaxDeciSec[i] <= 0 || Settings.shutterPosMaxDeciSec[i] >= 3000) Settings.shutterPosMaxDeciSec[i] = 150;
    if (Settings.shutterPosMinDeciSec[i] >= round((float)Settings.shutterPosMaxDeciSec[i] / 2)  ) Settings.shutterPosMinDeciSec[i] = 0;
    if (Settings.shutterPosCurrentDeciSec[i] >= Settings.shutterPosMaxDeciSec[i] + 100) Settings.shutterPosCurrentDeciSec[i] = Settings.shutterPosMaxDeciSec[i];
    if (Settings.shutterStartDelayDeciSec[i] >= round((float)Settings.shutterPosMaxDeciSec[i] / 4)  ) Settings.shutterStartDelayDeciSec[i] = 0;
    if (Settings.shutterLagUpwardsDeciSec[i] >= round((float)Settings.shutterPosMaxDeciSec[i] / 4)  ) Settings.shutterLagUpwardsDeciSec[i] = 0;
  }
}

void ExecutePowerUpdateShutterPos(byte device ) { // when a status is changing update the shuter pos
  int shutRuntime = 0;
  // update current position of the shutter after it is stopped, it is triggered by sonoff.ino when a power command is executed
  for (byte i = 1; i <= devices_present; i++) {
    byte mask = 0x01 << (i - 1); //set the mask to the right device
    if ((device == i) && ((power & mask) != mask ) ) {
      shutDeciSecMem[(i > 2)] = shutDeciSecCounter;  //Power was OFF => we are about to start, so I save start time
    }
    // now check if the current device is the right one and already ON and will go off => then calculate position
    // odd=up
    if (device & 0x01)  {
      //odd=up
      //Serial.print("We go up device is:");Serial.print(device);Serial.print(" power is:");Serial.println(power);
      if ((device == i) && (power & mask)) { //Power was ON => turned off, calculate new position and apply corrections
        shutRuntime = shutDeciSecCounter - shutDeciSecMem[(i > 2)];  // we calculate run duration, remove start delay, add lag Upwards - Settings.shutterStartDelayDeciSec[(i > 2)];
        shutRuntime = shutRuntime - (round((float)Settings.shutterLagUpwardsDeciSec[(i > 2)] / (Settings.shutterPosMaxDeciSec[(i > 2)] + Settings.shutterLagUpwardsDeciSec[(i > 2)]) * shutRuntime)  - Settings.shutterStartDelayDeciSec[(i > 2)]); //add delays like lag up/down and delayed start
        if (shutRuntime < 0) shutRuntime = 0;     // correct value if we were not ON long enough to move the shutter
        Settings.shutterPosCurrentDeciSec[(i > 2)] = Settings.shutterPosCurrentDeciSec[(i > 2)] - shutRuntime;
        if (Settings.shutterPosCurrentDeciSec[(i > 2)] < 0) Settings.shutterPosCurrentDeciSec[(i > 2)] = 0; // set the shutter back to 0 (as we are slower up, we need to go to neg value to go fully up
      }
    }
    else {
      //even=down
      //Serial.print("We go down device is:");Serial.print(device);Serial.print(" power is:");Serial.println(power);
      if ((device == i) && (power & mask)) { //Power is ON => turn it off, calculate new position
        shutRuntime = shutDeciSecCounter - shutDeciSecMem[(i > 2)] - Settings.shutterStartDelayDeciSec[(i > 2)] ; // we calculate, how long it did run and remove the delay
        if (shutRuntime < 0) {
          shutRuntime = 0;  // correct value if we were not ON long enough to move the shutter
        }
        Settings.shutterPosCurrentDeciSec[(i > 2)] = Settings.shutterPosCurrentDeciSec[(i > 2)] + shutRuntime; 
      }
    }
  }
  //Serial.print("ExecutePowerFunc Called for  Device:");Serial.print(device);Serial.print(" counter@:");Serial.print(shutDeciSecCounter);Serial.print(" Runtime@:");Serial.println(shutRuntime);
}

void Shutter_everySek() {
  /*  if (power = 0){  // we reset the counter always if no power is on, so we do not have an overflow problem
      shutDeciSecCounter = 0L;
      shutDeciSecMem[0] = 0L;
      shutDeciSecMem[1] = 0L;
    }
    Serial.print("Power status:");Serial.print(power);Serial.print(" Mem is:");Serial.println(shutDeciSecMem[0]);          */
}

void Shutter_every100ms() {
  // we need a counter to have 1/10 of a second exact time
  //    Serial.print("    DeciCounter:");Serial.println(shutDeciSecCounter);
  shutDeciSecCounter++;

  // control current position and stop mootor if we are outside min/max
  for (byte i = 0; i < devices_present; i++) {
    byte mask = 0x01 << (i); //set the mask to the right device
    if ( power & mask ) {
      // odd=up
      if (i == 0  || i == 2)  { // we go up, if we do not use (int) then we have an overflow!!!
        //Serial.print("Diff:");Serial.print((shutDeciSecCounter - shutDeciSecMem[i*2 >2]) );Serial.print(" Pos:");Serial.print(Settings.shutterPosCurrentDeciSec[i*2 >2]);Serial.print(" if: ");Serial.print(( (int)(Settings.shutterPosCurrentDeciSec[i*2 >2] - (shutDeciSecCounter - shutDeciSecMem[i*2 >2]) )));Serial.print("<=");Serial.println(-(round( ((float)Settings.shutterLagUpwardsDeciSec[i > 2] / (Settings.shutterPosMaxDeciSec[i > 2]+Settings.shutterLagUpwardsDeciSec[i > 2])* (shutDeciSecCounter - shutDeciSecMem[i*2 >2])) )) )  ;
        int shutX = shutDeciSecCounter - shutDeciSecMem[i * 2 > 2];
        if  ( (int)(Settings.shutterPosCurrentDeciSec[i * 2 > 2] - shutX ) <=  -(round( ((float)Settings.shutterLagUpwardsDeciSec[(i > 2)] / (Settings.shutterPosMaxDeciSec[i > 2] + Settings.shutterLagUpwardsDeciSec[i > 2])* shutX) ) + Settings.shutterStartDelayDeciSec[i > 2] - Settings.shutterPosMinDeciSec[i > 2]))  {
          ExecuteCommandPower(i + 1, 0, SRC_IGNORE);
        }
      }
      else { //we go down, and stop if we are over Max
        //Serial.print("    Position:");Serial.println(Settings.shutterPosCurrentDeciSec[i*2 >2]);
        if  ( Settings.shutterPosCurrentDeciSec[i * 2 > 2] + (shutDeciSecCounter - shutDeciSecMem[i * 2 > 2])  >=  Settings.shutterPosMaxDeciSec[i * 2 > 2] ) {
          ExecuteCommandPower(i + 1, 0, SRC_IGNORE);
        }
      }
    }
  }
}


void Shutter_every50ms() {
  //execute every 2nd round 100ms
  if ( MS100 ^= true )  Shutter_every100ms();
}


boolean MqttShutterCommand()
{
  char command [CMDSZ];
  uint16_t i = 0;
  uint16_t index; // index
  boolean serviced = true;
  boolean valid_entry = false;
  uint16_t payload = -99;

  snprintf_P(log_data, sizeof(log_data), PSTR("Shutter: Command Code: '%s', Payload: %d"), XdrvMailbox.topic, XdrvMailbox.payload);
  AddLog(LOG_LEVEL_DEBUG);
  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kShutterCommands);
  index = XdrvMailbox.index;
  payload = XdrvMailbox.payload;

  //Serial.print("command_index:") ;Serial.print(XdrvMailbox.index) ;Serial.print(" CMND_shutterMinDSec:") ;Serial.print(CMND_shutterMinDSec) ;Serial.print(" Mailbox:") ;Serial.print(XdrvMailbox.topic) ;Serial.print(" MailboxPayload:") ;Serial.print(XdrvMailbox.payload) ;Serial.print(" command:") ;Serial.println(command) ;
  if (-1 == command_code || index <= 0 || index > 2 || index*2 > devices_present  ) { // -1 means that no command is recorded with this value OR the INDEX is out of range or we have less devices than we address
    serviced = false;  // Unknown command
  }
  else if (CMND_shutterMinDSec == command_code) {
    if (payload >= 0 && payload <= round((float)Settings.shutterPosMaxDeciSec[index - 1] / 2)  ) Settings.shutterPosMinDeciSec[index - 1] = payload;
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.shutterPosMinDeciSec[index - 1]);
  }
  else if (CMND_shutterMaxDSec == command_code) {
    if (payload >= 0 && payload <= 3000 ) Settings.shutterPosMaxDeciSec[index - 1] = payload;
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.shutterPosMaxDeciSec[index - 1]);
  }
  else if (CMND_shutterPosDSec == command_code) {
    if (payload >= 0 && payload <= 3000 ) Settings.shutterPosCurrentDeciSec[index - 1] = payload;
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.shutterPosCurrentDeciSec[index - 1]);
  }
  else if (CMND_shutterGoPercent == command_code) {
    if (payload >= 0 && payload <= 100 ) {
      ExecuteCommandPower(index * 2 - 1, 0, SRC_IGNORE);                 // stop the relay to ensure the position is updated. after that you can change direction as wanted.
      // calculate and set desired position
      int shutterPosRequested = round(payload * (float)Settings.shutterPosMaxDeciSec[index - 1] / 100);
      //calculate distance from current position, to wanted position
      int shut_diff_DeciSec = shutterPosRequested - Settings.shutterPosCurrentDeciSec[index - 1] + Settings.shutterStartDelayDeciSec[(i > 2)];
      int shut_diff_DeciSec_up = shut_diff_DeciSec + round( ((float)Settings.shutterLagUpwardsDeciSec[(i > 2)] / Settings.shutterPosMaxDeciSec[(i > 2)] * shut_diff_DeciSec) ); // add % of the upLag to the distance to go

      if (shut_diff_DeciSec < 0 ) shut_diff_DeciSec = -shut_diff_DeciSec ; // we want a positive value
      if (shut_diff_DeciSec > Settings.shutterPosMaxDeciSec[index - 1]) shut_diff_DeciSec = Settings.shutterPosMaxDeciSec[index - 1] + Settings.shutterStartDelayDeciSec[(i > 2)]; // eliminate ridiculous values
      if (shut_diff_DeciSec_up < 0 ) shut_diff_DeciSec_up = -shut_diff_DeciSec_up ; // we want a positive value
      if (shut_diff_DeciSec_up > Settings.shutterPosMaxDeciSec[index - 1]) shut_diff_DeciSec_up = Settings.shutterPosMaxDeciSec[index - 1] + Settings.shutterStartDelayDeciSec[(i > 2)]; // eliminate ridiculous values

      //transform DeciSec into pulsetime sec
      if (shut_diff_DeciSec >  111) shut_diff_DeciSec =  round(((float)shut_diff_DeciSec / 10) + 100);
      // set pulsetime and and turn-own relay
      if (shutterPosRequested > Settings.shutterPosCurrentDeciSec[index - 1]) { // move shutter down
        Settings.pulse_timer[index * 2 - 1] = shut_diff_DeciSec;
        pulse_timer[index * 2 - 1] = 0;                                    // set count-up counter to 0
        ExecuteCommandPower(index * 2 - 0, 1, SRC_IGNORE);                 // activate relay
      } else {                                                              // move shutter up
        if ( shutterPosRequested != Settings.shutterPosCurrentDeciSec[index - 1]) { // do not react, if we are already on the right place
          //add delays like lag up/down and delayed start
          Settings.pulse_timer[index * 2 - 2] = shut_diff_DeciSec_up;
          pulse_timer[index * 2 - 2] = 0;                                    // set count-up counter to 0
          ExecuteCommandPower(index * 2 - 1, 1, SRC_IGNORE);                 // activate relay
        }
      }
    } else { //we give the position in %
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, round((float)Settings.shutterPosCurrentDeciSec[index - 1] / Settings.shutterPosMaxDeciSec[index - 1] * 100) );
    }
  }
  else if (CMND_shutStartDelayDSec == command_code) {
    if (payload >= 0 && payload <= round((float)Settings.shutterPosMaxDeciSec[index - 1] / 4)  ) Settings.shutterStartDelayDeciSec[index - 1] = payload;
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.shutterStartDelayDeciSec[index - 1]);
  }
  else if (CMND_shutLagUpwardsDSec == command_code) {
    if (payload >= 0 && payload <= round((float)Settings.shutterPosMaxDeciSec[index - 1] / 4)  ) Settings.shutterLagUpwardsDeciSec[index - 1] = payload;
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.shutterLagUpwardsDeciSec[index - 1]);
  }
  else { // no response for known command was defined, so we handle it as unknown
    serviced = false;  // Unknown command
  }
  return serviced;
}

void Shutter_MQTT_Position()
{
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_OSWATCH " Position %d, Percent %d, Max %d"), Settings.shutterPosCurrentDeciSec[0], round((float)Settings.shutterPosCurrentDeciSec[0] / Settings.shutterPosMaxDeciSec[0] * 100) , Settings.shutterPosMaxDeciSec[0]);
  AddLog(LOG_LEVEL_INFO);

}


/*********************************************************************************************\
   Interface
\*********************************************************************************************/
#define XDRV_91

boolean Xdrv91(byte function){
  boolean result = false;
  
  if (Settings.flag3.shuttermode) {
    switch (function) {
      case FUNC_INIT:
        ShutterInit();
        break;
      case FUNC_JSON_APPEND:
        // {"Time":"2018-07-17T23:06:46","Position Shutter 1 in %":49,"Position Shutter 2 in %":30}
        snprintf_P(mqtt_data, sizeof(mqtt_data), JSON_SHUTTER_PERCENT, mqtt_data, D_SHUT_PERCENT1, round((float)Settings.shutterPosCurrentDeciSec[0]/ Settings.shutterPosMaxDeciSec[0]*100) ); // it does what it says (Json append), it adds it to mqtt
        if (devices_present>2) snprintf_P(mqtt_data, sizeof(mqtt_data), JSON_SHUTTER_PERCENT, mqtt_data, D_SHUT_PERCENT2, round((float)Settings.shutterPosCurrentDeciSec[1]/ Settings.shutterPosMaxDeciSec[1]*100) );
        break;
      case FUNC_COMMAND:
        result = MqttShutterCommand();
        break;
      case FUNC_EVERY_SECOND:
        Shutter_everySek();
        break;
      case FUNC_EVERY_50_MSECOND:
        Shutter_every50ms();
        break;        
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        ;
        break;
#endif  // USE_WEBSERVER
      case FUNC_SAVE_BEFORE_RESTART:
        ;
    }
  }
  return result;
}

//#endif  // shutter

