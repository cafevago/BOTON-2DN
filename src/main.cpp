
// Version 2DN para boton industrial- Visualización de dirección y estado por cinta led inteligente externa de 3 pixeles, actualización de dirección manteniendo pulsado al encendido y guardada en memoria
// Actualización de firmware por OTA
// Falta por eliminar lo que no es necesario para un botón (display, manejho de batería, tactiles, etc)

//REFERENCIAS:
//https://wokwi.com/projects/307248583887815233
//https://github.com/RalphBacon/224-Superior-Serial.print-statements/tree/main/Simple_Example
//https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/
//https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
//https://forum.arduino.cc/t/convert-wifi-localip-to-string/1088380/10
//https://stackoverflow.com/questions/55804381/how-do-i-convert-wifi-localip-to-string-and-store-it-to-external-variable
//https://www.luisllamas.es/esp32-uart/
//https://simplyexplained.com/courses/programming-esp32-with-arduino/using-rtc-memory/




/* FORMATO GENERAL TRAMA DE DATOS: 

[$AA][$AB] // ENCABEZADO 
[$80] //DESTINO APLICACIÓN 
[$1] //ORIGEN # TERMINAL 
[$2] // NUMERO DE TRAMA 
[$1] // LONGITUD DE DATOS 
[$5] //DATOS,COMANDO 
[$89] // CHECKSUM: SE SUMAN LOS BYTES DESDE DESTINO HASTA COMANDO O DATOS */

//********************************************************************************************************************************************************************
//BLOQUE QUE PREPARA LA ACTUALIZACIÓN OTA
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <HardwareSerial.h>

#include <FastLED.h>  //Libreria para el manejo de la cinta RGB inteligente
#define NUM_LEDS 3  //Numero de leds (pueden ser hasta 500 leds)  En ciertos casos es posible definir más por si se quedan encendidos los últimos
#define DATA_PIN 23   //Salida de datos cable verde    pin 11 en UNO / pin 16 en MICRO / pin 12 en NANO / pin 23 en ESP32
CRGB leds[NUM_LEDS];  //Crea un arreglo para la manipulación de los leds
//#include "data.h"

//HardwareSerial MySerial(1); // definir un Serial para UART1
//const int MySerialRX = 9;
//const int MySerialTX = 10;

//const int MySerialRX = 18;
//const int MySerialTX = 19;

#define RXD2 18   // Cable naranja
#define TXD2 19   // Cable amarillo
//****
// Xbbe  naranja TX
//       amarillo rx
//       azul 3v
// BOTON   Verde GND
//         AMARILLO 17
// 



const char* host = "WCiel";         //Nombre de la terminal actuando como web server
//const char* ssid = "Fer1972";       //192.168.141.107  --  192.168.0.230
//const char* password = "SANTI2020";

const char* ssid = "Cieldevice";       //192.168.175.5
const char* password = "update2024";

//const char* ssid = "PCAP1";           //192.168.1.216
//const char* password = "APCiel18";

//const char* ssid = "WiFi Casa";           //192.168.1.216
//const char* password = "Arrozatollado2024";

String myip="000.000.000.000";  //Inicializa la cadena que contiene la dirección IP
String mychannel= "AT+C017";  //El canal corresponde a los dos últimos dígitos de la versión 
                              //Debe establecerse acorde a la configuración previa del módulo HC12 mientras se halla una solición al bloqueo en modo OTA


/*
// Set your Static IP address
IPAddress local_IP(192, 168, 59, 60);     //DIRECCIÓN IP FIJA PARA CELULAR
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
*/
/*
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional
*/

//********boton*******************
bool haboton=true;
int conta;
int estadop=0;            // Estado operación 0 ocupado, 1 libre, 2 sin turno, 3 OTA
int estadopact, estadopant;
bool libre;
bool pulsado;
bool visestado;
bool viscolor;
bool haydatonuevo=false;
int contadisponible;

//#define boton 39   //pull up externo

#define boton 17   

bool beingprog=false;   //bandera que indica si el botón está siendo programado mediante la pulsación constante al encendido









//*************************************** */



WebServer server(80);

/* Style */
String style =
"<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
"input{background:#f1f1f1;border:0;padding:0 15px}body{background:#00afb9;font-family:sans-serif;font-size:14px;color:#777}"
"#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
"#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#00afb9;width:0%;height:10px}"
"form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
".btn{background:#00afb9;color:#fff;cursor:pointer}</style>";

/* Login page */
String loginIndex = 
"<form name=loginForm>"
"<h1>Acceso a Dispositivo CIEL</h1>"
"<input name=userid placeholder='ID Usuario'> "
"<input name=pwd placeholder=Clave type=Password> "
"<input type=submit onclick=check(this.form) class=btn value=Acceso></form>"
"<script>"
"function check(form) {"
"if(form.userid.value=='Ciel' && form.pwd.value=='1983-2023')"
"{window.open('/serverIndex')}"
"else"
"{alert('Error de Usuario o Clave')}"
"}"
"</script>" + style;
 
/* Server Index Page */
String serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
"<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
"<label id='file-input' for='file'>   Elegir archivo *.bin...</label>"
"<input type='submit' class=btn value='Actualizar firmware'>"
"<br><br>"
"<div id='prg'></div>"
"<br><div id='prgbar'><div id='bar'></div></div><br></form>"
"<script>"
"function sub(obj){"
"var fileName = obj.value.split('\\\\');"
"document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
"};"
"$('form').submit(function(e){"
"e.preventDefault();"
"var form = $('#upload_form')[0];"
"var data = new FormData(form);"
"$.ajax({"
"url: '/update',"
"type: 'POST',"
"data: data,"
"contentType: false,"
"processData:false,"
"xhr: function() {"
"var xhr = new window.XMLHttpRequest();"
"xhr.upload.addEventListener('progress', function(evt) {"
"if (evt.lengthComputable) {"
"var per = evt.loaded / evt.total;"
"$('#prg').html('progress: ' + Math.round(per*100) + '%');"
"$('#bar').css('width',Math.round(per*100) + '%');"
"}"
"}, false);"
"return xhr;"
"},"
"success:function(d, s) {"
"console.log('success!') "
"},"
"error: function (a, b, c) {"
"}"
"});"
"});"
"</script>" + style;



//*********************************************************************************************************************************************************************

#define DEBUG 0            //Habilita / deshabilita salida serial para depuración 

#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif


int demobat[7] ={3000,3000,3150,3250,3450,3650,3650};
int bate=1;
int contasol=0;   //contador de solicitudes de conexión
int contaidle=0;   //contador de estado ocioso (libre)
int contawake;
#define DEEP_SLEEP_TIME 5  // Tiempo para despertarse en minutos EN CARGA

#include <Arduino.h>

#include <TaskScheduler.h>            //Para no usar millis()
#define _TASK_SLEEP_ON_IDLE_RUN // Enable 1 ms SLEEP_IDLE powerdowns between tasks if no callback methods were invoked during the pass
#define _TASK_STATUS_REQUEST    // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only



#define INTERVALO 1000
unsigned long time_1 = 0;

#define INTERVALO500 500
unsigned long time_2 = 0;

///lcd////
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
///lcd///



int numdatos;    //Cuenta la longitud de la trama
char tramain          [16]={0xAA,0xAB,0x80,0x01,0x00,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x8A};

const int touchPin1 = 13; 
const int touchPin2 = 15; 
int touchValue1;
int touchValue2;
int contouch1;
int contouch2;
int umbral=70;   //Para el toque
bool toggle;   // Variable general para intermitencias
int contacarga;
int numtrama;
bool encarga=false;
bool ocioso;
int trywifi;
bool hayip=false;
byte contaip;
bool estecla1=false;
bool estecla2=false;



// include library to read and write from flash memory
#include <EEPROM.h>

// define the number of bytes you want to access
#define EEPROM_SIZE 5

#define Threshold 60 /* Greater the value, more the sensitivity */
//touch_pad_t touchPin;



#define DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))  //Emula la función delay(ms) pero sin bloqueo

//***********DEFINICION DE PINES ****************//
const int pot= 36;    //entrada analoga del divisor de voltaje
const int buz= 33;    //Salida buzzer
//const int mtr= 32;    //Salida moto vibrador
const int chrge= 35;    //Entrada indicador de carga en curso del TP4056
const int stdby= 34;    //Entrada indicador de carga completa del TP4056
const int charled= 27;    //Entrada indicador de carga en curso del TP4056
const int stbyled= 26;    //Entrada indicador de carga completa del TP4056
// const int apo= 17;       //Auto power off
const int led = 2;     //LED uso general
const int setHCPin = 32;   //10 VERSION B - 32 VERSION C,D,E,F



//*************************************

String miestadoent;   //Permite que los estados sean globales 
String miestadosal;

bool origenOK;

char ct='0';
char dt='0';
char ut='0';

char myct;
char mydt;
char myut;
int deter; //Decenas de numter
int unter; // Unidades de numter

char mydtprog [21] = {'0','0','0','0','0','0','0','0','0','0','1','1','1','1','1','1','1','1','1','1','2'};  //Mapeo provisional de numero de terminal
char myutprog [21] = {'0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0'};
int  gaptime;   // Tiempo entre cambios de numeración de terminal
int  numprog;    // Numero de terminal programado, char para asegurar un solo byte pues se graba en una sola posición de memoria


//0,1,2,3,4,5,6,7,8,9,A,E,I,t,b,d,s,P,y,r,-,_
//Mapa de caracteres conforme a la lista anterior, se puede ampliar para incluir más:
//Orden de bits: 87654321  - AFED*BGC (*bit 4 no se utiliza, siempre en 0)  ~ ABCDEFG - 8315672
const char num[]={0xF5,0x05,0xB6,0x97,0x47,0xD3,0xF3,0x85,0xF7,0xD7,0xE7,0xF2,0x60,0x72,0x73,0x37,0xD3,0xE6,0x57,0x22,0x2,0x10,0x00,0xF0,0x67,0xE0, 0x12, 0x10, 0xE2, 0xE5, 0x75};  //La longitud de los dos arreglos debe ser la misma para que la función buscachar obtenga el adecuado resultado
const char mychar[]={'0','1','2','3','4','5','6','7','8','9','A','E','I','t','b','d','S','P','y','r','-','_',' ','C','H','R','=','.','F','N','U'};

const char bat[]={0x00,0x80,0x81,0x83,0x87,0x8F}; //Nivel de batería
                  //Nibble alto en 0: borde de la batería apagado
                  //Nibble alto en 8: borde de la batería encendido
                  //Nibble bajo en 1: primer nivel encendido
                  //Nibble bajo en 3: segundo y primer nivel encendido
                  //Nibble bajo en 7: tercer, segundo y primer nivel encendido
                  //Nibble bajo en F: cuarto, tercer, segundo y primer nivel encendido
 


int ict=0;
int idt=0;
int iut=0;

int potValue;       //Valor instantáneo del ADC
int promValue;     //Valor promediado del ADC
int contalevel;
int lecturas;
int acumlect;

bool isonline;
bool eslibre=false;
bool conectado=false;    // En false cuando no hay conexión
bool blinkbat;
bool programando;
bool esOTA=false;
bool libreautoe=false;  //Arranca por defecto en libre manual




int contasleep;  //Cuenta el tiempo antes de dormir, se resetea en toques de touchpad
int contal;    //contador de pulsos tactil pad libre
int contao;    //contador de pulsos t+actil pad ocupado


char serInString[20];  //Buffer de entrada serial
int serInIndx = 0;  //Indice del buffer
char serin;        //Variable de entrada serial


String miestado;
bool state;
bool blink, blinkaux;



int numter;           //Número de la terminal

//String to send to other nodes with sensor readings
String readings;
String mynodeid;

Scheduler userScheduler; // to control your personal task



//****Para Platformio******
void contaDormir() ; // Prototype so PlatformIO doesn't complain
void iniciando() ; // Prototype so PlatformIO doesn't complain
void base1seg() ; // Prototype so PlatformIO doesn't complain
void milis500() ; // Prototype so PlatformIO doesn't complain
void pita();
void Write_1621(); 
void levelBat();
void readSerial();
void solicitaconexion();
void compara_arranque();
void asigna_turno();
void confirma_ocupado_libre();
void hc12Sleep();
void hc12Wake();
void goToDeepSleep();
void vis_ip();
void set_chan();

void OTA();
void rutina();
void estadoBoton();

void visualizaEstado();

void visRojo();
void visVerde();
void visAzul();
void visMagenta();
void visTurquesa();
void prognumter();

//*****************







//Create tasks: to send messages and get readings;

Task taskContaDormir(TASK_SECOND * 3 , TASK_FOREVER, &contaDormir);
Task taskIniciando(TASK_SECOND * 2 , TASK_FOREVER, &iniciando);
Task taskBase1Seg(TASK_SECOND * 1 , TASK_FOREVER, &base1seg);  //Original 2 seg, para probar mejor velocidad en el arranque
Task taskMilis500(TASK_MILLISECOND * 500 , TASK_FOREVER, &milis500);

void set_chan(){
 
 digitalWrite(setHCPin, LOW);
 DELAY(200);     
 Serial2.print(mychannel);    //CORRESPONDE A LOS ÚLTIMOS DÍGITOS DE LA VERSIÓN

}



void buscachar(){
  
 //**Busca el caracter asociado en el array mychar por cada dígito y cuando lo encuentra toma el respectivo índice para que apunte en el array num

  debug(myct);
  debug(mydt);
  debugln(myut);

for(int k=0; k<sizeof(mychar);k++){
  if (myct==mychar[k]) {ict=k; break;}
  else ict=20;
}

for(int k=0; k<sizeof(mychar);k++){
  if (mydt==mychar[k]) {idt=k; break;}
  else idt=20;
}

for(int k=0; k<sizeof(mychar);k++){
  if (myut==mychar[k]) {iut=k; break;}
  else iut=20;
}


}




void milis500(){

toggle=!toggle;         //Conmuta estados para parpadeo de turno


 //Determina el tipo de acción de accuerdo a la longitud de la trama  
  if (numdatos==7) compara_arranque();         //Si la trama de entrada es de 7 datos, significa que la terminal es reconocida por el kiosco
  if (numdatos==12)  asigna_turno();            //Si la trama de entrada es de 12 datos, significa que contiene el turno (sin CR+LF)
  if (numdatos==14)  asigna_turno();            //Si la trama de entrada es de 14 datos, significa que contiene el turno (con CR+LF)
  if (numdatos==8)  confirma_ocupado_libre();


}



////////////////////*********************

#define CS   12  //Pin 12 como salida chip select
#define WR   14  //Pin 14 como salida de escritura
#define DATA 25  //Pin 25 como salida de datos 
//#define BLHT 23  //Pin 5 BACKLIGHT -- opcional, no se usará pues consume 60mA
 
//Definición de estados de las señales de control 
#define CS1    digitalWrite(CS, HIGH) 
#define CS0    digitalWrite(CS, LOW)
#define WR1    digitalWrite(WR, HIGH) 
#define WR0    digitalWrite(WR, LOW)
#define DATA1  digitalWrite(DATA, HIGH) 
#define DATA0  digitalWrite(DATA, LOW)

//Manejo a nivel de bits 
#define sbi(x, y)  (x |= (1 << y))  
#define cbi(x, y)  (x &= ~(1 <<y ))      
#define uchar   unsigned char 
#define uint   unsigned int 

//Comandos predefinidos ***NO ALTERAR*** 
#define  ComMode    0x52  
#define  RCosc      0x30  
#define  LCD_on     0x06 
#define  LCD_off    0x04 
#define  Sys_en     0x02 
#define  CTRl_cmd   0x80
#define  Data_cmd   0xa0   

//Variables de prueba - contador 3 dígitos
int contaun;
int contadc;
int contacn;
int contadc2;

char dispnum[6]={0x00,0x00,0x00,0x00,0x00,0x00};    //Bufer para visualizar los digitos del LCD

String version;
 





void SendBit_1621(uchar sdata,uchar cnt) {
  uchar i;
  for(i=0;i<cnt;i++)
  {
    WR0;
    if(sdata&0x80) DATA1;
    else DATA0;
    WR1;
    sdata<<=1;
  }
}
 
void SendCmd_1621(uchar command)
{
  CS0;
  SendBit_1621(0x80,4);
  SendBit_1621(command,8);
  CS1;
}
 
void Write_1621(uchar addr,uchar sdata)
{
  addr<<=2;
  CS0;
  SendBit_1621(0xa0,3);
  SendBit_1621(addr,6);
  SendBit_1621(sdata,8);
  CS1;
}
 
void HT1621_all_off(uchar num)   //Apaga todos los segmentos
{
  uchar i;
  uchar addr=0;
  for(i=0;i<num;i++)
  {
    Write_1621(addr,0x00);
    addr+=2;
  }
}
 
void HT1621_all_on(uchar num)  //Enciende todos los segmentos
{
  uchar i;
  uchar addr=0;
  for(i=0;i<num;i++)
  {
    Write_1621(addr,0xff);
    addr+=2;
  }
}
 
void Init_1621(void)     //Inicializa el módulo
{
  SendCmd_1621(Sys_en);
  SendCmd_1621(RCosc);
  SendCmd_1621(ComMode);
  SendCmd_1621(LCD_on);
}
 
void displaydata(int p)   
{
  uchar i=0;
  switch(p)
  {
    case 1:
    sbi(dispnum[0],7);
    break;
    case 2:
    sbi(dispnum[1],7);
    break;
    case 3:
    sbi(dispnum[2],7);
    break;
    default:break;
  }
  for(i=0;i<=5;i++)
  {
    Write_1621(i*2,dispnum[i]);
  }
}

/*F5 05 B6 97 47 D3 F3 85 F7 D7*/



void blinkBat() {
 // for (int i = 0; i < 5; i++) {  // Hacer que parpadee 5 veces
   if (blinkbat)  Write_1621(6, bat[1]);  // Mostrar el borde de batería
   
   if (!blinkbat) Write_1621(6, bat[0]);  // Apagar el borde de batería
    
 // }
}

void cargando() {

 
  //if (toggle) Write_1621(6,bat[5]);
   if (toggle) levelBat();        //Parpadea el nivel de batería si está cargando
   else Write_1621(6,bat[1]);     //Mostrar el borde de batería
  // contasleep=0;                  //Para que no se duerma si está cargando
   Write_1621(4,num[iut]);   // Visualiza unidades
   Write_1621(2,num[idt]);   // Visualiza decenas
   Write_1621(0,num[ict]);   // Visualiza centenas   


}


void levelBat() {
  potValue = analogRead(pot);  // Leer el valor del potenciómetro (SENBAT)
  //  potValue = demobat[bate];   //debug
  if (lecturas<16) {lecturas++; acumlect=acumlect+potValue;}
  if (lecturas==16) {promValue=acumlect/16; acumlect=0; lecturas=0;}   //Valor promediado de las 16 lecturas ADC
  
    
 
   
 // if ((!digitalRead(bok)) && (potValue >= 3123)) { Write_1621(6,bat[5]); Write_1621(6,bat[4]); Write_1621(6,bat[3]); Write_1621(6,bat[2]); blinkBat();}  //Parpadea el contorno de batería si está cargada
  
  if (promValue >= 3600) {
    Write_1621(6,bat[5]); //blink=false;
    //debugln(potValue);
  } else if (promValue < 3600 && promValue >= 3400) {
    Write_1621(6,bat[4]); //blink=false;
    //debugln(potValue);
  } else if (promValue < 3400 && promValue >= 3200) {
    Write_1621(6,bat[3]); //blink=false;
    //debugln(potValue);
  } else if (promValue < 3200 && promValue >= 3100) {
    Write_1621(6,bat[2]); //blink=false;
    //debugln(potValue);
  } else if (promValue < 3100) {
    blinkBat();

  }



}



void iniciando(){

    if (!conectado) { }

}

///////////////////**************

void setup() {

  pinMode(boton, INPUT_PULLUP);
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical
  Serial.begin(9600);  // 
  EEPROM.begin(EEPROM_SIZE);
  if(digitalRead(boton)==LOW) {beingprog=true; prognumter();}
 
  delay(3000);
 
 
  numter = EEPROM.read(0); 


  
  
  pinMode(setHCPin, OUTPUT);       // Pin de control del HC-12, en LOW acepta comandos AT, en HIGH entra en modo de funcionamiento normal; 
  digitalWrite(setHCPin, LOW);     //Inicia deshabilitando transmisión

  


  version="2DN";     //Versión con la que debe actualizarse en el rango #(A-F)(1-F)Canaal  . Ejemplo: 1C2C17; el canal se visualiza después de visualizar la ip
 // numter=4;  //coincidente con el último digito de versión y numero de terminal
//**ORIGINAL 2B1, CAMBIADO EN EL CÓDIGO POR 2C# */
 

  if (numter==1) { leds[2] = CRGB(0, 0, 0);    leds[1] = CRGB(0, 0, 0);   leds[0] = CRGB(32, 32, 0);  }
  if (numter==2) { leds[2] = CRGB(0, 0, 0);    leds[1] = CRGB(32, 32, 0);  leds[0] = CRGB(0, 0, 0);   }
  if (numter==3) { leds[2] = CRGB(0, 0, 0);    leds[1] = CRGB(32, 32, 0);  leds[0] = CRGB(32, 32, 0);  }
  if (numter==4) { leds[2] = CRGB(32, 32, 0);   leds[1] = CRGB(0, 0, 0);   leds[0] = CRGB(0, 0, 0);   }
  if (numter==5) { leds[2] = CRGB(32, 32, 0);   leds[1] = CRGB(0, 0, 0);   leds[0] = CRGB(32, 32, 0);  }
  if (numter==6) { leds[2] = CRGB(32, 32, 0);   leds[1] = CRGB(32, 32, 0);  leds[0] = CRGB(0, 0, 0);   }
  if (numter==7) { leds[2] = CRGB(32, 32, 0);   leds[1] = CRGB(32, 32, 0);  leds[0] = CRGB(32, 32, 0);  }
//************************************************* */

  
   FastLED.show(); 
   delay(2000);

  Serial.begin(9600);

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  EEPROM.begin(EEPROM_SIZE);
  //libreautoe=EEPROM.read(1);
  libreautoe=false;
 // numter = EEPROM.read(0); //*******************
 
  
  
  delay(200);                        //Los retardos son necesarios mientras el HC12 se estabiliza
 // Serial2.print("AT+FU3");           //Funcion transparente 1 para 3.6mA / 3 para 16mA MAYOR ALCANCE

  userScheduler.addTask(taskContaDormir);
  userScheduler.addTask(taskIniciando);
  userScheduler.addTask(taskBase1Seg);



  taskContaDormir.enable();
  taskIniciando.enable();
  taskBase1Seg.enable();
  taskMilis500.enable();


   /////*****
  pinMode(CS, OUTPUT); // 
  pinMode(WR, OUTPUT); // 
  //pinMode(DATA, OUTPUT); //
  pinMode(DATA, OUTPUT_OPEN_DRAIN); //Necesario para que no se apague el LCD aleatoriamente, con pull up de 1.5K 
  pinMode(4, INPUT_PULLUP);        //Necesario para poder despertar en pin 4 por un bajo activo conectado al sensor de vibración
  
                                   // especialmente útil para enviar el comando sleep y ahorrar batería
  

  CS1;
  DATA1;
  WR1;
  DELAY(50);
  Init_1621();
  HT1621_all_on(16);
  DELAY(250);
  HT1621_all_off(16);
  DELAY(250);


  
  
 // esp_sleep_enable_touchpad_wakeup();  //Configure Touchpad as wakeup source
 
 

    //Definición de puertos y estados
  //pinMode(apo, OUTPUT);
  //digitalWrite(apo,  HIGH);
  //pinMode(mtr, OUTPUT);
  //digitalWrite(mtr,  LOW);
  pinMode(buz, OUTPUT);
  digitalWrite(buz,  LOW);
  
  pinMode(chrge, INPUT_PULLUP);
  pinMode(stdby, INPUT_PULLUP);
  pinMode(stbyled, OUTPUT);
  pinMode(charled, OUTPUT);

  //Lee estado de teclas LIBRE y OCUPADO en el encendido para ajustar terminal a 0 y poder programar OTA
 //for (int j=0; j<15; j++) {
  touchValue1 = touchRead(touchPin1);
  touchValue2 = touchRead(touchPin2);

  DELAY(200);

 if ((touchValue1<umbral)&&(touchValue2<umbral)) {esOTA= true; numter=0;  pita();}  //Si los dos botones son tocados en el encendido habilita OTA con numter=0
 if (!esOTA){ 
              if (touchValue1<umbral) libreautoe=true;
              if (touchValue2<umbral) libreautoe=false;
              EEPROM.write(1, libreautoe); //Graba en memoria
              EEPROM.commit();  
             }



  //myct= 't';  //Caracter fijo indicando la terminal
  if (libreautoe) myct= 'A';    //Indica  modo libre automático
  else myct= 'N';           // Indica modo libre manual 


  deter=numter/10;
  unter=numter%10;

  mydt= deter + '0';  //Ajuste para convertir a int a char
  myut= unter + '0';

  debug("TER#");
  debug(deter);
  debugln(unter);
  
  buscachar();
 
  // set_chan();    //Inicializa el canal de transmisión pero por razón deconocida se reinicia la terminal al encontrar red para OTA


  miestadosal= "CONNECTING";   //Inicia enviando el estado conectando
 // miestadosal="DEBUGGING";

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4,0); //1 = High, 0 = Low    Se despertará por un bajo en GPIO4 / PIN 26

        Write_1621(4,num[iut]);   // Visualiza unidades terminal 
        Write_1621(2,num[idt]);   // Visualiza decenas terminal
        Write_1621(0,num[ict]);   // Visualiza centenas terminal (caracter t)

  if ((digitalRead(chrge)==LOW) && (digitalRead(stdby)==HIGH)) {hc12Sleep();}     //Inablilita rf cargando en el arranque
  if ((digitalRead(chrge)==HIGH) && (digitalRead(stdby)==LOW)) {hc12Sleep();}     //Inablilita rf en carga full en el arranque
//no va
 // if ((numter>0) && (numter<15)) {WiFi.mode(WIFI_OFF); hc12Wake();}  //Apaga WiFi si la dirección no es 00 para ahorrar batería y deja HC12 despierto 
 // else  hc12Sleep();  //Duerme HC12 para que no envíe verbose a la red RF y la sature o bloquee 





}   // Fin setup

void pita(){
  
  digitalWrite(buz, HIGH); 
 // digitalWrite(mtr, HIGH); 
  DELAY(100);  
  digitalWrite(buz, LOW);
 // digitalWrite(mtr, LOW);


}



void envialibre(){
  char chksm=0;  //Asegura 8 bits
 // numtrama++;
  serin= ' ';   //Evita bloqueo del serial
 // numter=1;  //Provisional para pruebas
//                        [$AA][$AB][$80][$1][$1][$1][$6][$89]     8 BYTES
char tramalibre     [8]={0xAA,0xAB,0x80,0x01,0x01,0x01,0x06,0x89} ;

 numtrama++;    //Incrementa numero de trama
 if (numtrama==0xAA) numtrama=0xAC;   //Evita que aparezca AA o AB en posición diferente del encabezado
 tramalibre[0]= 0xAA;         //1er byte encabezado
 tramalibre[1]= 0xAB;         //2do byte encabezado
 tramalibre[2]= 0x80;         //Destino
 tramalibre[3]= numter;       //Origen
 tramalibre[4]= numtrama;     //Número de trama
 tramalibre[5]= 0x01;         //Longitud de datos
 tramalibre[6]= 0x06;         //Comando LIBRE
 tramalibre[7]= 0x00;         //CHECKSUM  (sin calcular)
 


 for (int k=2;k<7;k++) { chksm= chksm + tramalibre[k];} 
 
 tramalibre[7]= chksm;        //CHECKSUM  (calculado)
 

  debug("TX: ");
  for (int t=0; t<8;t++){
 
   // Serial.write(int(tramaout[t]));
    Serial2.write(int(tramalibre[t]));

  }
  
  eslibre=true;
  if (!ocioso)  pita();    //Pita solamente si no está en guiones para no saturar de pitos cuando está ocioso
  contaidle=0;

  /* visVerde();
   DELAY(250);   //para el boton
   haboton=true;*/

 }

 void enviaconfirmacion(){

  char chksm=0;  //Asegura 8 bits
  serin= ' ';   //Evita bloqueo del serial
 // numter=1;  //Provisional para pruebas
//                       [$AA][$AB][$80][$1][$1][$0][$82]  7 bytes
char tramaconfirma  [7]={0xAA,0xAB,0x80,0x01,0x01,0x00,0x82} ;  // La longitud de datos es 0

// numtrama++;    //No se usa aumenta para confirmación, se sigue el vigente
 tramaconfirma[0]= 0xAA;         //1er byte encabezado
 tramaconfirma[1]= 0xAB;         //2do byte encabezado
 tramaconfirma[2]= 0x80;         //Destino
 tramaconfirma[3]= numter;       //Origen
 tramaconfirma[4]= numtrama;     //Número de trama
 tramaconfirma[5]= 0x00;         //Longitud de datos
 tramaconfirma[6]= 0x00;         //CHECKSUM  (sin calcular)

 for (int k=2;k<7;k++) { chksm= chksm + tramaconfirma[k];} 
 
 tramaconfirma[6]= chksm;        //CHECKSUM  (calculado)
 

   debug("Confirma: ");
  for (int t=0; t<7;t++){
 
   // Serial.write(int(tramaout[t]));
    Serial2.write(int(tramaconfirma[t]));

  }

}



 void enviaocupado(){
  esOTA=false;
  char chksm=0;  //Asegura 8 bits
  serin= ' ';   //Evita bloqueo del serial
 // numter=1;  //Provisional para pruebas
//                        [$AA][$AB][$80][$1][$1][$1][$5][$88]     8 BYTES
char tramaocupado   [8]={0xAA,0xAB,0x80,0x01,0x01,0x01,0x05,0x88} ;

 numtrama++;    //Incrementa numero de trama
 if (numtrama==0xAA) numtrama=0xAC;   //Evita que aparezca AA o AB en posición diferente del encabezado
 tramaocupado[0]= 0xAA;         //1er byte encabezado
 tramaocupado[1]= 0xAB;         //2do byte encabezado
 tramaocupado[2]= 0x80;         //Destino
 tramaocupado[3]= numter;       //Origen
 tramaocupado[4]= numtrama;     //Número de trama
 tramaocupado[5]= 0x01;         //Longitud de datos
 tramaocupado[6]= 0x05;         //Comando OCUPADO
 tramaocupado[7]= 0x00;         //CHECKSUM  (sin calcular)

 for (int k=2;k<8;k++) { chksm= chksm + tramaocupado[k];}   //Calcula checksum
 
 tramaocupado[7]= chksm;        //CHECKSUM  (calculado)
 

 
  for (int t=0; t<8;t++){
  
   
    Serial2.write(int(tramaocupado[t]));

  }

  eslibre=false;
  pita();

  ocioso=false;

  libre=false;

  viscolor=true;
  visRojo();

  //Serial2.println(promValue);  //***prueba 

  /*
  viscolor=true;
  visRojo();
   DELAY(250);   //para el boton
   haboton=true;
   libre=false;*/

 }

 void confirma_ocupado_libre(){
  numdatos=1;   //Para que no quede en bucle
  //[$AA][$AB][$1][$80][$4][$1][$5][$8B]	
  char chksm=0;  //Asegura 8 bits
  for (int k=2;k<7;k++){ chksm= chksm + serInString[k];}  //calcula checksum 
  debug("CONFIRMANDO OCUPADO ");
  //DEPURACIÓN
  debug("checksum: ");
  debugln(chksm);
  debug("NUMTERSTRING: ");
  debugln(int(serInString[2]));
  debug("CHKSMTRAMA: ");
  debugln(serInString[7]);
  debug("NUMTERorigen: ");
  debugln(numter);

  debug("REVISANDO TRAMA ENTRADA:  ");
  for (int k=0;k<8;k++){ debug(serInString[k]);} 
 

  debug("REVISANDO CONDICIONES:  ");
  debug(numter);
  debug(chksm);
  


  
  if ( ((serInString[2])==numter) && (serInString[3]==0x80) && (serInString[6]==0x05) && (serInString[7]==chksm) ) { debugln("CONFOCUPADO"); miestadosal="CONFIRMA_O"; pita(); libre=false; viscolor=true; visRojo(); enviaconfirmacion(); /*estadop=0; visualizaEstado();*/  haboton=false;  }   //parater corresponde al numter de origen
  if ( ((serInString[2])==numter) && (serInString[3]==0x80) && (serInString[6]==0x06) && (serInString[7]==chksm) ) { debugln("CONFLIBRE"); miestadosal="CONFIRMA_L";  pita(); /*estadop=1; visualizaEstado();*/ haboton=false; libre=true; viscolor=true; visVerde(); }  
  

}




void contaDormir(){

  if (contasleep<92) {contasleep++; debugln(contasleep);}     // original 240 Si no se toca ninguna tecla, va incrementandose el contador para dormir
 // if (contadisponible<10) contadisponible++;    //Inicia en 0 en la rutina de visAzul, disponible
 // if (contadisponible==5) estadop=2;   // Si pasan mas de 4 segundos en - - - (azul), asegura que no pase a ocupado sino directamente a libre
  
  //Serial.print(contasleep);
 // Serial.println(ocioso);
   if (!esOTA){
     if (haydatonuevo){
    Serial.print(myct);
    Serial.print(mydt);
    Serial.println(myut);
    haydatonuevo=false;}
    }


  if (contasleep==90) {enviaocupado(); }   //Pasa a ocupado automáticamente al minuto y medio (90seg) si no se toca alguna otra tecla
  //no dormirá en boton
  /*
  if ((contasleep==240) || ((digitalRead(stdby)==LOW)&&(contasleep==60))) {              //Se duerme en CUATRO minutos (240 seg) si no se ha tocado ninguna tecla o si han pasado 1 minuto despues de carga lista
     hc12Sleep();               //Duerme HC12 para que pase de 16 mA en IDLE a 22uA
     debugln("A dormir ya");
     Write_1621(6, bat[0]);  // Apagar el nivel de batería, ya que visualizaría el último estado de carga, pero se seguirá descargando lentamente y daría una lectura equivocada del estado de carga
     DELAY(100);
     esp_deep_sleep_start();  //Empieza a dormir ( 1mA con deep sleep y 16mA con light_sleep ), equivale al despertar con un reset

  }
      */
}
void loop() {
   estadoBoton();
 if (esOTA) server.handleClient();
  
  // contasleep=0;   //prueba para que nunca duerma y probar descarga de batería
   if (numter==0) {server.handleClient(); contasleep=0;}    //Habilita OTA si la terminal está direccionada como 00 y evita que se duerma
   
     if(millis() >= time_1 + INTERVALO){         //Intervalo de 1 seg
        time_1 +=INTERVALO;
        base1seg();

    }
      if(millis() >= time_2 + INTERVALO500){         //Intervalo de 500 mseg
        time_2 +=INTERVALO500;
        milis500();
 

    }

   //readSerial();    //descomentar si se quiere receopción constante
 
  
  digitalWrite(charled, digitalRead(chrge));     //Copia el estado de las salidas del TP4056 a los pines de salida indicadores led (en carga y cargado)
  digitalWrite(stbyled, digitalRead(stdby));

  levelBat();

///**NO VA EN BOTON */
  /* if ((digitalRead(chrge)==LOW) && (digitalRead(stdby)==HIGH) && (promValue < 4080)) { hc12Sleep(); myct='C'; mydt='H'; myut='R'; buscachar();   cargando(); }          // Deshabilita comunicaciones mientras está cargando, llama a la función de visualización para que parpadee el nivel de bateria estando en carga
   if (((digitalRead(chrge)==HIGH) && (digitalRead(stdby)==LOW))) {hc12Sleep(); myct='R'; mydt='d'; myut='y'; buscachar(); levelBat(); }          //Visualiza la carga estáticamente, sin parpadear cuando está a plena carga
   if ((digitalRead(chrge)==LOW) && (digitalRead(stdby)==HIGH) && (promValue > 4080)) { hc12Sleep(); myct='R'; mydt='d'; myut='y'; buscachar(); levelBat(); }          //Si  no se activa la señal stdby del TP4056 asume la carga cercana a 3.3V del divisor (~4.2V en batería)
   if ((digitalRead(chrge)==HIGH) && (digitalRead(stdby)==HIGH)){ levelBat();   /*readSerial(); }*/      //Visualiza la carga estáticamente, sin parpadear cuando está solo con bateria, sin cargar
  
  // if ((digitalRead(chrge)==HIGH) && (digitalRead(stdby)==HIGH)) encarga=false; 
  // else encarga=true;

//*********************** */
   if (!encarga) { readSerial(); levelBat();   }    //solo recibe datos si no está en carga
  readSerial();
  //touchValue1 = touchRead(touchPin1);
  //touchValue2 = touchRead(touchPin2);

 
 


  if (touchValue1<umbral){ contasleep=0; contouch1++; DELAY(5);}
  else contouch1=0; 
 
  if ((contouch1==30) && (numter!=0)){ miestadosal="LIBRE"; if(!encarga) envialibre(); /*pita();*/  }

/*

 if ((touchValue2>umbral) && (programando)) {
  programando=false; //Evita sobreescritura constante en la memoria
  numter=numprog-1;     //Backup de identificación, ajustado para que coincida variable y pantalla
   if ((digitalRead(chrge)==HIGH) && (digitalRead(stdby)==HIGH)) {   //Solo graba en memoria si no está cargando
      EEPROM.write(0, numter); //Graba en memoria
      EEPROM.commit();  
      pita(); 
      DELAY(500); 
   }
  ESP.restart();   //Reinicia para salir del modo programación
  }           
  */


  if (touchValue2<umbral){ contasleep=0; contouch2++; DELAY(5);}
  else contouch2=0; 
 
    if ((contouch2==30) && (numter!=0)) { miestadosal="OCUPADO"; if(!encarga) enviaocupado(); /*pita();*/  }
    if ((contouch2==10) && (hayip)) {if (contaip<7) contaip++; if (contaip==7) contaip=0; /*Serial.println(contaip);*/ }     //contador de octetos para visualizar IP
    if (contouch2==500)  {miestadosal="PROGRAMANDO"; myct= 't';  mydt= 'E';  myut= 'r'; pita();   numprog= 0; buscachar(); }
    if (contouch2==(500+(250*numprog))) { mydt= mydtprog[numprog]; myut= myutprog[numprog]; numprog++; buscachar(); debugln(numprog); debugln(2); programando=true;}   //A partir de 500 incrementa cada 250 ciclos el contador de terminal asignando los apuntadores para la visualización
    if (contouch2==6000){contouch2=510; numprog=1;} //Si numprog llega a 20 reinicia en 01
   //  debugln(contouch);
   //  debugln(numprog);


   if (numter==0) {myct=version[0];mydt=version[1];myut=version[2]; buscachar();}   //Visualiza versión del firmware únicamente con la direccion 00 
  
   
  // iut=2;idt=7;ict=9; //prueba
    
  if (eslibre) {     //Parpadea en libre

      if (toggle)  {
        Write_1621(4,num[iut]);   // Visualiza unidades
        Write_1621(2,num[idt]);   // Visualiza decenas
        Write_1621(0,num[ict]);   // Visualiza centenas   
      }
      else{
        Write_1621(4,num[22]);   // Visualiza en blanco
        Write_1621(2,num[22]);   // Visualiza en blanco
        Write_1621(0,num[22]);   // Visualiza en blanco

      }
  }    
  else{ 
        Write_1621(4,num[iut]);   // Visualiza unidades
        Write_1621(2,num[idt]);   // Visualiza decenas
        Write_1621(0,num[ict]);   // Visualiza centenas
  }

if ((tramain[2]==numter) &&(tramain[7]==0x2D) && (tramain[8]==0x2D) && (tramain[9]==0x2D))  ocioso=true;  

//if (ocioso) visAzul();//else  ocioso=false;  //***********revisar



if (hayip) vis_ip();   //Si se obtuvo conexiín de red WiFi habilita la visualización de la IP

} // fin loop;

void vis_ip(){
if (contaip==0){   //Visualiza IP-
   myct='I';
   mydt='P';
   myut='='; 
    
  }
if (contaip==1){  //Visualiza primer octeto de la dirección IP
   myct=myip[0];
   mydt=myip[1];
   myut=myip[2]; 
   
  }
if (contaip==2){  //Visualiza segundo octeto de la dirección IP
   myct=myip[3];
   mydt=myip[4];
   myut=myip[5]; 
   
  }

if (contaip==3){  //Visualiza tercer octeto de la dirección IP
   myct=myip[6];
   mydt=myip[7];
   myut=myip[8]; 
     
  }
if (contaip==4){  //Visualiza cuarto octeto de la dirección IP
   myct=myip[9];
   mydt=myip[10];
   myut=myip[11]; 
    
  }
if (contaip==5){  //Visualiza cuarto octeto de la dirección IP
   myct=myip[12];
   mydt=myip[13];
   myut=myip[14]; 
    
  }  
  
if (contaip==6){  //Visualiza canal
   myct=mychannel[3];
   mydt=mychannel[5];
   myut=mychannel[6]; 
    
  } 

  buscachar();   //
  Write_1621(4,num[iut]);   // Visualiza unidades
  Write_1621(2,num[idt]);   // Visualiza decenas
  Write_1621(0,num[ict]);   // Visualiza centenas   


}



void readSerial(){
while (Serial2.available()>0) {

    serin = char(Serial2.read());
//**ACCIONES DE PRUEBA POR COMANDOS- COMENTAR PARA GRABAR**//
   // if (serin == '$') serInIndx = 0; 
   // if (serin == '*') buscachar2(); //para depuración
   // if (serin == '/') {isonline=true; serin='?'; numter=int(serInString[1])-'0';  }
   // if (serin == '#') { numter=serInString[1]-'0'; EEPROM.write(0, numter); EEPROM.commit(); debug("NUMTER: ");  debug(numter); serin=' ';}
   // if (serin == 'L') {miestadosal="LIBRE";   }//*/envialibre();
   // if (serin == 'O') {miestadosal="OCUPADO";  }//*/enviaocupado();
   // if (serin == 'C') {miestadosal="CONFIRMAL"; }//*/enviaconfirmacion();
   // if (serin == '*')  { bate = serInString[1] -'0'; Serial.println(bate); Serial.println(potValue);  }
   




     if (serin==0xAA) {serInIndx = 0; debug("RX: ");}


    serInString[serInIndx]=serin;
    tramain[serInIndx]=serin;  //copia buffer entrada

    serInIndx++;    //Actualiza el indice cada vez que llega un dato


    numdatos= serInIndx;       //Numero de datos para identificar el tipo de trama que envía el kiosco
   

   // contasleep=0;        //Reinicia el contador para evitar que se duerma en medio de transmisión
   

  }

  while (Serial.available()>0) {




  }

}  

void solicitaconexion(){

if ((contasol<5) && ((numter>0) && (numter<15)))           //Solamente con una dirección mayor a 0 y menor a 15 solicita conexión durante los primeros 5 segundos de encendido paa no saturar la banda con peticiones
    {contasol++;
    debugln("SOLICITANDO CONEXION");
    char tramaencendido [16]={0xAA,0xAB,0x80,0x01,0x00,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x8A};
    tramaencendido[3]= numter; 
    char chksm=0; //Inicializa en 1 byte
    for (int k=2;k<14;k++){ chksm=  chksm + tramaencendido[k];}  //calcula checksum 
    tramaencendido[15]= chksm;


    for (int j=0; j<16;j++){           //segun longitud de trama

   
    Serial2.write (tramaencendido[j]);
    
    Serial.write (tramaencendido[j]);

    }

  }
   //descomentar bloque para solicitar conexión


   //  miestadoent="ONLINE";   //Simula que es reconocido por el kiosco
}


void compara_arranque(){               //Para saber si la trama de petición es correcta, recibiendo 7 bytes
   //[$AA][$AB][$1][$80][$0][$0][$81]   
  char chksm=0;  //Asegura 8 bits
  for (int k=2;k<5;k++){ chksm= chksm + serInString[k];}  //calcula checksum 
  
  //DEPURACIÓN
  debug("checksum: ");
  debugln(chksm);
  debug("NUMTERSTRING: ");
  debugln(int(serInString[2]));
  debug("CHKSMTRAMA: ");
  debugln(serInString[6]);
  debug("NUMTERorigen: ");
  debugln(numter);

  numdatos=0;   //evita bloqueo

  
  if((int(serInString[2])==numter) && (serInString[3]==0x80) && (serInString[6]==chksm)) { debugln("RECONOCIDO"); miestadoent="ONLINE";  }  
 // if((int(serInString[2])==0x80) && (serInString[6]==chksm)) { debugln("RECONOCIDO2"); miestadosal="DUMMY";  }   


 }



void asigna_turno(){
  numdatos=1;   //Para que no quede en bucle
  haydatonuevo=true;
  //[$AA][$AB][$1][$80][$1][$5][$6][$2D][$2D][$2D][$2D][$41] 
  serin= ' ';   //Evita bloqueo del serial

  for (int k=0;k<12;k++) { debug (tramain[k]);}     //prueba de entrada
  


  char chksm=0;  //Asegura 8 bits

  for (int k=2;k<11;k++) { chksm= chksm + tramain[k];} 
  debugln(chksm);
  debugln(numter);

 // if ((tramain[2]==numter) && (tramain[4]!=numtramant) && (tramain[11]==chksm)){   //********prueba con numero de trama
  if ((tramain[2]==numter) && (serInString[3]==0x80) && (tramain[11]==chksm)){   //
    
    myct= tramain[7]+'0';    //Ajusta sumandole el caracter 0 al valor recibido en hexa para poder visualizar
    mydt= tramain[8]+'0';
    myut= tramain[9]+'0';
    
  //  tramain[4]= numtramant;

    miestadoent="ASIGNA";
  
    miestadosal="ASIGNADO";

    debug("TURNO: ");
    debug(myct);
    debug(mydt);
    debugln(myut);
 
   /* if ((myct=='-')&&(mydt=='-')&&(myut=='-'))  visAzul();
    else visVerde();*/

    if (haydatonuevo){
    Serial.print(myct);
    Serial.print(mydt);
    Serial.println(myut);
    haydatonuevo=false;}
    
    if ((tramain[7]==0x2D) && (tramain[8]==0x2D) && (tramain[9]==0x2D)) {viscolor=true; visAzul();}
    else {viscolor=true; visVerde();}

     }
  

  ocioso=false;   // No está disponible en guiones



}

 
void base1seg(){
  contaDormir();
 // debugln(contasleep);
 // if (digitalRead(chrge)==LOW) contawake++;
 // if (contawake==14) {toggle=true;}     //Garantiza la visualización del nivel de la batería antes de dormir
 // if (contawake==15) {goToDeepSleep();} //Prepara la rutina de dormir


 // if (miestadosal== "DEBUGGING") { Serial.println(touchValue1);  Serial.println(touchValue2);  Serial.println(); }   //Debug para determinar el valor real del touch
  if (miestadosal=="CONNECTING") { debugln("SOLCON"); if(!encarga) solicitaconexion();} 
  if (miestadoent=="CONNECTED") {miestadosal="CONNECTED"; debugln("CONECTADO"); pita(); }        //Significa que la terminal se unió a la red  -- originalmente para Espmesh
  if (miestadoent=="WAITING4KIOSK") {miestadosal="WAITING4KIOSK";debugln("ESPERANDO CONEXION");}              //Espera trama de respuesta a conexión  -- originalmente para Espmesh
  if (miestadoent=="ONLINE") {miestadosal="ONLINE";  debug("ONLINE: "); debug(numter); pita();pita();pita(); miestadoent="LISTO";miestadosal="READY";  conectado=true; if (libreautoe) envialibre();}        //Terminal en linea con el kiosco. La bandera conectado impide que se quede en transmisión. Se reconoce por pitido largo. Según libreautoe enciende o no en libre
  if (miestadoent=="ASIGNA") {if(!ocioso) pita(); miestadosal="CONFIRMAL"; miestadoent="ESPERANDO"; enviaconfirmacion(); /*Para que  no se quede pitando*/ }        //Terminal en linea con el kiosco
  if (miestadoent=="CONFIRMA_L") {miestadosal="ESPERANDO"; }        //Terminal en linea con el kiosco
  if (miestadoent=="CONFIRMA_O") {pita(); miestadosal="CONFIRMAO";  miestadoent="ESPERANDO"; }

        
  blinkbat = !blinkbat;   //Conmuta estados para parpadeo del borde de batería cuando esta por debajo del umbral mínimo indicando que requiere recarga inmediata

 
  //encarga = !encarga;
  /*
  debug("CARGA: ");
  debugln(digitalRead(chrge));
  debugln(digitalRead(stdby));
  */



  if ((touchValue2>50) && (programando)) {programando=false; numter=numprog; /*EEPROM.write(0, numter); EEPROM.commit();*/  pita();}
   
  if (digitalRead(stdby)==LOW)  { contacarga++;}

  buscachar();
  //no va en boton
  // if(digitalRead(stdby)==LOW) contasleep++;   //Si la carga está lista incrementa el contador para dormir  //*****VERIFICAR*****

   if ((ocioso) && (contaidle<6)) contaidle++;    //Si está en guiones y el contador es menor a 5 incrementa
   if ((contaidle==5) && (eslibre) && (libreautoe)) envialibre();   //Pasa a libre en cinco segundos si el estado es libre para que no espere botón y el sistema le asigne automáticamente
   if (contaidle==6) contaidle=0;


}

void hc12Sleep(){
  digitalWrite(setHCPin, LOW);
  DELAY(200);                        //Los retardos son necesarios mientras el HC12 se estabiliza
  Serial2.print("AT+SLEEP");
  DELAY(200);
  digitalWrite(setHCPin, HIGH);
}

void hc12Wake(){
  digitalWrite(setHCPin, LOW);
  DELAY(200);
  digitalWrite(setHCPin, HIGH);
}


void goToDeepSleep()
{
    debugln("A dormir mientras carga...");
    
    // Configure the timer to wake us up! Time in uS
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME * 60 * 1000000);  //Tiempo para despertarse en minutos
    hc12Sleep();  // duerme el transceptor
    Write_1621(6, bat[1]);  //Visualiza el borde de la batería, especialmente cuando está descargada
    levelBat();    //Asegura que al dormir quede visualizando el estado de carga
    // Go to sleep! Zzzz
    
    
    //***Garantiza mensaje de carga lista o suspensión
    if (promValue>4080) {myct ='9'; mydt='5'; myut='P'; }
    else { myct ='S'; mydt='U'; myut='S'; }
    buscachar();   //Visualiza versión del firmware únicamente con la direccion 00 
    Write_1621(4,num[iut]);   // Visualiza primer caracter
    Write_1621(2,num[idt]);   // Visualiza segundo caracter
    Write_1621(0,num[ict]);   // Visualiza tercer caracter
    esp_deep_sleep_start();  //duerme el esp32
}

void estadoBoton(){                                     //Conmuta los estados de LIBRE / OCUPADO
  // Serial.println(conta);
 // if (haboton){

      if (digitalRead(boton)==HIGH) conta=0;                                        //Mantiene el contador en 0 si el botón está en reposo
      if ((digitalRead(boton==LOW)&& conta<2000)) {                                 //Si se pulsa el botón cuenta hasta el tope dado
           if (!esOTA) conta++;
          if (conta>300) DELAY(1);                                                //Después de cierto límite cueenta con retardo
          if (conta==2000)  OTA();                                                 //Al alcanzar el tope final entra a modo OTA  (aprox. 5 segundos manteniendo pulsado el botón)
      }
      if (conta==300) {                                                             //Si se cumple el tope la activación del botón es válida
            contasleep=0;
            libre= !libre;                                                          //complementa el estado libre / ocupado  
           // if(estadop==2) libre=true;                                               //Si el estado anterior es DISPONIBLE, siempre pasa a LIBRE, para PnP local, en web no es aconsejable
            if (libre) {estadop=1; miestadosal="LIBRE"; envialibre(); }
            else {estadop=0; miestadosal="OCUPADO";/*viscolor=true; visRojo();*/ enviaocupado(); }
            estadopant=estadop;
            
            
        }
        
          if (estadopact!=estadopant) {
            visestado=true;
    
      }
 // }
}


void rutina(){
  

  DELAY(250);
  haboton=true;

}

void OTA(){
  pita();
 // DELAY(2000);
  viscolor=true;
  visMagenta();
  esOTA= true;

  //*******************************************************************************************************
  // BLOQUE QUE GESTIONA LA COMUNICACIÓN OTA
    // Connect to WiFi network
                  myct=version[0];
                    mydt=version[1];
                    myut=version[2]; 
                    buscachar();   //Visualiza versión del firmware únicamente con la direccion 00 
                    
                    Write_1621(4,num[iut]);   // Visualiza unidades versión
                    Write_1621(2,num[idt]);   // Visualiza decenas versión
                    Write_1621(0,num[ict]);   // Visualiza centenas versión
  
                    WiFi.begin(ssid, password);
                    Serial.println("");
                    Serial2.println("");

                    // Wait for connection
                    while (WiFi.status() != WL_CONNECTED) {
                      DELAY(500);
                      Serial.print(".");
                      Serial2.print(".");
                      trywifi++;
                      Serial.println(trywifi);
                    //  if (trywifi==30) hc12Sleep();    //si no se conecta en 30 segundos apaga RF para no saturar el canal
                      if (trywifi==120) {
                          
                           
                          pita();  
                         // esp_deep_sleep_start(); 
                         ESP.restart();


                      } 
                    }
                   
                    
                    Serial.println("");
                    Serial.print("Conectado a ");
                    Serial.println(ssid);
                    Serial.print("Direccion IP= ");
                    Serial.println(WiFi.localIP());
                    Serial.print("MAC= ");
                    Serial.println(WiFi.macAddress());
                    Serial.print("Firmware Ver.= ");
                    Serial.println(version);

                    myip= WiFi.localIP().toString();

                    Serial2.println("");
                    Serial2.print("Conectado a ");
                    Serial2.println(ssid);
                    Serial2.print("Direccion IP= ");
                    Serial2.println(WiFi.localIP());
                    Serial2.print("MAC= ");
                    Serial2.println(WiFi.macAddress());
                    Serial2.print("Firmware Ver.= ");
                    Serial2.println(version);
                  //myip= WiFi.localIP().toString();
                    viscolor=true;
                    visTurquesa();


                  //  Serial.println(myip);
                    hayip=true;                          //Habilita visualización de IP
                    pita();
                   
                  

                    /*use mdns for host name resolution*/
                    if (!MDNS.begin(host)) { //http://esp32.local
                      Serial.println("Error estableciendo MDNS responder!");
                      Serial2.println("Error estableciendo MDNS responder!");
                      while (1) {
                        delay(1000);
                      }
                    }
                    //Serial.println("mDNS responder inicializado");
                    //Serial2.println("mDNS responder inicializado");
                    /*return index page which is stored in serverIndex */
                    server.on("/", HTTP_GET, []() {
                      server.sendHeader("Connection", "close");
                      server.send(200, "text/html", loginIndex);
                    });
                    server.on("/serverIndex", HTTP_GET, []() {
                      server.sendHeader("Connection", "close");
                      server.send(200, "text/html", serverIndex);
                    });
                    /*handling uploading firmware file */
                    server.on("/update", HTTP_POST, []() {
                      server.sendHeader("Connection", "close");
                      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
                      ESP.restart();
                    }, []() {
                      HTTPUpload& upload = server.upload();
                      if (upload.status == UPLOAD_FILE_START) {
                        Serial.printf("Update: %s\n", upload.filename.c_str());
                        Serial2.printf("Update: %s\n", upload.filename.c_str());
                        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
                          Update.printError(Serial);
                        }
                      } else if (upload.status == UPLOAD_FILE_WRITE) {
                        /* flashing firmware to ESP*/
                        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                          Update.printError(Serial);
                        }
                      } else if (upload.status == UPLOAD_FILE_END) {
                        if (Update.end(true)) { //true to set the size to the current progress
                          pita();
                          Serial.printf("Actualizacion exitosa: %u\nReiniciando...\n", upload.totalSize);
                          Serial2.printf("Actualizacion exitosa: %u\nReiniciando...\n", upload.totalSize);
                           ESP.restart();   ///reinicia al final de la actualización
                        } else {
                          Update.printError(Serial);
                        }
                      }
                    });
                    server.begin();
 
 estadop=4;
 Serial.println("OTA"); 



}

void visualizaEstado(){
    
    if (visestado){
        if (estadop==0)  { enviaocupado(); leds[0] = CRGB(128, 0, 0); leds[1] = CRGB(128, 0, 0); leds[2] = CRGB(128, 0, 0); Serial.println("OCUPADO");}      //Luz en rojo si el turno es numérico
        if (estadop==1)  { envialibre();   leds[0] = CRGB(0, 128, 0); leds[1] = CRGB(0, 128, 0); leds[2] = CRGB(0, 128, 0); Serial.println("LIBRE");}        //Luz en verde si el turno es numérico
        if (estadop==2)  {                 leds[0] = CRGB(0, 0, 128); leds[1] = CRGB(0, 0, 128); leds[2] = CRGB(0, 0, 128); Serial.println("DISPONIBLE");}   //Si el turno devuelto es ---, no hay turno en fila
        
        FastLED.show();
        visestado=false;
    }
        estadopact=estadopant;

        haboton=false;   //Garantiza que no actúe el botón hasta tanto la rutina se halla ejecutado
      
        rutina();



}



void visRojo(){
 if (viscolor){
    leds[0] = CRGB(64, 0, 0); 
    leds[1] = CRGB(64, 0, 0); 
    leds[2] = CRGB(64, 0, 0);
    FastLED.show(); 
    Serial.println("OCUPADO");
    haboton=true; 
    viscolor=false;  //para que se ejecute solo una vez
  }
}

void visVerde(){
 if (viscolor){
    leds[0] = CRGB(0, 64, 0); 
    leds[1] = CRGB(0, 64, 0); 
    leds[2] = CRGB(0, 64, 0);
    FastLED.show(); 
    Serial.println("LIBRE");
    haboton=true; 
    viscolor=false;  //para que se ejecute solo una vez
  }

}
void visAzul(){
 if (viscolor){
    leds[0] = CRGB(0, 0, 64); 
    leds[1] = CRGB(0, 0, 64); 
    leds[2] = CRGB(0, 0, 64);
    FastLED.show(); 
    Serial.println("DISPONIBLE");
    haboton=true; 
    viscolor=false;  //para que se ejecute solo una vez
   // contadisponible=0;  //EN PRUEBA
  }
}

void visMagenta(){
 if (viscolor){
    leds[0] = CRGB(32, 0, 32); 
    leds[1] = CRGB(32, 0, 32); 
    leds[2] = CRGB(32, 0, 32);
    FastLED.show(); 
    Serial.println("OTA");
    haboton=true; 
    viscolor=false;  //para que se ejecute solo una vez
  }
}

void visTurquesa(){
 if (viscolor){
    leds[0] = CRGB(0, 16, 32); 
    leds[1] = CRGB(0, 16, 32); 
    leds[2] = CRGB(0, 16, 32);
    FastLED.show(); 
    Serial.println("ONLINE");
    haboton=true; 
    viscolor=false;  //para que se ejecute solo una vez
  }
}


void prognumter(){
int conta=0;   //dummy para git sin uso
Serial.println("Programando numter: ");
  if (beingprog){
    for (int i=1;i<8;i++){
      
      numter=i;
     
      //ws2812b.setPixelColor(0, ws2812b.Color(128, 0, 128));  // it only takes effect if pixels.show() is called

      if (numter==1) { leds[2] = CRGB(0, 0, 0);    leds[1] = CRGB(0, 0, 0);   leds[0] = CRGB(32, 0, 32);  }
      if (numter==2) { leds[2] = CRGB(0, 0, 0);    leds[1] = CRGB(32, 0, 32);  leds[0] = CRGB(0, 0, 0);   }
      if (numter==3) { leds[2] = CRGB(0, 0, 0);    leds[1] = CRGB(32, 0, 32);  leds[0] = CRGB(32, 0, 32);  }
      if (numter==4) { leds[2] = CRGB(32, 0, 32);   leds[1] = CRGB(0, 0, 0);   leds[0] = CRGB(0, 0, 0);   }
      if (numter==5) { leds[2] = CRGB(32, 0, 32);   leds[1] = CRGB(0, 0, 0);   leds[0] = CRGB(32, 0, 32);  }
      if (numter==6) { leds[2] = CRGB(32, 0, 32);   leds[1] = CRGB(32, 0, 32);  leds[0] = CRGB(0, 0, 0);   }
      if (numter==7) { leds[2] = CRGB(32, 0, 32);   leds[1] = CRGB(32, 0, 32);  leds[0] = CRGB(32, 0, 32);  }


      FastLED.show();
    //  Serial.println(i); 
      Serial2.print("Actualizando como terminal #: ");
      Serial2.println(numter);
      Serial.print("Actualizando como terminal #: ");
      Serial.println(numter);
      delay(1000);
   

      if((digitalRead(boton)==HIGH)||(numter==7)){
        
        EEPROM.write(0, numter); //Graba en memoria
        EEPROM.commit(); 
        delay(100);
        Serial.print("Programado como terminal #: ");
        Serial.println(numter);
        Serial2.print("Programado como terminal #: ");
        Serial2.println(numter);
        ESP.restart();   //Reinicia para salir del modo programación

      } 
    }
  }

}



/*

https://keraltylaboratoriospruebasd5.cielingenioparalavida.com:5530/Player.aspx?Recurso=2_SELECTORWEBGRUPOETARIOSINIMPRESION&IdOficina=43&IdSelector=43&IdSala=85&IdCodigoSede=167213&IdCodigoDANECiudad=11001&fullscreen=1
http://23.101.113.55:6612/HostServicioTerminalRest.svc

https://keraltylaboratoriospruebasd5.cielingenioparalavida.com:5510/Paginas/GestionInformacion/Turnos.aspx

https://keraltylaboratoriospruebasd5.cielingenioparalavida.com:5510/Paginas/Operacion/ElementosSistema/Terminales.aspx

https://keraltylaboratoriospruebasd5.cielingenioparalavida.com:5510/Paginas/Configuracion/ElementosSistema/Tableros.aspx

https://keraltylaboratoriospruebasd5.cielingenioparalavida.com:5530/Player.aspx?Recurso=4_TABLEROPRIMERPLANOHORIZONTAL4FILASMP&IdOficinaLaboratorios=43&IdTableroLaboratorios=63&IdOficinaImagenes=1&IdTableroImagenes=1&fullscreen=1

ambiente alkosto:
https://comercial.digiturno5.com:8301/Player.aspx?Recurso=Tablero_Alkosto&idoficina=1&idtablero=1&fullscreen=1


MAC DE TERMINALES:
MAC1: e0:5a:1b:22:97:10  **
MAC2: e0:5a:1b:22:96:f8  **
MAC3: e0:5a:1b:22:96:d8
MAC4: e0:5a:1b:22:97:04  
MAC5: e0:5a:1b:22:97:14
MAC6: e0:5a:1b:22:96:d0
MAC7: e0:5a:1b:22:97:00    **



*CONEXIONES HC12*
NARANJA   - +3.3V
NEGRO     - GND
VIOLETA   - RXD
BLANCO    - TXD
CAFE      - SET

CONFIGURACIÓN:
OK+B9600
OK+RC017
OK+RP:+20dBm
OK+FU3
   
mac ter: DC:0D:30:EA:62:47


INI

[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
lib_deps = arkhipenko/TaskScheduler@^3.7.0
           fastled/FastLED@^3.7.0


*/



