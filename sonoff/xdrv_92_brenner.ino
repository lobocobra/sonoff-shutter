//lobocobra modifications in this tab
/*
  xsns_92_brenner.ino - Count the seconds your devices are ON

  Add-on by 

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



// #ifdef brenner

/*********************************************************************************************\
   Translations
  \*********************************************************************************************/
// Translation of the new commands
#define D_CMND_BRENNERttl             "BRENNERttl"
#define D_CMND_BRENNERcycle           "BRENNERcycle"
#define D_CMND_BRENNERoelstand        "BRENNERoelstand"
#define D_CMND_BRENNERoeltemp         "BRENNERoeltemp"
#define D_CMND_BRENNERoelverb1H       "BRENNERoelverb1H"
#define D_CMND_BRENNERoelmaxcapacity  "BRENNERoelmaxcapacity"
#define D_CMND_BRENNERoelmincapacity  "BRENNERoelmincapacity"
#define D_CMND_BRENNERcorrcyclesec    "BRENNERCorrCycleSec"
#define D_CMND_BRENNERoelprice100L    "BRENNERoelprice100L"
#define D_CMND_BRENNERPowerPrice1kwh  "BRENNERPowerPrice1kwh"
// MQTT Ausgabe
#define D_JSON_SEKperCyle_24h         "B_Sek/Cycle_24h"          // 
#define D_JSON_SEKperCycle_M1         "B_Sek/Cycle_Day-1"        // 
#define D_JSON_Brenner_OelstandL      "BrennerOelstandL"         // 
#define D_JSON_Brenner_OelstandFr     "BrennerOelstandFr"        // 
#define D_JSON_OelVerbLheute          "BrennerOelVerbLheute"     // 
#define D_JSON_OelVerbLgestern        "BrennerOelVerbLgestern"   // 
#define D_JSON_LC_Device_SecTTL       "BrennerSekTTL"            // 
#define D_JSON_LC_Device_Cycle        "BrennerCycles"            //




/*********************************************************************************************\
   Variables
\*********************************************************************************************/
enum BrennerCommands {CMND_BRENNERttl, CMND_BRENNERcycle, CMND_BRENNERoelstand, CMND_BRENNERoeltemp, CMND_BRENNERoelverb1H, CMND_BRENNERoelmaxcapacity, CMND_BRENNERoelmincapacity, CMND_BRENNERcorrcyclesec, CMND_BRENNERoelprice100L, CMND_BRENNERPowerPrice1kwh};
const char kBrennerCommands[] PROGMEM = D_CMND_BRENNERttl "|" D_CMND_BRENNERcycle  "|" D_CMND_BRENNERoelstand "|" D_CMND_BRENNERoeltemp "|" D_CMND_BRENNERoelverb1H "|" D_CMND_BRENNERoelmaxcapacity "|" D_CMND_BRENNERoelmincapacity  "|" D_CMND_BRENNERcorrcyclesec "|" D_CMND_BRENNERoelprice100L "|" D_CMND_BRENNERPowerPrice1kwh;
// Global variables
boolean LC_MS100 = false;

bool energy_max_reached = false;                 // did we reach the activation level (we were once higher than powerhigh)
bool LC_Device_Cycle_on = false;                 // Is Burner cyle on? (Needed to find next cyle
long LC_DEV_ONsec  = 0L;                         // Prevent counting of fake cycles an seconds when WATT PEAKS occur less than 10 secnds (sensor issues)

/*********************************************************************************************\
    Procedures / Functions
  \*********************************************************************************************/
void BrennerInit() {
  for (byte i = 0; i < devices_present; i++)  {
    //eliminate impossible values
    if ((Settings.LC_DEVhist_Sec_Period[0] > Settings.LC_DEVmeter_TTLSec ) || (Settings.LC_DEVhist_Sec_Period[0] ==0)){Settings.LC_DEVhist_Sec_Period[0] = Settings.LC_DEVmeter_TTLSec;}; // correct impossible value of history to prevent errors till daily reset
    if ((Settings.LC_DEVhist_Sec_Period[1] > Settings.LC_DEVmeter_TTLSec ) || (Settings.LC_DEVhist_Sec_Period[1] ==0)){Settings.LC_DEVhist_Sec_Period[1] = Settings.LC_DEVmeter_TTLSec;}; // correct impossible value of history to prevent errors till daily reset                                                                                                                                                               

    if ((Settings.LC_DEVhist_Cycle_Period[0] > Settings.LC_DEVmeter_TTLCycles ) || (Settings.LC_DEVhist_Cycle_Period[0] ==0)){Settings.LC_DEVhist_Cycle_Period[0] = Settings.LC_DEVmeter_TTLCycles;}; // correct impossible value of history to prevent errors till daily reset
    if ((Settings.LC_DEVhist_Cycle_Period[1] > Settings.LC_DEVmeter_TTLCycles ) || (Settings.LC_DEVhist_Cycle_Period[1] ==0)){Settings.LC_DEVhist_Cycle_Period[1] = Settings.LC_DEVmeter_TTLCycles;}; // correct impossible value of history to prevent errors till daily reset                                                                                                                                                               
  }
}

boolean ExecuteUpdateBrenner(byte device ) { // when a status is changing we count the ON seconds
  boolean result = true; //turn it false if you want to interrupt any start (over limit)

  return result;  
}

void Brenner_everySek() {

  // we need a counter to have 1/10 of a second exact time


//lobocobra: increase Device uptime every second after powerhigh was reached and we do not drop below powerlow
      if (energy_power > Settings.energy_max_power || energy_power > Settings.energy_min_power && energy_max_reached == true) { 
        energy_max_reached = true;                // ok we passed activation level and we count until we are below powerlow
        LC_DEV_ONsec++;                           // prevent POWERLOW  peaks, count the seconds of this cycle to prevent peaks
        Settings.LC_DEVmeter_TTLSec++;            // Increase the seconds, device over Highpower in seconds in total
          if (LC_Device_Cycle_on == false){
              Settings.LC_DEVmeter_TTLCycles++;   // increase cycle counter
              LC_Device_Cycle_on = true;          // prevent increase on same cycle
          } // increase cycle if it is a new cycle        
      } //powerhigh is used to set the trigger but we only do NOT enter here if we are also below PowerLOW
      else {
             LC_Device_Cycle_on = false;  // we dropped below powerlow
             energy_max_reached = false;  // we want to be ready for next powerhigh
             if (( LC_DEV_ONsec >= 1 ) && (LC_DEV_ONsec <= 30 ) ) {   // POWERHIGH was less than 31 sec on, so it is a false alert
                //let's reverse all countings as the cycle was a false alert due to a WATT Peak
                Settings.LC_DEVmeter_TTLSec -= LC_DEV_ONsec;       // remove fake seconds
                Settings.LC_DEVmeter_TTLCycles-- ;                 // lower cycle by 1 as there was a fake start and we remove all fake data
             } 
     LC_DEV_ONsec =0;              // prevent POWERHIGH peaks, as we are on powerlow, powerhigh is 0 
     }

//at midnight, we reset the numbers
    if (RtcTime.valid) {
      //Serial.print("RTCtime.valid:");Serial.print(RtcTime.valid);Serial.print(" Midnight:");Serial.print(Midnight()); Serial.print(" LocalTime:");Serial.println(LocalTime());    
      if (LocalTime() == Midnight()) {
        // put here the code to 0 the day
        // TTL sec                   
        Settings.LC_DEVhist_Sec_Period[1] = Settings.LC_DEVhist_Sec_Period[0];       // save what is now yesterday
        Settings.LC_DEVhist_Sec_Period[0] = Settings.LC_DEVmeter_TTLSec;             // save new today start point, I know that this value is not corrected, we will do it when we print, if we update, we have yesterday and today corrected
        // TTL cycles
        Settings.LC_DEVhist_Cycle_Period[1] = Settings.LC_DEVhist_Cycle_Period[0];  // save what is now yesterday
        Settings.LC_DEVhist_Cycle_Period[0] = Settings.LC_DEVmeter_TTLCycles;       // save new today start point
      }
    }
}

void Brenner_every100ms() {


}


void Brenner_every50ms() {
  //execute every 2nd round 100ms
  if ( LC_MS100 ^= true )  Brenner_every100ms();
}


boolean MqttBrennerCommand() //react ont received MQTT commands
{
  char command [CMDSZ];
  byte i = 0;
  byte index; // index
  boolean serviced = true;
  long    payload = -99L;           //no payload
  double  payload_double = -99.0;   //no payload
  char    payload_char[XdrvMailbox.data_len+1];  //create the string variable (+1 needed for termination "0")
      
  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kBrennerCommands);
  index = XdrvMailbox.index;
  if (XdrvMailbox.data_len > 0 && XdrvMailbox.data_len <= 12 ) { //get the MQTT message and as the original value is INT only, we read the data ourselves
    strlcpy(payload_char,  XdrvMailbox.data, XdrvMailbox.data_len+1); 
    payload_double = CharToDouble(payload_char); //we transform it into double, just in case  // use internal function as atof(), takes too much space
    payload = round(payload_double);             //we round the double, in case we want integer
  } else payload = XdrvMailbox.payload; // unclear why I made this... probably to avoid we loose data?

  BrennerInit(); // ensure that we init the values once a while, which is the case when we have commands
  
  if (-1 == command_code || index <= 0 || index > devices_present ) { // -1 means that no command is recorded with this value OR the INDEX is out of range or we have less devices than we address
    serviced = false;  // Unknown command
  }
  else if (CMND_BRENNERttl == command_code) {
    Serial.print("Payload: \t");Serial.println(payload);
    Serial.print("TTLsec: \t");Serial.println(Settings.LC_DEVmeter_TTLSec);
    if (payload >= 0 && payload <= 2147483647)  Settings.LC_DEVmeter_TTLSec = payload; // only load new data, if it is within range
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_LVALUE, command, Settings.LC_DEVmeter_TTLSec ); 
  }
  else if (CMND_BRENNERcycle == command_code) {
    if (payload >= 0 && payload <= 2147483647) Settings.LC_DEVmeter_TTLCycles = payload;
   snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_LVALUE, command, Settings.LC_DEVmeter_TTLCycles );
  }
  else if (CMND_BRENNERoelstand == command_code) {
    if ((payload >= Settings.LC_Config_OilMinCapacity) && (payload <= Settings.LC_Config_OilMaxCapacity)) { // if it is a new value it must be within Capacity limits
        Settings.LC_DEVmeter_TTLOil = payload; // ok we received a value, so we overwrite the existing, but only if it is something that makes sence and is bigger than minimal setting
        } 
    if (payload == -1) snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_LVALUE, command, Settings.LC_DEVmeter_TTLOil ); // we show the startpoint of calculation
    else //we calculate the value
                                                                                                                                      // float to get digits                 remove the correction factor as the burner starts later            calculate hours * usage
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_LVALUE, command, Settings.LC_DEVmeter_TTLOil - round((((float)Settings.LC_DEVmeter_TTLSec -(Settings.LC_Config_CorrCycleSec * Settings.LC_DEVmeter_TTLCycles)) / 3600) * Settings.LC_Config_OilConsHour) ); // float is needed as you want to divide an integer, if not it gets cut
  }
  else if (CMND_BRENNERoeltemp == command_code) { // not yet used 
    if (payload_double >= 0) { // temp in house can not be below 0
      Settings.LC_DEVmeter_OilTemp = payload_double;
     }
     char buffer[16]; // Arduino does not support %f, so we have to use dtostrf
     snprintf_P(mqtt_data, sizeof(mqtt_data),PSTR("{\"%s\":%s}"), command,  dtostrf(Settings.LC_DEVmeter_OilTemp,XdrvMailbox.data_len+1,2,buffer) ); // we handle float here
  }
    else if (CMND_BRENNERoelverb1H == command_code) {
    if (payload_double >= 0) {
      Settings.LC_Config_OilConsHour = payload_double; //without float its rounded to 1 / 2 etc....
     }
     char buffer[16]; // Arduino does not support %f, so we have to use dtostrf
     snprintf_P(mqtt_data, sizeof(mqtt_data),PSTR("{\"%s\":%s}"), command, dtostrf(Settings.LC_Config_OilConsHour,XdrvMailbox.data_len+1,4,buffer) ); // we handle float here
  }  
    else if (CMND_BRENNERoelmaxcapacity == command_code) {
    if (payload >= 0) {
      Settings.LC_Config_OilMaxCapacity = payload;
     }
   snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_LVALUE, command, Settings.LC_Config_OilMaxCapacity );     
  } 
    else if (CMND_BRENNERoelmincapacity == command_code) {
    if (payload >= 0) {
      Settings.LC_Config_OilMinCapacity = payload;
     }
   snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_LVALUE, command, Settings.LC_Config_OilMinCapacity );     
  }
    else if (CMND_BRENNERcorrcyclesec == command_code) {
    if ((payload_double >= 0) && (payload_double <= 3600) ){ //more than 1h is not needed
      Settings.LC_Config_CorrCycleSec = payload_double;
     }
     char buffer[16]; // Arduino does not support %f, so we have to use dtostrf
     snprintf_P(mqtt_data, sizeof(mqtt_data),PSTR("{\"%s\":%s}"), command, dtostrf(Settings.LC_Config_CorrCycleSec,XdrvMailbox.data_len+1,2,buffer) ); // we handle float here
  }
    else if (CMND_BRENNERoelprice100L == command_code) {
    if (payload_double >= 0) {
      Settings.LC_Config_OilPrice100L = payload_double;
     }
     char buffer[16]; // Arduino does not support %f, so we have to use dtostrf
     snprintf_P(mqtt_data, sizeof(mqtt_data),PSTR("{\"%s\":%s}"), command, dtostrf(Settings.LC_Config_OilPrice100L,XdrvMailbox.data_len+1,2,buffer) ); // we handle float here

  }  
    else if (CMND_BRENNERPowerPrice1kwh == command_code) {
    if (payload_double >= 0) {
      Settings.LC_Config_PowerPrice1kwh = payload_double;
     }
     char buffer[16]; // Arduino does not support %f, so we have to use dtostrf
     snprintf_P(mqtt_data, sizeof(mqtt_data),PSTR("{\"%s\":%s}"), command,  dtostrf(Settings.LC_Config_PowerPrice1kwh,XdrvMailbox.data_len+1,2,buffer) ); // we handle float here     
  } 
    else { // no response for known command was defined, so we handle it as unknown
    serviced = false;  // Unknown command
  }  
    
  return serviced;
}

//lobocobra start
long lobo_calcOelstand(boolean Liter){ //0= calculate liter and 1=calculate $
     long result;
     result = Settings.LC_DEVmeter_TTLOil - round( (( (float)Settings.LC_DEVmeter_TTLSec - (Settings.LC_Config_CorrCycleSec * Settings.LC_DEVmeter_TTLCycles)) / 3600) * Settings.LC_Config_OilConsHour ); // float is needed as you want to divide an integer, if not it gets cut
          
     if (Liter) { //we calculate liter
     return result;
     } 
     else {  //we calculate $
        result = result*Settings.LC_Config_OilPrice100L; // float is needed as you want to divide an integer, if not it gets cut
        result = round((float)result/100);
     }     

     if (result < 0) {result =0;}; // just in case something went wrong... we never go below 0
     return result;      
}

long lobo_calcOelVerbrauch(long TTLsec, long TTLcycle, boolean R_Type ){ //0= Corrected sec, 1 = liter, 2 = $, 3 = oil left in tank
     long result;
     float KorrSec;

    
     KorrSec = round( (float)TTLsec - (Settings.LC_Config_CorrCycleSec * TTLcycle) ); // correct brenner sekunden 

     if      (R_Type == 1) { //we calculate liter
        result = round( KorrSec/3600 * Settings.LC_Config_OilConsHour ); // float is needed as you want to divide an integer, if not it gets cut
     } 
     else if (R_Type == 2) {  //we calculate $
        result = round( KorrSec/3600 * Settings.LC_Config_OilConsHour * ( (float)Settings.LC_Config_OilPrice100L / 100) ); // float is needed as you want to divide an integer, if not it gets cut
     }
     else {
     result = KorrSec; // must be plain seconds
     }   
     if (result < 0) {
         result = 0;
     }; // just in case something went wrong... we never go below 0     
return result;     
}

//lobocobra end
 
void ShowBrennerPos(byte relay_i, uint8_t type_i) { 
  // INDEX99 show all devices
  // TYPE 2) STAT 3) SENSOR

  //16:15:09 MQT: tele/ug_heizung_brenner/SENSOR = {"Time":"2018-06-17T16:15:09","ENERGY":{"Total":355.872,"Yesterday":0.413,"Today":0.161,"Period":1,"Power":10,"Factor":0.39,"Voltage":231,"Current":0.107,"BrennerSek24h":0,"BrennerSekDay-1":1462,"BrennerOelstandL":3945, "BrennerOelstandFr":3086,"BrennerSekTTL":349817,"BrennerCycles":1264,"BrennerOelVerbLheute":0,"BrennerOelVerbLgestern":1}} (retained)
  //"BrennerSek24h":0,"BrennerSekDay-1":1462,"BrennerOelstandL":3945, "BrennerOelstandFr":3086,"BrennerSekTTL":349817,"BrennerCycles":1264,"BrennerOelVerbLheute":0,"BrennerOelVerbLgestern":1
  // BrennerSekTTL BrennerCycles BrennerOelstandL BrennerOelstandFr

snprintf_P(mqtt_data, sizeof(mqtt_data),PSTR("%s,\"BRENNER\":{\"%s\":%d,\"%s\":%d,\"%s\":%d,\"%s\":%d,\"%s\":%d,\"%s\":%d,\"%s\":%d,\"%s\":%d}"), 
        mqtt_data,
        D_JSON_LC_Device_SecTTL, Settings.LC_DEVmeter_TTLSec - round(Settings.LC_DEVmeter_TTLCycles*Settings.LC_Config_CorrCycleSec),  //we want to show the corrected time
        D_JSON_LC_Device_Cycle, Settings.LC_DEVmeter_TTLCycles,
        D_JSON_Brenner_OelstandL, lobo_calcOelstand(1),
        D_JSON_Brenner_OelstandFr, lobo_calcOelstand(0),
        D_JSON_OelVerbLheute,   lobo_calcOelVerbrauch(Settings.LC_DEVmeter_TTLSec - Settings.LC_DEVhist_Sec_Period[0], Settings.LC_DEVmeter_TTLCycles - Settings.LC_DEVhist_Cycle_Period[0] ,1),
        D_JSON_OelVerbLgestern, lobo_calcOelVerbrauch(Settings.LC_DEVmeter_TTLSec - Settings.LC_DEVhist_Sec_Period[1], Settings.LC_DEVmeter_TTLCycles - Settings.LC_DEVhist_Cycle_Period[1] ,1),
        D_JSON_SEKperCyle_24h, (Settings.LC_DEVhist_Cycle_Period[0]!=0 ? round((float)(Settings.LC_DEVhist_Sec_Period[0] - round(Settings.LC_DEVhist_Cycle_Period[0]*Settings.LC_Config_CorrCycleSec)) / Settings.LC_DEVhist_Cycle_Period[0]) : 0), //we correct the history data and remove warm-up times
        D_JSON_SEKperCycle_M1, (Settings.LC_DEVhist_Cycle_Period[1]!=0 ? round((float)(Settings.LC_DEVhist_Sec_Period[1] - round(Settings.LC_DEVhist_Cycle_Period[1]*Settings.LC_Config_CorrCycleSec)) / Settings.LC_DEVhist_Cycle_Period[1]) : 0)  //we correct the history data and remove warm-up times



//        D_Relay_Consumption, index, Settings.relayCurrency[index-1],dtostrf(relayconsumptioncost,Rlength[1]+1+4,2,Rbuffer2),
//        D_Relay_Consumption, index, D_CMND_relayOnDsec, Settings.relayOnDsec[index - 1] + (type_i==3 ? 0: RelayRuntime),    //type_i, when we tele, then we need to add current values, if STAT, then we are launched by OFF trigger, so its not needed
//        D_Relay_Consumption, index, D_CMND_relayStarts, Settings.relayStarts[index - 1] + (type_i==3 ? 0: (RelayRuntime>0)) //type_i, when we tele, then we need to add current values, if STAT, then we are launched by OFF trigger, so its not needed
        );    
  

/*         
#define D_JSON_LC_Device_Cycle      "BrennerCycles"  
  long          LC_DEVmeter_TTLSec;        // Meter of total seconds above Highvalue since last manual oil mesurement. Reset to 0 if you enter new Oil level.     
  long          LC_DEVmeter_TTLCycles;     // Burning cycles... see above. Reset to 0 if you enter new Oil level.
  long          LC_DEVmeter_TTLOil;        // Liter of oil left in tank measured the last time manually. It will not be updated unless you enter a new value.
  float         LC_Config_OilConsHour;     // how much oil is used per hour in average
  float         LC_Config_CorrCycleSec;    // correct each cycle by X seconds / negative value will be reduced, positive added / maybe your device has a worm-up time?
  long          LC_Config_OilMaxCapacity;  // how much oil can you put in your tank
  long          LC_Config_OilMinCapacity;  // how much oil is the minimum before we shut-down the Brenner (it would damage the Brenner)
  long          LC_DEVhist_Sec_Period[2];  // Array with history data about Device SEC TTL / TodaySummed=0, yesterday=1 etc / I save the data so after a outage/reset we go on as before without data loss
  long          LC_DEVhist_Cycle_Period[2];// Array with history data about Device SEC TTL / TodaySummed=0, yesterday=1 etc / I save the data so after a outage/reset we go on as before without data loss
  float         LC_DEVmeter_OilTemp;       // Temp of the oil in the tank... will be used to calculate liter at 15° Celsius
  float         LC_Config_OilPrice100L;    // Price per 10000l Oil, so we can add 2 digits
  float         LC_Config_PowerPrice1kwh;  // Price per 100kw, so we can add 2 digits
*/


}

/*********************************************************************************************\
   Interface
\*********************************************************************************************/
#define XDRV_92

boolean Xdrv92(byte function){
  boolean result = false;
  
  if (Settings.flag3.brennermode) {
    switch (function) {
      case FUNC_INIT:
        BrennerInit();
        break;
      case FUNC_JSON_APPEND:
        ShowBrennerPos(1,2); //99 = show all relay
        //MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_STATE), Settings.flag.mqtt_sensor_retain); // after data is loaded to mqtt_data, we force the send out of the message
        break;
      case FUNC_COMMAND:
        result = MqttBrennerCommand();
        break;
      case FUNC_EVERY_SECOND:
        Brenner_everySek();
        break;
      case FUNC_EVERY_50_MSECOND:
        Brenner_every50ms();
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

//#endif  // brenner

