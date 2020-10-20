#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "RF24.h"
 
#define adc_disable() (ADCSRA &= ~(1<<ADEN)) // disable ADC (before power-off)
#define adc_enable()  (ADCSRA |=  (1<<ADEN)) // re-enable ADC

#define MAX_CICLOS_SLEEP 30 //se envia un mensaje cada 30 seg
#define PIN_LED 1

//ATtiny85 pins para RF24
// CE and CSN are configurable, specified values for ATtiny85 as connected above
#define CE_PIN 3 // Chip Enable
#define CSN_PIN 4 //Chip Select Not
//#define CSN_PIN 3 // uncomment for ATtiny85 3 pins solution

/* crea la comunicacion por SPI, la libreria maneja el protocolo SPI */
RF24 unRf24(CE_PIN, CSN_PIN); //crea la instancia y la comunicacion por SPI
byte myAddresses[9]="collarRF"; //la direccion de este nodo (transmisor). 
byte baseAddresses[7]="baseRF"; //la direccion de la base (receptor) 

unsigned long payload = 0; //ver campo porque si esta todo el tiempo prendido! tendrias que mandar la hora o algo
unsigned long intervaloEnvio = 30000 ; // intervalo de envio de la señal en ms

void setup()
{
  indicadorCodigoLed (10,200); //indicamos que se configuro
  configurarDormir();
  configurarRf24();
  pinMode(PIN_LED, OUTPUT);
  indicadorCodigoLed (10,200); //indicamos que se configuro
}
 
void loop()
{
  enviarMensajeRF24();
  //espera un tiempo antes de enviar otro mensaje
  dormirCiclos(MAX_CICLOS_SLEEP);
  indicadorCodigoLed (10,200); //indicamos que se configuro
}
 
ISR (WDT_vect) {
  WDTCR |= _BV(WDIE);
  //contadorCiclosEnSleep++;
}

void enviarMensajeRF24(){
  payload++;
  unRf24.write( &payload, sizeof(unsigned long) );  
}

void setearTodosPuertosInput(){
    // Power Saving setup
  for (byte i = 0; i < 6; i++) {
    pinMode(i, INPUT);      // Set all ports as INPUT to save energy
    digitalWrite (i, LOW);  //
  }
}

void dormirCiclos(int ciclos){
  //indicadorCodigoLed (2,500); //indicamos que se duerme
  cli();
  for (int i = 0; i < ciclos; i++) {
    sleep_enable();
    sleep_bod_disable();
    sei();
    sleep_cpu();
    sleep_disable();
    //cuando termine de dormir salta a la funcion ISR
  }
  sei();
  //indicadorCodigoLed (15,100); //indicamos que se levanto
}


//funcion que prende y apag el led para informar el funcionamiento
void indicadorCodigoLed(int cantidad, int tiempo) {
  for (int i = 0; i < cantidad; i++)
  {
    digitalWrite(PIN_LED, HIGH);
    delay(tiempo);
    digitalWrite(PIN_LED, LOW);
    delay(tiempo);
  }
}

void configurarDormir(){
  //setearTodosPuertosInput();
  adc_disable();          // Disable Analog-to-Digital Converter
 
  wdt_reset();            // Watchdog reset
  wdt_enable(WDTO_1S);    // Watchdog enable Options: 15MS, 30MS, 60MS, 120MS, 250MS, 500MS, 1S, 2S, 4S, 8S
  WDTCR |= _BV(WDIE);     // Interrupts watchdog enable
  sei();                  // enable interrupts
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Sleep Mode: max
}

void configurarRf24(){
// Setup and configure rf radio
  unRf24.begin(); // Start up the radio
  unRf24.setAutoAck(1); // Ensure autoACK is enabled

  /* setRetries	(	uint8_t 	delay, uint8_t 	count )
		 Parameters
  delay	How long to wait between each retry, in multiples of 250us, max is 15. 0 means 250us, 15 means 4000us.
  count	How many retries before giving up, max 15 */
  unRf24.setRetries(15,15); // define delay max y reintentos max

  unRf24.openWritingPipe(myAddresses); // Write to device address 
  //radio.openReadingPipe(1,addresses[0]); // Read on pipe 1 for device address '1Node'
  //unRf24.startListening(); // Start listening
  unRf24.stopListening(); // First, stop listening so we can talk.
}
