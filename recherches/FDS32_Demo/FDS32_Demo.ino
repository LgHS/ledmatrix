// FDS32 TTL Matrixbord demo software
// Versie 1.6
// Roland Kamp, mei 2015

// De Arduino Nano op het bord bevat de UNO bootloader, dus kies voor de Arduino UNO om het programma te uploaden naar het bord.

// Het matrix bord in opgebouwd uit rijen en kolommen, een timed interrupt routine zorgt constant voor het inklokken van de bits voor een rij,
// daarna wordt de rij aangezet, tot de volgende interrupt, dan herhaalt zich het voor de volgende rij.
// De data voor de display wordt uit een buffer gelezen. De software schrijft dus in die buffer om iets op het display te tonen.
// Op deze manier krijg je een stabiel beeld en hoeft de hoofd routine zich niet met de display timing bezig te houden.
// De rijen worden met een 3-to-8 line decoder (74HCT238) aangestuurd en de clock en data lijnen zijn gebufferd met een 74HCT08.
// Door gebruik van de 74HCT variant kan het bord ook met 3,3 volt aangestuurd worden, in het geval er een andere 
// microcontroller aangesloten wordt die op 3,3 volt werkt. 
// De 74HCT238 heeft 3 enable ingangen, deze zijn alle drie op de header afgewerkt en beschikbaar. Om de rijen aan te zetten moeten 
// enable 1 en 2 laag zijn en enable 3 hoog. Deze software gebruikt enable 1 in de interrupt routine om tijdens het inklokken de rijen uit te zetten,
// Enable 2 wordt gebruikt om eenvoudig in de software de hele display aan of uit te zetten, bijvoorbeeld om het display te laten knipperen.
// Enable 3 wordt nu niet gebuikt, deze is wel met een I/O pin verbonden en wordt constant hoog gehouden. Hier is door PWM nog dimmen van de display mogelijk,
// dat zit niet in deze software.
// Deze I/O pin is eventueel vrij te maken voor ander gebruik, zorg er dan voor dat A3 altijd hoog blijft. Laten zweven van deze ingang mag niet, dit geeft 
// vreemde effecten op de display.
// 
// Door de hardware is bepaald dat de uitgangen van de schuifregisters niet exact uitkomen met het begin van de display, deze sofware 
// houdt rekening met dit verschil en gaat uit van deze meer logische indeling :
//                                                              0                                     89
// Kolommen in deze software  Regel 1 : 0 tot 89                |||||||||||||||||||||||||||||||||||||||
//                                                              90                                    179
//                            Regel 2 : 90 tot 179              |||||||||||||||||||||||||||||||||||||||   
//                                                             180                                    269 
//                            Regel 3 : 180 tot 269             |||||||||||||||||||||||||||||||||||||||  
//
// Deze waarde komt steeds als position terug in de software. De regels tellen in de software als 1, 2 en 3.
//

#include <SPI.h>                   // SPI library, zit standaard bij Arduino
#include <avr/pgmspace.h>          // pgmspace is nodig om tabellen met data in het programmageheugen te laden
#include <TimerOne.h>              // TimerOne library, hier te vinden : https://github.com/PaulStoffregen/TimerOne
                                
// Bit level port macro's
// Deze zijn nodig om snel bits te kunnen besturen in de interrupt. 
// De Arduino functie digitalwrite is te traag
#define cbit(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit));  // clear bit
#define sbit(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit));   // set bit
#define tbit(sfr, bit) (_SFR_BYTE(sfr) ^= _BV(bit));   // toggle bit
                                                       
// Aansluitingen van het matrixboard naar de FDS32 Interface :
#define A1 10        // Enable 1 (is ook SPI SS)   
#define A2 4         // Enable 2                   
#define A3 3         // Enable 3                   
#define DataPin 11   // SPI Data       
#define ClockPin 13  // SPI Clock                      
#define R0 5         // A0 3-to-8 line decoder
#define R1 6         // A1 3-to-8 line decoder
#define R2 7         // A2 3-to-8 line decoder
  
volatile int RowCount = 0;   // Teller for the huidige rij

#include "font75.h";         // Standard 5x7 font ASCII Tabel
#include "charWidth.h"       // Character breedte tabel, wordt gelezen om het font proportioneel te maken
#include "numbers.h"         // Number font, not proportional

volatile unsigned char DisplayBuffer[7][34];   // Dit is de buffer voor de matrix display. 
                                               // De interrupt routine leest deze buffer en zet de juiste led's aan of uit
                                               // De array heeft zeven levels, voor elke row en is 34 bytes diep
                                               // De array is volatile omdat hij gelezen wordt in een interrupt routine

// =========================================================================================
// Setup
// =========================================================================================

 void setup() {   
  pinMode (A1, OUTPUT);
  pinMode (A2, OUTPUT);
  pinMode (A3, OUTPUT);   
  pinMode (R0, OUTPUT);  
  pinMode (R1, OUTPUT);  
  pinMode (R2, OUTPUT);  

  Timer1.initialize(1000);                 // initialize timer1, deze regelt de matrixdisplay schuifregisters en row aansturing, periode in microseconds
  Timer1.attachInterrupt(DisplayControl);  // koppel timer1 interrupt routine
 
  SPI.setClockDivider(SPI_CLOCK_DIV2);     // Stel SPI parameters in, clock op 8 Mhz  
  SPI.setBitOrder(MSBFIRST);               // MSB first  
  SPI.begin();                             // Start SPI

  digitalWrite(A1, LOW);                   // Beginwaarde A1
  digitalWrite(A2, LOW);                   // Beginwaarde A2
  digitalWrite(A3, HIGH);                  // Beginwaarde A3

// Start programma
  
  ClearDisplay();
  PlaceText("FDS32 bord demo",5);
  delay(1000);
  ClearDisplay();
  delay(1000);
 }

// =========================================================================================
// loop routine
// =========================================================================================

void loop()  
{  
  PlaceText("FDS32 bord demo",5);
  PlaceText("DropDownLine",100);
  PlaceText("speed 50",204);
  delay(3000);

  DropDownLine(1,50);
  DropDownLine(2,50);
  DropDownLine(3,50);
  delay(1000);
  
  ClearDisplay();
  PlaceText("FDS32 bord demo",5);
  PlaceText("DropDownSection",90);
  PlaceText("speed 150",200);
  _delay_ms(3000);

  ScrollDownSection(0, 35, 150);
  ScrollDownSection(180, 269, 150);
  _delay_ms(1000);  
  ClearDisplay();

  PlaceText("FDS32 bord demo",5);
  PlaceText("PullUpLine",110);
  PlaceText("speed 50",204);
  delay(3000);
  PullUpLine(2,50);
  PullUpLine(3,50);
  PullUpLine(1,50);
  delay(1000);

  PlaceText("FDS32 bord demo",5);
  PlaceText("EraseLineRight",95);
  PlaceText("speed 15",204);
  delay(3000);
  EraseLineRight(1,15);
  EraseLineRight(2,15);
  EraseLineRight(3,15);
  delay(1000);

  PlaceText("FDS32 bord demo",5);
  PlaceText("EraseLineLeft",95);
  PlaceText("speed 15",204);
  delay(3000);
  EraseLineLeft(1,15);
  EraseLineLeft(2,15);
  EraseLineLeft(3,15);
  delay(1000);
  
  PlaceText("FDS32 bord demo",5);
  PlaceText("BlinkDisplay",105);
  PlaceText("speed 500",200);
  delay(3000);
  
  BlinkDisplay(5, 500);
  ClearDisplay();
  delay(1000);

  PlaceText("FDS32 bord demo",5);
  TickerTape(3, 10, 1, 0, "speed 30       ");
  TickerTape(2, 30, 1, 1, "Dit is een demo van de tickertape functie");
  TickerTape(3, 8, 1, 1, "                       ");
  delay(500);
  TickerTape(3, 10, 1, 0, "speed 10       ");
  TickerTape(2, 10, 1, 1, "The quick brown fox jumps over the lazy dog");
  TickerTape(3, 8, 1, 1, "                       ");

  delay(500);
  TickerTape(3, 10, 1, 0, "spacing 3     "); 
  TickerTape(2, 10, 3, 1, "The quick brown fox jumps over the lazy dog");
  TickerTape(3, 8, 1, 1, "                       ");

  delay(1000);
}

// =========================================================================================
// Functions
// =========================================================================================

// =========================================================================================
// Timer 1 interrupt 
// Hier worden alle bits in de schuiregisters geladen en de rij aangestuurd
// In de praktijk was er wat ghosting zichtbaar op de display, dit is opgelost door een kleine
// delay na het disablen van de rij multiplexer. 5 uS bleek in de praktijk genoeg.
// =========================================================================================

void DisplayControl() 
{  
  sbit(PORTB,2);                                      // Disable de rij multiplexer, nu wordt er geen rij aangestuurd
  delayMicroseconds(5);                               // Voorkom ghosting, minimum is 5 uS
 
  for (int q = 0; q<34; q++)                          // Shift alle bytes in voor de huidige rij 
  {
  SPI.transfer(DisplayBuffer[RowCount][q]);           // Gebruik SPI voor het inklokken van de data
  }
     
  PORTD &= ~(_BV(5) | _BV(6) | _BV(7));               // Alle rij uitgangen laag
  PORTD = (PORTD | RowCount << 5);                    // via een shift en OR de huidige rij op de uitgangen zetten

  cbit(PORTB,2);	                                    // Enable de rij multiplexer

  RowCount++;                                         // Ophogen rij teller
  if (RowCount == 7) RowCount = 0 ;                   // rij telt van 0 tot 6
}  


// =========================================================================================
// Plaats een tekst op de display, gebruikt het proportionele 5x7 font
// invoer : Tekst als string, positie waar de tekst moet starten 
// Er is geen controle of de tekst past
// =========================================================================================

void PlaceText(char text[], int colpos) {
	unsigned char displayByte;
	unsigned char CurChar;
	int bitpos;
	int bytepos;
	int pixelpos;
	int charCount = strlen(text);
        colpos = colpos + 2;	                                            // Corrigeer zichtbare deel van schuifregisters
        int store = colpos;
	unsigned char width;
	bool bitStatus;
 
        for (int i = 0; i < charCount; i++) {                               // Loop voor alle karakters in de string
  	  CurChar = text[i];                                                // Lees ASCII waarde van huidige karakter
  	  CurChar = CurChar - 32;                                           // Min 32 omdat de eerste 32 ASCII codes niet in de tabel staan
          width = pgm_read_byte(&(charWidth[CurChar]));

		for (int y = 0; y<7; y++) {                                       // y loop is voor de 7 rijen 
			for (int x = 0; x<width; x++) {                           // x loop is pointer naar elke bit uit de tabel 
			displayByte = pgm_read_byte(&(Font75[(CurChar*7)+y]));    // Lees byte uit de tabel met behulp van progmem

                        bytepos = colpos/8;                                       // Reken bit en byte positie uit
                        if (bytepos > 33) break;                                  // Bewaking dat er niet voorbij de buffer geschreven wordt
                        bitpos = colpos - (bytepos*8);  
                        bitpos = 7-bitpos;

			bitStatus = (1 << x) & displayByte;                       // Lees bit waarde uit tabel 
			if(bitStatus)
			  {
			  DisplayBuffer[y][bytepos] |= (1 << bitpos);             // set bit in buffer als gelezen bit 1 is
			  }
			  else
			  {
			  DisplayBuffer[y][bytepos] &= ~(1 << bitpos);            // clear bit in buffer als gelezen bit 0 is
			  }
			colpos++;                                                 // Ophogen colpos, voor de volgende kolom
                        }
		        colpos = store;                                           // Reset de kolom teller zodat de volgende byte op dezelfde positie wordt gezet
      		        
		}
		colpos = colpos + width + 1;                                      // Ruimte tussen de karakters, hier wordt het proportioneel gemaakt
		store = colpos;                                                   // Onthoud nieuwe kolom positie voor volgende karakter
            }
}

// =========================================================================================
// Clear display
// Alle bytes in de buffer worden op 0 gezet
// =========================================================================================

void ClearDisplay() {
    for (int y = 0; y<7; y++) {     
       for (int x = 0; x<34; x++) {     
         DisplayBuffer[y][x] = 0;
       }
    }     
}  

// =========================================================================================
// Plaats een enkel karakter (symbool) op de display, gebruik het proportionele 5x7 font
// invoer : ASCII waarde van karakter, positie waar het karakter moet starten 
// Dit kan handig zijn als er speciale karakters gedefinieerd zijn in de ASCII tabel
// =========================================================================================

void PlaceSymbol(unsigned char symbol, int colpos) {
  colpos = colpos + 2;                                                  // Corrigeer zichtbare deel van schuifregisters
  unsigned char displayByte;
  unsigned int bitpos;
  unsigned int bytepos;
  unsigned int store = colpos;
  unsigned char width;

    symbol = symbol - 32;                                               // ASCII min 32 omdat de eerste 32 ASCII codes niet in de tabel staan
    width = pgm_read_byte(&(charWidth[symbol]));                        // Lees breedte van karakter uit de tabel
	
	for (int y = 0; y<7; y++) {                                     // y loop is voor de 7 rijen 
	  for (int x = 0; x<width; x++) {                               // x loop is pointer naar elke bit uit de tabel
                displayByte = pgm_read_byte(&(Font75[(symbol*7)+y]));   // Lees byte uit de tabel met behulp van progmem
                bytepos = colpos/8;                                     // Reken bit en byte positie uit
                bitpos = colpos - (bytepos*8);
                bitpos = 7-bitpos;

                bool bitStatus = (1 << x) & displayByte;                // Lees bit waarde uit tabel
     	        if(bitStatus)
		{
		  DisplayBuffer[y][bytepos] |= (1 << bitpos);           // set bit in buffer als gelezen bit 1 is
		}
		else
		{
		  DisplayBuffer[y][bytepos] &= ~(1 << bitpos);          // clear bit in buffer als gelezen bit 0 is
		}
	   colpos++;
	   }
	colpos = store;                                                 // Reset de kolom teller zodat de volgende byte op dezelfde positie wordt gezet
        }
}

// =========================================================================================
// Toon 4 cijferig getal op de display
// invoer : getal (0 tot 9999), positie waar het getal moet starten op de display
// voorloop nullen worden getoond
// =========================================================================================

void Show4Digit(unsigned int number, unsigned int Position, bool leadingZero) {
	unsigned char thous;
	unsigned char hunds;
	unsigned char tens;
	unsigned char ones;
	unsigned char store = number;
	
	thous = number / 1000;                 
	number = number - ( thous * 1000);
	hunds = number / 100;                  
	number = number - ( hunds * 100);
	tens = number / 10;                    
	number = number - ( tens * 10);   	   
    ones = number;
	
	if (leadingZero == 0) 
	{
	PlaceNumber(ones,Position+18);         
        PlaceNumber(tens,Position+12);         
	PlaceNumber(hunds,Position+6);         
	PlaceNumber(thous,Position+0);         
	}
        else
	{
	PlaceNumber(ones,Position+18);         
	if (store > 9) PlaceNumber(tens,Position+12);         
	if (store > 99) PlaceNumber(hunds,Position+6);         		
        if (thous > 0) PlaceNumber(thous,Position+0);         		
        }
}


// =========================================================================================
// Zet een kolom aan of uit
// invoer : kolom postitie (0 tot 269) en status aan(1) of uit(0)
// =========================================================================================

void OneColumn(int colpos, bool status){
  colpos = colpos + 2;                          // Corrigeer zichtbare deel van schuifregisters
  int bitpos;
  int bytepos;
  
  for (int y = 0; y<7; y++){                    // y loop is voor de 7 rijen
  bytepos = int(colpos/8);                      // Reken bit en byte positie uit 
  bitpos = colpos - (bytepos*8);
  bitpos = 7-bitpos;

  if (status == 1)
   { 
   DisplayBuffer[y][bytepos] |= (1 << bitpos);           // set bit in buffer als status 1 is
   }
   else
   {
   DisplayBuffer[y][bytepos] &= ~(1 << bitpos);          // clear bit in buffer als status 0 is
   }
 }
}

// ======================================================================================================
// Toon een regel tekst als lichtkrant op de display, de tekst scrolt van rechts naar links
// Invoer :  line = de regel waar de tekstop moet komen (1,2 of 3)
//           spacing = afstand tussen de letters
//           blankout = regel opvullen met spaties zodat hij helemaal uit beeld verdwijnt (1=aan, 0=uit)
//           text = de string om te laten scrollen 
//           speed = snelheid van het scrollen
// Opmerking : bij sommige snelheden kan er flikkering onstaan, dit komt als de scroll snelheid dicht
// bij de multiplex frequentie komt. Probeer verschillende waardes uit en kijk welke het beste werkt.
// ======================================================================================================

void TickerTape(unsigned char line, unsigned int speed, unsigned char spacing, bool blankout, char text[])
{
	unsigned char startByte = 0;
	unsigned char startBit = 0;
        unsigned int Shiftfrom = 0;
        unsigned int Shiftto = 0;
	unsigned char displayByte;
	unsigned char CurChar;
	unsigned char width;
	bool bitStatus;
	unsigned char spaceWidth = spacing + pgm_read_byte(&(charWidth[0]));   // Lees de breedte van het spatie karakter
	unsigned int spaceFill = 90 / spaceWidth;			       // Reken aantal spaties uit wat na de tekst moet komen
	unsigned int stringCount = strlen(text);			       // Nieuw totaal aantal karakters in de string
	unsigned int charCount = strlen(text)+(spaceFill*blankout);	       // Totaal aantal karakters om te scrollen (neemt blankout waarde mee)
 
    	switch (line)                // Bepaal voor gekozen regel de parameters
   	{
    	case 1:
    	Shiftfrom = 0;
	    Shiftto = 89;
	    startByte = 11;
    	startBit = 4;
    	break;
    	case 2:
    	Shiftfrom = 90;
    	Shiftto = 179;
    	startByte = 22;
    	startBit = 2;
    	break;
    	case 3:
    	Shiftfrom = 180;
    	Shiftto = 269;
    	startByte = 33;
    	startBit = 0;
   	}

	
	for (int i = 0; i < charCount; i++) {                       // Buitenste loop voor elk karakter in de string

	if (i >= stringCount) {                                     // Vul op met spaties indien nodig (= blankout) 
		CurChar = 32;                                       // Huidige karakter is spatie (lees uit ASCII tabel)
	}
	else
	{
		CurChar = text[i];                                  // Lees ASCII waarde van huidige karakter
	}
		CurChar = CurChar - 32;                             // Min 32, de eerste 32 tekens staan niet in de tabel 
	width = pgm_read_byte(&(charWidth[CurChar]));               // Lees byte uit tabel met progmem 
	
	for (int x = 0; x<(width + spacing); x++) {				// Loop voor elke kolom van het huidige karakter
		for (int y = 0; y<7; y++) {					// Loop voor elke rij
			displayByte = pgm_read_byte(&(Font75[(CurChar*7)+y]));  // Lees byte uit tabel met progmem
			bitStatus = (1 << x) & displayByte;
			if(bitStatus)						// Zet byte op de meest rechter positie van de regel 
			{
			   DisplayBuffer[y][startByte] |= (1 << startBit);	// set bit
			}
			else
			{
			   DisplayBuffer[y][startByte] &= ~(1 << startBit);     // clear bit
			}
		}
             delay(speed);                                                      // Delay, dit bepaalt de snelheid van de lichtkrant  			
                    sectionShiftLeft(Shiftfrom,Shiftto);			// Call functie om hele regel naar links te schuiven
	}
   }
}

// =========================================================================================
// Scroll een sectie van een regel naar links.
// Wordt gebruikt in de tickertape functie
// =========================================================================================
void sectionShiftLeft(unsigned int from,unsigned int to)
{
unsigned int colpos;
unsigned int bitpos;
unsigned int bytepos;
bool ShiftbitStatus;

  for (int p = 0; p<7; p++){                                       
    for(int q=from;q<to;q++){
	// Calculate "from" bit and byte
	colpos = q + 3;
        bytepos = int(colpos/8);
        bitpos = colpos - (bytepos*8);
        bitpos = 7-bitpos;
	ShiftbitStatus = (1 << bitpos) & DisplayBuffer[p][bytepos];

       // Calculate "to" bit and byte 
	colpos = q + 2;
        bytepos = colpos/8;
        bitpos = colpos - (bytepos*8);
        bitpos = 7-bitpos;

	// Do the bit manipulation
	if(ShiftbitStatus == 1)
	{
	  DisplayBuffer[p][bytepos] |= (1 << bitpos);   // set bit
	}
	else
	{
	  DisplayBuffer[p][bytepos] &= ~(1 << bitpos);  // clear bit
	}
      }
   }
}

// =========================================================================================
// Veeg een regel uit van links naar rechts
// Inputs : line, (regel 1, 2 of 3), speed
// =========================================================================================

void EraseLineLeft(unsigned char line, unsigned int speed) {
  line = line -1;
  unsigned int bitpos;
  unsigned int bytepos;
  unsigned int colpos;                       

  for (int z = (line*90); z<(line*90)+90; z++)           // z loop voor kolommen van de regel
  {   
      for (int y = 0; y<7; y++)                          // Y loop voor elke rij
      {
       colpos = z + 2;                                   // Reken bit en byte positie uit 
       bytepos = int(colpos/8);                         
       bitpos = colpos - (bytepos*8);
       bitpos = 7-bitpos;
       
       DisplayBuffer[y][bytepos] &= ~(1 << bitpos);      // clear bit in buffer
    }
  delay(speed);                                          // Delay bepaalt de snelheid
  } 
}

// =========================================================================================
// Veeg een regel uit van rechts naar links
// Inputs : line, (regel 1, 2 of 3), speed
// =========================================================================================

void EraseLineRight(unsigned char line, unsigned int speed) {
  line = line -1;
  unsigned int bitpos;
  unsigned int bytepos;
  unsigned int colpos;                       
    
    for (int z = ((line+1)*90)-1; z >= (line * 90); z--) // z loop voor kolommen van de regel
    {   
      for (int y = 0; y<7; y++)                          // Y loop voor elke rij
      {
       colpos = z + 2;                                   // Reken bit en byte positie uit 
       bytepos = int(colpos/8);                         
       bitpos = colpos - (bytepos*8);
       bitpos = 7-bitpos;
        
       DisplayBuffer[y][bytepos] &= ~(1 << bitpos);      // clear bit in buffer
      }
  delay(speed);                                          // Delay bepaalt de snelheid
    } 
}

// =========================================================================================
// Veeg een regel uit van boven naar beneden
// Inputs : line, (regel 1, 2 of 3), speed
// =========================================================================================

void DropDownLine(unsigned char line, unsigned int speed) {
  line = line -1;
  unsigned int bitpos;
  unsigned int bytepos;
  unsigned int colpos;                       
    
    for (int y = 0; y<7; y++)                          // Y loop voor elke rij
    {   
       for (int z = ((line+1)*90)-1; z >= (line * 90); z--) // z loop voor kolommen van de regel
      {
       colpos = z + 2;                                   // Reken bit en byte positie uit 
       bytepos = colpos/8;                         
       bitpos = colpos - (bytepos*8);
       bitpos = 7-bitpos;
        
       DisplayBuffer[y][bytepos] &= ~(1 << bitpos);      // clear bit in buffer
      }
  delay(speed);                                          // Delay bepaalt de snelheid
    } 
}

// =========================================================================================
// Veeg een regel uit van benden naar boven
// Inputs : line, (regel 1, 2 of 3), speed
// =========================================================================================

void PullUpLine(unsigned char line, unsigned int speed) {
  line = line -1;
  unsigned int bitpos;
  unsigned int bytepos;
  unsigned int colpos;                       
    
    for (int y = 6; y>=0; y--)                             // Y loop voor elke rij
    {   
      for (int z = ((line+1)*90)-1; z >= (line * 90); z--) // z loop voor kolommen van de regel
      {
       colpos = z + 2;                                     // Reken bit en byte positie uit 
       bytepos = colpos/8;                         
       bitpos = colpos - (bytepos*8);
       bitpos = 7-bitpos;
        
       DisplayBuffer[y][bytepos] &= ~(1 << bitpos);        // clear bit in buffer
      }
  delay(speed);                                            // Delay bepaalt de snelheid
    } 
}

// =========================================================================================
// Knipperen van de display, inhoud van de buffer blijft behouden
// Inputs : aantal keer, speed
// =========================================================================================

void BlinkDisplay(unsigned char times, unsigned int speed) 
{
   for (int y = 0; y<times; y++) 
   {     
   digitalWrite(A2, HIGH);
   delay (speed);
   digitalWrite(A2, LOW);
   delay (speed);
   }
 digitalWrite(A2, LOW);  
}  

// =========================================================================================
// Led Test
// Wordt gebruikt voor metingen en led test
// =========================================================================================
void ledTest()
{
   	for (int y = 0; y<90; y++) {            // 1/3 leds aan
		OneColumn(y, 1);
	}
	delay(1000);

	for (int y = 90; y<180; y++) {          // 2/3 leds aan
		OneColumn(y, 1);
	}
	delay(1000);

	for (int y = 180; y<270; y++) {         // Alle leds aan
		OneColumn(y, 1);
	}

	delay(1000);
        ClearDisplay();

        PlaceText("Dit is[test]REGEL",0);
	delay(1000);

        PlaceText("Dit is[test]REGEL",90);
	delay(1000);

        PlaceText("Dit is[test]REGEL",180);
	delay(1000);

        ClearDisplay();
}

// =========================================================================================
// Place number, uses the number font table (non proportional)
// =========================================================================================

void PlaceNumber(unsigned char symbol, unsigned int colpos) {
	unsigned char displayByte;
	unsigned int bitpos;
	unsigned int bytepos;
	colpos = colpos + 2;
	unsigned int store = colpos;
		

	for (int row = 0; row<7; row++) {                                         // y loop is used to handle the 7 rows
		for (int column = 0; column<5; column++) {                            // x loop is the pointer to the individual bits in the byte from the table
			displayByte = pgm_read_byte(&(Numbers[(symbol*7)+row]));          // Read byte from table using progmem
			bytepos = colpos/8;
			bitpos = colpos - (bytepos*8);
			bitpos = 7-bitpos;

			if ((1 << column) & displayByte)                                  // Read bit from table
			{
				DisplayBuffer[row][bytepos] |= (1 << bitpos);                 // set bit in buffer
			}
			else
			{
				DisplayBuffer[row][bytepos] &= ~(1 << bitpos);                // clear bit in buffer
			}
			colpos++;
		}
		colpos = store;                                                     // Reset the column pointer so the next byte is shown on the same position
	}
}


// =========================================================================================
// Scroll een sectie van het bord naar beneden
// =========================================================================================

void ScrollDownSection(unsigned int colstart, unsigned int colstop, unsigned int speed) {
	colstart = colstart + 2;
	colstop = colstop + 2;
	unsigned int bitpos;
	unsigned int bytepos;
    bool bitstatus;

    for (int x = 0; x<7; x++) 
	{
	
		for (int column = colstart; column<(colstop+1); column++) 
		{
			bytepos = column/8;
			bitpos = column - (bytepos*8);
			bitpos = 7-bitpos;
            
			for (int row = 5; row>=0; row--) 
			{
			
				bitstatus = (1 << bitpos) & DisplayBuffer[row][bytepos];
			
				if (bitstatus == 1)
				{
				DisplayBuffer[row+1][bytepos] |= (1 << bitpos);  // set bit in buffer	
				}
				else
				{
				DisplayBuffer[row+1][bytepos] &= ~(1 << bitpos); // clear bit in buffer
				}
			}
              
			  DisplayBuffer[0][bytepos] &= ~(1 << bitpos); // clear top row
		}
             delay(speed);                                                      
  	}
}


