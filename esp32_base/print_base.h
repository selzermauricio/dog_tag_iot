#include "heltec.h"
#include "oled/OLEDDisplay.h"

#define ALTURA_LINEA 10
#define LINEA0 0
#define LINEA1 1 * ALTURA_LINEA 
#define LINEA2 2 * ALTURA_LINEA
#define LINEA3 3 * ALTURA_LINEA
#define LINEA4 4 * ALTURA_LINEA
#define LINEA5 5 * ALTURA_LINEA
//#define LINEA6 6 * ALTURA_LINEA

#define LINEA_OPERACION LINEA0
#define LINEA_MENSAJE LINEA1

#define LINEA_ESTADO_SENAL LINEA1

#define LINEA_TIEMPO_LOOP LINEA2

#define LINEA_ESTADO_NUBE LINEA3
#define LINEA_RESPONSE_NUBE LINEA4

#define LINEA_ESTADO_COLLAR LINEA5

#define TIEMPO_MENSAJE_RF24 500 //ms
#define TIEMPO_MENSAJE_COLLAR 500 //ms
#define TIEMPO_MENSAJE_NUBE 200 //ms

/************************* Funciones Print *********************************/
void imprimirLineaOnledCord(uint8_t x, uint8_t y, unsigned int espera, String texto)
{
    //Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    //Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(x, y, texto);
    Heltec.display->display();
    delay(espera);
}
void limpiarLineaOnled(uint8_t ejeY)
{
    //Heltec.display->clearPixel(0, ejeY);  //no esta implementado en la libreria Heltec fck!
    //imprimirLineaOnledCord(0, ejeY, 0, LINEA_LIMPIA);
    Heltec.display->setColor(BLACK);
    const uint8_t comienzoY = ejeY+2; //por prueba y error 
    const uint8_t finY = ejeY + ALTURA_LINEA +1;
    for (int y=comienzoY; y<= finY; y++)
      {
       for (int x=0; x<Heltec.display->getWidth(); x++)
       {
        Heltec.display->setPixel(x, y);
       }
      } 
     Heltec.display->display();
    Heltec.display->setColor(WHITE);
    
}

void imprimirLineaCompletaOnled( uint8_t y, unsigned int espera, String texto)
{
    limpiarLineaOnled(y);
    imprimirLineaOnledCord(0,y,espera,texto);
}

//muestra en el onled el texto en la posicion 0,0 en una pantalla limpia (clear)
/* void imprimirLineaOnledLimpia(unsigned int espera, String texto)
{
    Heltec.display->clear();
    imprimirLineaOnled(0, LINEA0, espera, texto);
} */


/************************* COLLAR *********************************/
//muestra los datos en Onled cuando se recibe el mensaje
void mostrarReciboSenalOnled(unsigned long contador)
{
    imprimirLineaCompletaOnled( LINEA_OPERACION, 0, "Recibiendo senal collar");
    imprimirLineaCompletaOnled( LINEA_MENSAJE, 0, "mensaje num:");
    imprimirLineaOnledCord(90, LINEA_MENSAJE, TIEMPO_MENSAJE_COLLAR, String(contador));
}

//muestra el estado conectado en Onled
//se puede agregar una imagen para indicar conectividad! (un icono)
void mostrarCollarConectado()
{
    imprimirLineaCompletaOnled( LINEA_ESTADO_COLLAR, 0, "COLLAR: CONECTADO");
    Serial.println("COLLAR: CONECTADO");
}
//muestra el estado conectado en Onled
//se puede agregar una imagen para indicar conectividad! (un icono)
void mostrarCollarDesconectado()
{
    imprimirLineaCompletaOnled( LINEA_ESTADO_COLLAR, 0, "COLLAR: DESCONECTADO");
    Serial.println("COLLAR: DESCONECTADO");
}

//muestra los datos en Onled cuando NO se recibe el mensaje
void mostrarReintentoOnled(uint8_t reintento, String causa)
{
    Serial.print(causa);
    Serial.print("reintento: ");
    Serial.println(reintento);
    imprimirLineaCompletaOnled( LINEA_OPERACION, 0, causa);
    imprimirLineaCompletaOnled( LINEA_MENSAJE, 0, "reintento num: ");
    imprimirLineaOnledCord(90, LINEA_MENSAJE, TIEMPO_MENSAJE_COLLAR, String(reintento));
}
/************************* FIN COLLAR *********************************/

/************************* NRF24 *********************************/
void mostrarRstablecerFallaRF24(){
    Serial.println("Radio failure detected, restarting radio");

    imprimirLineaCompletaOnled( LINEA_OPERACION, 0, "Falla RF24");
    imprimirLineaCompletaOnled( LINEA_MENSAJE, TIEMPO_MENSAJE_RF24, "Restableciendo RF24..");
}
void mostrarFinRstablecerFallaRF24(){
    Serial.println("Finalizo Restablecimiento RF24..");
    imprimirLineaCompletaOnled( LINEA_OPERACION, TIEMPO_MENSAJE_RF24, "Finalizo Restablecimiento RF24..");
}


void mostrarConfigurandoRF24(){
    Serial.println("Configurado nRF24L01..");
    limpiarLineaOnled(LINEA_MENSAJE);
    imprimirLineaCompletaOnled( LINEA_OPERACION, TIEMPO_MENSAJE_RF24, "Configurado nRF24L01..");
}
void mostrarFinConfigurandoRF24(){
    Serial.println("Finalizo Configuracion nRF24L01..");
    limpiarLineaOnled(LINEA_MENSAJE);
    imprimirLineaCompletaOnled( LINEA_OPERACION, TIEMPO_MENSAJE_RF24, "Finalizo Configuracion nRF24L01..");
}

void mostrarIntentandoLeerRF24(){
    Serial.println("Intentando recibir mensaje..");
    limpiarLineaOnled(LINEA_MENSAJE);
    imprimirLineaCompletaOnled( LINEA_OPERACION, TIEMPO_MENSAJE_RF24, "Intentando recibir mensaje..");
}

/************************* FIN NRF24 *********************************/

void imprimirSerialByte(const byte address[9])
{
    for (int i = 0; i < sizeof(address) - 1; ++i)
    {
        Serial.print(address[i]);
    }
    Serial.println("");
}
//muestra config
void mostrarConfiguracion(const byte address[9], uint8_t maxReintentos, uint8_t contadorReintentos,
                          unsigned long intervaloEnvioCollar, unsigned long ultimaConexion,
                          unsigned long ultimoNumPaquete, unsigned long tiempoUltimaConfiRF24)
{
    Serial.println("**************** Configuracion ***************/");
    //collar
    Serial.print("address              = ");
    //Serial.println(address);
    imprimirSerialByte(address);

    Serial.print("maxReintentos        = ");
    Serial.println(maxReintentos);

    Serial.print("contadorReintentos   = ");
    Serial.println(contadorReintentos);

    Serial.print("intervaloEnvioCollar = ");
    Serial.println(intervaloEnvioCollar);

    Serial.print("ultimaConexion       = ");
    Serial.println(ultimaConexion);

    Serial.print("ultimoNumPaquete     = ");
    Serial.println(ultimoNumPaquete);

    //nRF24L01
    Serial.print("tiempoUltimaConfiRF24 = ");
    Serial.println(tiempoUltimaConfiRF24);

    Serial.println("**************** FIN Configuracion ************/");

    //TODO onled
}

//******************* Nube ***************
void mostrarEscribirNube(int contadorPaquetes, int collarConectado)
{
    Serial.println("escribir Nube num paquete = " + String(int(contadorPaquetes)));
    Serial.println("escribir Nube Collar Conectado = " + String(int(collarConectado)));
    //TODO onled
}
void mostrarActualizoExitoNube()
{
    Serial.println("Channel update successful.");
    limpiarLineaOnled(LINEA_RESPONSE_NUBE);
    imprimirLineaCompletaOnled( LINEA_ESTADO_NUBE, TIEMPO_MENSAJE_NUBE, "Actualizo nube");
}
void mostrarActualizoSinExitoNube(int response)
{
    Serial.println("Problem updating channel. HTTP error code " + String(response));
    imprimirLineaCompletaOnled( LINEA_ESTADO_NUBE, 0, "Error actualizar nube");
    imprimirLineaCompletaOnled( LINEA_RESPONSE_NUBE, TIEMPO_MENSAJE_NUBE, "HTTP error code " + String(response));
}
//******************* FIN Nube ***************

//******************* WIFI ***************
void mostrarConectarWifi(String ssid){
    Serial.print("Connecting to ");
    Serial.println(ssid);
    imprimirLineaCompletaOnled(LINEA0, 0, "Conectando SSID: ");
    imprimirLineaCompletaOnled(LINEA1, 0, ssid);
}
void mostrarEsperaWifi(uint8_t progreso){
    // puntitos hasta que conecta
    Serial.print(".");

    // Draws a rounded progress bar with the outer dimensions given by width and height. Progress is
    // a unsigned byte value between 0 and 100
    /* void drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress); */
    limpiarLineaOnled(LINEA3);
    Heltec.display->drawProgressBar(0,LINEA3,112,ALTURA_LINEA,progreso);
    Heltec.display->display();
}
void mostrarConexionExitosaWifi(String localIp){
    Serial.print("IP number assigned by DHCP is ");
    Serial.println(localIp);

    // Draws a rounded progress bar with the outer dimensions given by width and height. Progress is
    // a unsigned byte value between 0 and 100
    /* void drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress); */
    delay(500);
    Heltec.display->drawProgressBar(0,LINEA3,112,ALTURA_LINEA,100);
    imprimirLineaCompletaOnled(LINEA4, 2000, "Conexion Wifi Exitosa");
    Heltec.display->clear();
}

//******************* FIN WIFI ***************

void mostrarTiempoTotalLoop(int tiempoTotal)
{
    imprimirLineaCompletaOnled( LINEA_TIEMPO_LOOP, 0, "Tiempo loop: ");
    imprimirLineaOnledCord(90, LINEA_TIEMPO_LOOP, 100, String(tiempoTotal));
}

void mostrarEsperaDelay(uint8_t progreso){
    // puntitos hasta que termina delay
    Serial.print(".");

    // Draws a rounded progress bar with the outer dimensions given by width and height. Progress is
    // a unsigned byte value between 0 and 100
    /* void drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress); */
    limpiarLineaOnled(LINEA4);
    Heltec.display->drawProgressBar(0,LINEA4+2,112,ALTURA_LINEA-2,progreso);
    Heltec.display->display();
}

