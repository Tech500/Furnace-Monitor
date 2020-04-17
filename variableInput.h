//
//   "Rain Gauge Two.ino" and  
//   variableInput.h //Place in Sketch folder
//   William M. Lucid   4/17/2020 @ 07:15 EDT    
// 

// Replace with your network details  
//const char * host  = "esp32";

// Replace with your network details
const char * ssid = "Removed";
const char * password = "Removed;


//publicIP accessiable over Internet with Port Forwarding; know the risks!!!
//WAN IP Address.  Or use LAN IP Address --same as server ip; no Internet access. 
//#define publicIP  "73.102.122.239"  //Part of href link for "GET" requests
//String LISTEN_PORT = "8030"; //Part of href link for "GET" requests



//Server settings
#define ip {172,16,0,7}
#define subnet {255,255,255,0}
#define gateway {172,16,0,1}
#define dns {172,16,0,1}

//FTP Credentials
const char * ftpUser = "Removed";
const char * ftpPassword = "Removed";
