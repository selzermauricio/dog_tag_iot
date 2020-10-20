
/*
* 
* The printDetails output should appear as follows for radio #0:
* 
* STATUS         = 0x0e RX_DR=0 TX_DS=0 MAX_RT=0 RX_P_NO=7 TX_FULL=0
* RX_ADDR_P0-1   = 0x65646f4e31 0x65646f4e32
* RX_ADDR_P2-5   = 0xc3 0xc4 0xc5 0xc6
* TX_ADDR        = 0x65646f4e31
* RX_PW_P0-6     = 0x20 0x20 0x00 0x00 0x00 0x00
* EN_AA          = 0x3f
* EN_RXADDR      = 0x02
* RF_CH          = 0x4c
* RF_SETUP       = 0x03
* CONFIG         = 0x0f
* DYNPD/FEATURE  = 0x00 0x00
* Data Rate      = 1MBPS
* Model          = nRF24L01+
* CRC Length     = 16 bits
* PA Power       = PA_LOW
*
*/
#include "print_base.h"
#include "heltec.h"
#include <SPI.h>
#include "RF24.h"
#include "printf.h"
#include "ThingSpeak.h"
#include "secrets.h"
#include <WiFi.h>

#define LED_ALERTA 25 
int tiempoTotalLoop = 0;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(2, 17); //no cambiar los pines, no todos funcionan en esp32 heltec

struct collar
{
    const byte address[9];
    uint8_t maxReintentos; //cantidad de fallos permitidos, en variable por ahi podemos sumar reintentos en algun caso
    uint8_t contadorReintentos;
    unsigned long intervaloEnvioCollarSeg; //cada 30 seg el collar envia una señal, no vale la pena leer antes
    unsigned long ultimaConexion;       //en seg porque no hace falta tanta precision
    unsigned long ultimoNumPaquete;
};
struct collar unCollar = {
    "collarRF", 3 /* maxReintentos */, 0 /* contadorReintentos */,
    30 /* intervaloEnvioCollarSeg */,
    0 /* ultimaConexion */, 0 /* ultimoNumPaquete */
};

//const byte address[9] = "collarRF";
unsigned long tiempoSegUltimaConfiRF24 = 0; // en seg

//ThingSpeak config
unsigned long myChannelNumber = SECRET_CH_ID;
const char *myWriteAPIKey = SECRET_WRITE_APIKEY;

WiFiClient client;

/**********************************************************/
//Function to configure the radio
void configureRadio()
{
    mostrarConfigurandoRF24();
    radio.begin();
    // Set the PA Level low to prevent power supply related issues since this is a
    // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
    radio.setPALevel(RF24_PA_LOW);
    radio.openReadingPipe(1, unCollar.address);
    // Start the radio listening for data
    radio.startListening();
    radio.printDetails();
    tiempoSegUltimaConfiRF24 = tiempoEnSegundos();
    mostrarFinConfigurandoRF24();
}
/**********************************************************/

//Function para la configuracion del esp32 heltec
void configurarHeltecEsp32()
{
    //you can set band here directly,e.g. 868E6,915E6
    Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, 868E6 /*long BAND*/);
    //display
    Heltec.display->init();
    Heltec.display->flipScreenVertically();
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->clear();
    delay(1000);
}

void setup()
{
    Serial.begin(115200);
    configurarHeltecEsp32(); //configuracion para el esp32 heltec
    //Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, 868E6 /*long BAND*/);
    ThingSpeak.begin(client); // Initialize ThingSpeak
    conectarWifi();
    printf_begin();
    configureRadio();
    mostrarConfiguracion(unCollar.address, unCollar.maxReintentos, unCollar.contadorReintentos,
                         unCollar.intervaloEnvioCollarSeg, unCollar.ultimaConexion,
                         unCollar.ultimoNumPaquete, tiempoSegUltimaConfiRF24);
}

/************************* LOOP *********************************/
void loop()
{
    tiempoTotalLoop = 0;
    long inicioLoop = tiempoEnSegundos();
    
    manejarFallasModuloRF24(10); //cada 10seg verifica si el RF24 funciona
    handleModuloRF24();
    manejarAlertaCollar();
    //si el collar manda señal cada 30s, loop cada 15s
    realizarDelayConProgreso( (unCollar.intervaloEnvioCollarSeg / 2) * 1000 , 100); 
    Serial.println("radio.available()  = " + radio.available() );

    tiempoTotalLoop = tiempoEnSegundos() - inicioLoop;
    mostrarTiempoTotalLoop(tiempoTotalLoop);

} // FIN Loop

/************************* Funciones Util *********************************/
unsigned long tiempoEnSegundos()
{
    return (millis() / 1000);
}

/************************* Funciones modulo RF24*********************************/
//verifica si existe una falla en el modulo RF24
bool existeFallaModuloRF24()
{
    if (radio.getDataRate() != RF24_1MBPS)
    {
        radio.failureDetected = true;
        Serial.print("Radio configuration error detected");
    }
    return radio.failureDetected;
}

void restablecerModuloRF24()
{
    mostrarRstablecerFallaRF24();
    radio.failureDetected = false;
    delay(250);
    configureRadio();
    mostrarFinRstablecerFallaRF24();
}

//  cada intervalo se encarga de las fallas del modulo y lo intenta restablecer
void manejarFallasModuloRF24(uint8_t intervaloSegundos)
{
    if (tiempoEnSegundos() - tiempoSegUltimaConfiRF24 > intervaloSegundos)
    {
        if (existeFallaModuloRF24())
        {
            Serial.println("manejarFallasModuloRF24: existeFallaModuloRF24 " );
            restablecerModuloRF24();
        }
    }
}

void handleModuloRF24()
{ //el collar manda la señal cada 30seg, no hace falta leer todo el tiempo
    bool lecturaExitosa = false;
    unsigned long tiempoNuevaLectura = tiempoEnSegundos() - unCollar.ultimaConexion;
    if (tiempoNuevaLectura >= unCollar.intervaloEnvioCollarSeg) //expiro tiempo lectura??
    {
        lecturaExitosa = leerModuloRF24();
        if ( lecturaExitosa ){
            Serial.println("Lectura con EXITO en tiempo: " + String(tiempoNuevaLectura) );
            unCollar.contadorReintentos = 0 ;
        }
        else{
            Serial.println("Lectura con FALLO en tiempo: " + String(tiempoNuevaLectura) );
            restablecerModuloRF24();
        }
    }
    else
    {
        Serial.println("Todavia no leeo tiempo: " + String(tiempoNuevaLectura));
    }
}

bool leerModuloRF24()
{   
    bool lecturaCorrecta = false;
    mostrarIntentandoLeerRF24();
    if (radio.available())
    {
        Serial.println("Senal disponible...");

        Serial.print("radio.failureDetected = ");
        Serial.println(radio.failureDetected);
        unsigned long failTimer = tiempoEnSegundos(); // Variable for the received timestamp
        while (radio.available() && !(radio.failureDetected) )
        {                                           // While there is data ready
            if (tiempoEnSegundos() - failTimer > 1) //damos 1 segundo para leer la senal
            {
                sumarReintento("Senal disponible, con falla detectada");
                radio.failureDetected = true;
                lecturaCorrecta = false;
                //break;
            }
            else
            {
                radio.read(&unCollar.ultimoNumPaquete, sizeof(unsigned long)); // Get the payload
                unCollar.ultimaConexion = tiempoEnSegundos();
                lecturaCorrecta = true;
            }
        }
        Serial.println("unCollar.ultimoNumPaquete: ");
        Serial.println(unCollar.ultimoNumPaquete);
        mostrarReciboSenalOnled(unCollar.ultimoNumPaquete);
        //escribirNubeNumPaquete(payload);
    }
    else
    {
        sumarReintento("Sin senal");
        lecturaCorrecta = false;
    }
    return lecturaCorrecta;
}

/************************* Funciones Collar*********************************/
bool pasaCondicionesAlerta()
{
    bool pasoCondiciones = true;
    if (unCollar.contadorReintentos >= unCollar.maxReintentos)
    {
        pasoCondiciones = false;
        Serial.println("no pasa condicion, max reintentos alcanzados");
    }
    return pasoCondiciones;
}

void alertarCollarDesconectado()
{
    digitalWrite(LED_ALERTA, HIGH);
    escribirNube(unCollar.ultimoNumPaquete, false, unCollar.ultimaConexion);

    mostrarCollarDesconectado();
}
void avisarCollarConectado()
{
    digitalWrite(LED_ALERTA, LOW);
    escribirNube(unCollar.ultimoNumPaquete, true, unCollar.ultimaConexion);
    mostrarCollarConectado();
}

void manejarAlertaCollar()
{
    if (pasaCondicionesAlerta())
    {
        avisarCollarConectado();
    }
    else //collar desconectado
    {
        alertarCollarDesconectado();
    }
}

//sumar reintento cuando el collar falla o no esta conectado, etc.
//muestra los mensaje que correspondan
void sumarReintento(String causaReintento)
{
    mostrarReintentoOnled(unCollar.contadorReintentos, causaReintento);
    unCollar.contadorReintentos += 1;
}

/************************* Funciones WiFi*********************************/
void conectarWifi()
{
    mostrarConectarWifi(SECRET_SSID);
    WiFi.disconnect();
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(SECRET_SSID, SECRET_PASS);

    unsigned int puntoEjeX = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        mostrarEsperaWifi(puntoEjeX);
        puntoEjeX += 1;
    }
    mostrarConexionExitosaWifi(WiFi.localIP().toString());
}

/************************* Funciones Nube *********************************/
void escribirNube(unsigned long contadorPaquetes, bool collarConectado, unsigned long ultimaConexion)
{
    // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
    // pieces of information in a channel.  Here, we write to field 1.

    // set the fields with the values
    ThingSpeak.setField(1, (int)contadorPaquetes);
    ThingSpeak.setField(2, (int)collarConectado);
    ThingSpeak.setField(3, (int)ultimaConexion);
    //ThingSpeak.setField(3, number3);

    int codigoRespuesta = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    mostrarEscribirNube(contadorPaquetes, collarConectado);
    manejarResponseNube(codigoRespuesta);
}

void manejarResponseNube(int response)
{
    //funcion que maneja la repuesta de la nube, en el caso de algun error se puede tratar aca
    if (response == 200)
    {
        mostrarActualizoExitoNube();
    }
    else
    {
        mostrarActualizoSinExitoNube(response);
    }
}

void realizarDelayConProgreso(uint32_t delayTotalMillis, uint8_t cantidadProgresos)
{
    //cantidadProgresos = es la cantidad de dealy que se van a realizar, mayor cantidad es mas real

    //division entera, se aprox al delay
    const uint32_t tiempoCadaProgreso = delayTotalMillis / cantidadProgresos;
    Serial.println("realizarDealyMs Total: "+String(delayTotalMillis) );
    Serial.println("tiempoCadaProgreso: "+String(tiempoCadaProgreso) );

    for (uint8_t i = 0; i < cantidadProgresos; i++)
    {
        mostrarEsperaDelay( (i+1) * (100/cantidadProgresos) );
        delay(tiempoCadaProgreso);
    }
}