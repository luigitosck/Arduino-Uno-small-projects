#include <LiquidCrystal.h>
#include <DHT.h>
#include <Servo.h>
#include <IRremote.hpp>

// Ricevitore IR
const int IR_PIN = A4;

// Codici telecomando
#define IR_0          0x16
#define IR_1          0x0C
#define IR_2          0x18
#define IR_3          0x5E
#define IR_4          0x08
#define IR_5          0x1C
#define IR_6          0x5A
#define IR_7          0x42
#define IR_8          0x52
#define IR_9          0x4A

#define IR_EQ         0x19
#define IR_ST_REPT    0x0D

#define IR_DOWN       0x07
#define IR_UP         0x09
#define IR_LEFT       0x44
#define IR_PLAY       0x40
#define IR_RIGHT      0x43

#define IR_VOL_MINUS  0x15
#define IR_VOL_PLUS   0x46

#define IR_POWER      0x45
#define IR_FUNCSTOP   0x47

// 0 significa: nessun comando ricevuto in questo giro
uint16_t comandoIR = 0;
//=====================================================
// DISPLAY LCD
//=====================================================

// Ordine: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 7, 6, 5, 4);

//=====================================================
// SENSORE DHT11
//=====================================================

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

//=====================================================
// SERVO
//=====================================================
int angoloServo = 90;
Servo servoMotore;

const int servoPin = 13;

//=====================================================
// PIN DEL SISTEMA
//=====================================================

// Uscite di allarme
const int ledPin = 8;
const int buzzerPin = 9;

// Pulsante fisico universale per tornare al menu
const int backButtonPin = A0;

// Joystick
const int joystickXPin = A1;
const int joystickYPin = A2;
const int joystickSwitchPin = A3;

// Sensore a ultrasuoni
const int trigPin = 3;
const int echoPin = 10;

//=====================================================
// MACCHINA A STATI
//=====================================================

enum Modalita {
  MENU,
  MODALITA_SERVO,
  MODALITA_PROSSIMITA,
  MODALITA_TEMPERATURA,
  MODALITA_DISPLAY
};

// Modalità iniziale
Modalita modalitaAttuale = MENU;

// Elemento attualmente selezionato nel menu
int voceMenu = 0;

// Numero complessivo di modalità selezionabili
const int numeroVociMenu = 4;

//=====================================================
// SOGLIE JOYSTICK
//=====================================================

// Valore centrale tipico: circa 512
const int joystickBasso = 350;
const int joystickAlto = 650;

// Serve per evitare movimenti troppo veloci nel menu
unsigned long ultimoMovimentoJoystick = 0;
const unsigned long ritardoJoystick = 220;

//=====================================================
// LETTURA PULSANTI E DEBOUNCE
//=====================================================

bool vecchioStatoSwitch = HIGH;
bool vecchioStatoBack = HIGH;

//=====================================================
// TEMPERATURA E UMIDITA
//=====================================================

float temperatura = 0.0;
float umidita = 0.0;

bool letturaDhtValida = false;

unsigned long ultimaLetturaDht = 0;
const unsigned long intervalloDht = 2000;

// Soglia iniziale dell'allarme termico.
// Non è const perché verrà modificata con il joystick.
float sogliaTemperatura = 28.0;

//=====================================================
// PROSSIMITA
//=====================================================

// Soglia iniziale, modificabile con il joystick
int sogliaDistanza = 20;

// Filtro anti-misura-spuria per il sensore a ultrasuoni.
// L'allarme cambia stato solo dopo 2 misure consecutive coerenti.
int conteggioVicino = 0;
int conteggioLontano = 0;
bool allarmeProssimitaStabile = false;

//=====================================================
// EDITOR DEL DISPLAY
//=====================================================

// Matrice che rappresenta i 32 caratteri del display
char grigliaDisplay[2][16];

// Posizione del cursore
int cursoreX = 0;
int cursoreY = 0;

// Indica se bisogna ridisegnare tutta la griglia
bool aggiornaGriglia = true;

//=====================================================
// FUNZIONE DI LETTURA DELLA DISTANZA
//=====================================================

float misuraDistanza() {

  // Porta inizialmente TRIG a livello basso
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Impulso di trigger lungo 10 microsecondi
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Misura la durata dell'impulso ECHO
  // Timeout di 30 ms per evitare blocchi
  unsigned long durata =
      pulseIn(echoPin, HIGH, 30000UL);

  // Nessun eco ricevuto
  if (durata == 0) {
    return -1.0;
  }

  // Conversione tempo → distanza
  // 0,0343 cm/us è circa la velocità del suono
  // Divisione per 2: andata + ritorno
  return durata * 0.0343 / 2.0;
}

//=====================================================
// LETTURA PERIODICA DEL DHT11
//=====================================================

void aggiornaDht() {

  unsigned long tempoAttuale = millis();

  // Il DHT11 deve essere letto circa ogni 2 secondi
  if (tempoAttuale - ultimaLetturaDht < intervalloDht) {
    return;
  }

  ultimaLetturaDht = tempoAttuale;

  float nuovaTemperatura = dht.readTemperature();
  float nuovaUmidita = dht.readHumidity();

  if (!isnan(nuovaTemperatura) &&
      !isnan(nuovaUmidita)) {

    temperatura = nuovaTemperatura;
    umidita = nuovaUmidita;
    letturaDhtValida = true;

  } else {

    letturaDhtValida = false;
  }
}

//=====================================================
// RILEVAMENTO DELLA PRESSIONE DI UN PULSANTE
//=====================================================

// Restituisce true soltanto nel momento della pressione,
// non per tutto il tempo in cui il pulsante resta premuto.
bool pulsanteAppenaPremuto(
    int pin,
    bool &statoPrecedente) {

  bool statoAttuale = digitalRead(pin);

  bool eventoPressione =
      (statoPrecedente == HIGH &&
       statoAttuale == LOW);

  statoPrecedente = statoAttuale;

  return eventoPressione;
}

//=====================================================
// GESTIONE ALLARME GENERALE
//=====================================================

void gestisciAllarme(
    bool allarmeTemperatura,
    bool allarmeProssimita) {

  bool allarmeAttivo =
      allarmeTemperatura ||
      allarmeProssimita;

  if (allarmeAttivo) {

    digitalWrite(ledPin, HIGH);
    digitalWrite(buzzerPin, HIGH);

  } else {

    digitalWrite(ledPin, LOW);
    digitalWrite(buzzerPin, LOW);
  }
}
//=====================================================
// DISEGNO DEL MENU
//=====================================================

void mostraMenu() {

  lcd.noBlink();
  lcd.noCursor();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(">");

  switch (voceMenu) {

    case 0:
      lcd.print("Servo");
      lcd.setCursor(0, 1);
      lcd.print(" SW: entra");
      break;

    case 1:
      lcd.print("Prossimita");
      lcd.setCursor(0, 1);
      lcd.print(" SW: entra");
      break;

    case 2:
      lcd.print("Temp/Umidita");
      lcd.setCursor(0, 1);
      lcd.print(" SW: entra");
      break;

    case 3:
      lcd.print("Editor LCD");
      lcd.setCursor(0, 1);
      lcd.print(" SW: entra");
      break;
  }
}

//=====================================================
// GESTIONE DEL MENU
//=====================================================

void gestisciMenu() {
// Selezione diretta della modalità tramite telecomando

if (comandoIR == IR_1) {
  modalitaAttuale = MODALITA_SERVO;
  lcd.clear();
  return;
}

if (comandoIR == IR_2) {
  modalitaAttuale = MODALITA_PROSSIMITA;
  lcd.clear();
  return;
}

if (comandoIR == IR_3) {
  modalitaAttuale = MODALITA_TEMPERATURA;
  lcd.clear();
  return;
}

if (comandoIR == IR_4) {
  modalitaAttuale = MODALITA_DISPLAY;
  aggiornaGriglia = true;
  lcd.clear();
  return;
}
  int valoreY = analogRead(joystickYPin);

  unsigned long tempoAttuale = millis();

  // Movimento verso l'alto
  if (valoreY > joystickAlto &&
      tempoAttuale - ultimoMovimentoJoystick
          >= ritardoJoystick) {

    voceMenu--;

    if (voceMenu < 0) {
      voceMenu = numeroVociMenu - 1;
    }

    ultimoMovimentoJoystick = tempoAttuale;
    mostraMenu();
  }

  // Movimento verso il basso
  if (valoreY < joystickBasso &&
      tempoAttuale - ultimoMovimentoJoystick
          >= ritardoJoystick) {

    voceMenu++;

    if (voceMenu >= numeroVociMenu) {
      voceMenu = 0;
    }

    ultimoMovimentoJoystick = tempoAttuale;
    mostraMenu();
  }

  // Pressione del pulsante integrato nel joystick
  if (pulsanteAppenaPremuto(
          joystickSwitchPin,
          vecchioStatoSwitch)) {

    switch (voceMenu) {

      case 0:
        modalitaAttuale = MODALITA_SERVO;
        break;

      case 1:
        modalitaAttuale = MODALITA_PROSSIMITA;
        break;

      case 2:
        modalitaAttuale = MODALITA_TEMPERATURA;
        break;

      case 3:
        modalitaAttuale = MODALITA_DISPLAY;
        aggiornaGriglia = true;
        break;
    }

    lcd.clear();
  }
}

//=====================================================
// MODALITA SERVO
//=====================================================

void gestisciModalitaServo() {

  // Legge l'asse X del joystick
  int valoreX = analogRead(joystickXPin);

  //---------------------------------------------------
  // CONTROLLO CON JOYSTICK
  //---------------------------------------------------

  // Joystick verso sinistra
  if (valoreX < joystickBasso) {
    angoloServo -= 2;
  }

  // Joystick verso destra
  if (valoreX > joystickAlto) {
    angoloServo += 2;
  }

  //---------------------------------------------------
  // CONTROLLO CON TELECOMANDO
  //---------------------------------------------------

  // Freccia sinistra
  if (comandoIR == IR_LEFT) {
    angoloServo -= 5;
  }

  // Freccia destra
  if (comandoIR == IR_RIGHT) {
    angoloServo += 5;
  }

  //---------------------------------------------------
  // LIMITI DEL SERVO
  //---------------------------------------------------

  // Impedisce di andare sotto 0° o sopra 180°
  angoloServo = constrain(angoloServo, 0, 180);

  //---------------------------------------------------
  // COMANDO DEL SERVO
  //---------------------------------------------------

  servoMotore.write(angoloServo);

  //---------------------------------------------------
  // DISPLAY
  //---------------------------------------------------

  lcd.setCursor(0, 0);
  lcd.print("CONTROLLO SERVO ");

  lcd.setCursor(0, 1);
  lcd.print("Angolo:");
  lcd.print(angoloServo);
  lcd.write(byte(223));
  lcd.print("    ");

  // Nessun allarme in questa modalità
  gestisciAllarme(false, false);

  // Evita che il joystick faccia muovere
  // il servo troppo velocemente
  delay(20);
}

//=====================================================
// MODALITA PROSSIMITA
//=====================================================

void gestisciModalitaProssimita() {

  int valoreY = analogRead(joystickYPin);
  unsigned long tempoAttuale = millis();

  //---------------------------------------------------
  // CONTROLLO SOGLIA CON JOYSTICK
  //---------------------------------------------------

  if (valoreY > joystickAlto &&
      tempoAttuale - ultimoMovimentoJoystick >= ritardoJoystick) {

    sogliaDistanza++;
    ultimoMovimentoJoystick = tempoAttuale;
  }

  if (valoreY < joystickBasso &&
      tempoAttuale - ultimoMovimentoJoystick >= ritardoJoystick) {

    sogliaDistanza--;
    ultimoMovimentoJoystick = tempoAttuale;
  }

  //---------------------------------------------------
  // CONTROLLO SOGLIA CON TELECOMANDO
  //---------------------------------------------------

  if (comandoIR == IR_UP) {
    sogliaDistanza++;
  }

  if (comandoIR == IR_DOWN) {
    sogliaDistanza--;
  }

  sogliaDistanza = constrain(sogliaDistanza, 5, 200);

  //---------------------------------------------------
  // MISURA DISTANZA
  //---------------------------------------------------

  float distanza = misuraDistanza();
  bool distanzaValida = (distanza > 0.0);

  bool misuraVicino =
      distanzaValida &&
      (distanza <= sogliaDistanza);

  //---------------------------------------------------
  // FILTRO ANTI-MISURE SPURIE
  //---------------------------------------------------

  if (misuraVicino) {

    conteggioVicino++;
    conteggioLontano = 0;

    if (conteggioVicino >= 2) {
      allarmeProssimitaStabile = true;
      conteggioVicino = 2;
    }

  } else {

    conteggioLontano++;
    conteggioVicino = 0;

    if (conteggioLontano >= 2) {
      allarmeProssimitaStabile = false;
      conteggioLontano = 2;
    }
  }

  //---------------------------------------------------
  // DISPLAY
  //---------------------------------------------------

  lcd.setCursor(0, 0);

  if (distanzaValida) {
    lcd.print("Dist:");
    lcd.print(distanza, 0);
    lcd.print("cm      ");
  } else {
    lcd.print("Nessun eco      ");
  }

  lcd.setCursor(0, 1);
  lcd.print("Soglia:");
  lcd.print(sogliaDistanza);
  lcd.print("cm     ");

  //---------------------------------------------------
  // ALLARME
  //---------------------------------------------------

  gestisciAllarme(false, allarmeProssimitaStabile);

  delay(40);
}

//=====================================================
// MODALITA TEMPERATURA E UMIDITA
//=====================================================

void gestisciModalitaTemperatura() {

  //---------------------------------------------------
  // MODIFICA DELLA SOGLIA CON L'ASSE Y
  //---------------------------------------------------

  // Legge la posizione verticale del joystick
  int valoreY = analogRead(joystickYPin);

  // Legge il tempo corrente per limitare la velocità
  // con cui cambia la soglia
  unsigned long tempoAttuale = millis();

  // Joystick verso l'alto:
  // aumenta la soglia di 1 grado
  if (valoreY > joystickAlto &&
      tempoAttuale - ultimoMovimentoJoystick
          >= ritardoJoystick) {

    sogliaTemperatura += 1.0;

    // Limita la soglia tra 0 e 50 °C
    sogliaTemperatura =
        constrain(sogliaTemperatura, 0.0, 50.0);

    ultimoMovimentoJoystick = tempoAttuale;
  }

  // Joystick verso il basso:
  // diminuisce la soglia di 1 grado
  if (valoreY < joystickBasso &&
      tempoAttuale - ultimoMovimentoJoystick
          >= ritardoJoystick) {

    sogliaTemperatura -= 1.0;

    // Limita la soglia tra 0 e 50 °C
    sogliaTemperatura =
        constrain(sogliaTemperatura, 0.0, 50.0);

    ultimoMovimentoJoystick = tempoAttuale;
  }

  //---------------------------------------------------
  // LETTURA DEL DHT11
  //---------------------------------------------------
//---------------------------------------------------
// CONTROLLO SOGLIA CON TELECOMANDO
//---------------------------------------------------

// Freccia su: aumenta la soglia di 1 °C
if (comandoIR == IR_UP) {
  sogliaTemperatura += 1.0;
}

// Freccia giu: diminuisce la soglia di 1 °C
if (comandoIR == IR_DOWN) {
  sogliaTemperatura -= 1.0;
}

// Limiti consentiti
sogliaTemperatura =
    constrain(sogliaTemperatura, 0.0, 50.0);
  aggiornaDht();

  // Se il sensore non ha ancora fornito
  // una lettura valida, mostra un errore
  if (!letturaDhtValida) {

    lcd.setCursor(0, 0);
    lcd.print("Errore DHT11    ");

    lcd.setCursor(0, 1);
    lcd.print("Attendere...    ");

    gestisciAllarme(false, false);

    return;
  }

  //---------------------------------------------------
  // AGGIORNAMENTO DEL DISPLAY
  //---------------------------------------------------

  // Prima riga: temperatura e umidità misurate
  lcd.setCursor(0, 0);

  lcd.print("T:");
  lcd.print(temperatura, 1);
  lcd.write(byte(223));
  lcd.print(" H:");
  lcd.print(umidita, 0);
  lcd.print("%  ");

  // Seconda riga: soglia impostata
  lcd.setCursor(0, 1);

  lcd.print("Soglia:");
  lcd.print(sogliaTemperatura, 0);
  lcd.write(byte(223));
  lcd.print("C     ");

  //---------------------------------------------------
  // CONTROLLO DELL'ALLARME
  //---------------------------------------------------

  // L'allarme si attiva quando la temperatura
  // misurata raggiunge o supera la soglia impostata
  bool temperaturaAlta =
      temperatura >= sogliaTemperatura;

  gestisciAllarme(temperaturaAlta, false);
}
//=====================================================
// INIZIALIZZAZIONE DELL'EDITOR
//=====================================================

void inizializzaGriglia() {

  // Tutte le celle partono dal carattere O
  for (int riga = 0; riga < 2; riga++) {

    for (int colonna = 0;
         colonna < 16;
         colonna++) {

      grigliaDisplay[riga][colonna] = 'O';
    }
  }
}

//=====================================================
// DISEGNO DELL'EDITOR
//=====================================================

void disegnaGriglia() {

  lcd.noBlink();
  lcd.noCursor();

  for (int riga = 0; riga < 2; riga++) {

    lcd.setCursor(0, riga);

    for (int colonna = 0;
         colonna < 16;
         colonna++) {

      lcd.print(grigliaDisplay[riga][colonna]);
    }
  }

  // Posiziona il cursore sulla cella selezionata
  lcd.setCursor(cursoreX, cursoreY);
  lcd.blink();

  aggiornaGriglia = false;
}

//=====================================================
// MODALITA EDITOR DISPLAY
//=====================================================

void gestisciModalitaDisplay() {

  int valoreX = analogRead(joystickXPin);
  int valoreY = analogRead(joystickYPin);

  unsigned long tempoAttuale = millis();

  if (aggiornaGriglia) {
    disegnaGriglia();
  }

  //---------------------------------------------------
  // MOVIMENTO ORIZZONTALE
  //---------------------------------------------------

  // Joystick verso destra
  if (valoreX < joystickBasso &&
      tempoAttuale - ultimoMovimentoJoystick
          >= ritardoJoystick) {

    cursoreX++;

    if (cursoreX > 15) {
      cursoreX = 0;
    }

    ultimoMovimentoJoystick = tempoAttuale;
    aggiornaGriglia = true;
  }

  // Joystick verso sinistra
  if (valoreX > joystickAlto &&
      tempoAttuale - ultimoMovimentoJoystick
          >= ritardoJoystick) {

    cursoreX--;

    if (cursoreX < 0) {
      cursoreX = 15;
    }

    ultimoMovimentoJoystick = tempoAttuale;
    aggiornaGriglia = true;
  }

  //---------------------------------------------------
  // MOVIMENTO VERTICALE
  //---------------------------------------------------

  // Joystick verso il basso
  if (valoreY < joystickBasso &&
      tempoAttuale - ultimoMovimentoJoystick
          >= ritardoJoystick) {

    cursoreY = 1;

    ultimoMovimentoJoystick = tempoAttuale;
    aggiornaGriglia = true;
  }

  // Joystick verso l'alto
  if (valoreY > joystickAlto &&
      tempoAttuale - ultimoMovimentoJoystick
          >= ritardoJoystick) {

    cursoreY = 0;

    ultimoMovimentoJoystick = tempoAttuale;
    aggiornaGriglia = true;
  }
//---------------------------------------------------
// CONTROLLO CURSORE CON TELECOMANDO
//---------------------------------------------------

// DESTRA
if (comandoIR == IR_RIGHT) {

  cursoreX++;

  if (cursoreX > 15) {
    cursoreX = 0;
  }

  aggiornaGriglia = true;
}

// SINISTRA
if (comandoIR == IR_LEFT) {

  cursoreX--;

  if (cursoreX < 0) {
    cursoreX = 15;
  }

  aggiornaGriglia = true;
}

// SU
if (comandoIR == IR_UP) {

  cursoreY = 0;

  aggiornaGriglia = true;
}

// GIU
if (comandoIR == IR_DOWN) {

  cursoreY = 1;

  aggiornaGriglia = true;
}
//---------------------------------------------------
// PLAY = CAMBIA O <-> X
//---------------------------------------------------

if (comandoIR == IR_PLAY) {

  if (grigliaDisplay[cursoreY][cursoreX] == 'O') {

    grigliaDisplay[cursoreY][cursoreX] = 'X';

  } else {

    grigliaDisplay[cursoreY][cursoreX] = 'O';
  }

  aggiornaGriglia = true;
}
  //---------------------------------------------------
  // CAMBIO DEL CARATTERE
  //---------------------------------------------------

  if (pulsanteAppenaPremuto(
          joystickSwitchPin,
          vecchioStatoSwitch)) {

    if (grigliaDisplay[cursoreY][cursoreX] == 'O') {
      grigliaDisplay[cursoreY][cursoreX] = 'X';
    } else {
      grigliaDisplay[cursoreY][cursoreX] = 'O';
    }

    aggiornaGriglia = true;
  }


// Controlla se tutte le 32 celle sono diventate X
bool tutteX = true;

for (int riga = 0; riga < 2; riga++) {
  for (int colonna = 0; colonna < 16; colonna++) {

    if (grigliaDisplay[riga][colonna] != 'X') {
      tutteX = false;
    }
  }
}

// Se tutte le celle sono X, mostra il messaggio
// e attiva LED e buzzer per 4 secondi
if (tutteX) {

  lcd.noBlink();
  lcd.noCursor();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("BRAVA ULISSA !!");

  lcd.setCursor(0, 1);
  lcd.print(":)");

  digitalWrite(ledPin, HIGH);

// Beep 1
digitalWrite(buzzerPin, HIGH);
delay(150);
digitalWrite(buzzerPin, LOW);
delay(100);

// Beep 2
digitalWrite(buzzerPin, HIGH);
delay(150);
digitalWrite(buzzerPin, LOW);
delay(100);

// Beep 3 più lungo
digitalWrite(buzzerPin, HIGH);
delay(350);
digitalWrite(buzzerPin, LOW);
delay(150);

// Pausa
delay(300);

// Finale: tre beep rapidi
for (int i = 0; i < 3; i++) {
  digitalWrite(buzzerPin, HIGH);
  delay(100);

  digitalWrite(buzzerPin, LOW);
  delay(100);
}

// Mantieni il messaggio visibile fino a circa 4 secondi
delay(2400);

digitalWrite(ledPin, LOW);
digitalWrite(buzzerPin, LOW);

  // Ripristina tutte le celle a O
  inizializzaGriglia();

  cursoreX = 0;
  cursoreY = 0;
  aggiornaGriglia = true;
}


  gestisciAllarme(false, false);
}

//=====================================================
// USCITA UNIVERSALE VERSO IL MENU
//=====================================================

void controllaPulsanteIndietro() {

  if (pulsanteAppenaPremuto(
          backButtonPin,
          vecchioStatoBack)) {

    // Torna al menu da qualunque modalità
    modalitaAttuale = MENU;

    // Spegne gli attuatori
    digitalWrite(ledPin, LOW);
digitalWrite(buzzerPin, LOW);
    // Nasconde il cursore dell'editor
    lcd.noBlink();
    lcd.noCursor();

    lcd.clear();
    mostraMenu();

    // Piccolo ritardo antirimbalzo
    delay(100);
  }
}
void aggiornaTelecomando() {

  // Di default nessun nuovo comando
  comandoIR = 0;

  // Se è arrivato un comando...
  if (IrReceiver.decode()) {

    comandoIR = IrReceiver.decodedIRData.command;

    // Prepara il ricevitore per il prossimo comando
    IrReceiver.resume();
  }
}
//=====================================================
// SETUP
//=====================================================

void setup() {
  // Avvia il ricevitore IR.
  // Feedback LED DISABILITATO perché D13 è usato dal servo.
  IrReceiver.begin(IR_PIN, DISABLE_LED_FEEDBACK);
  // Uscite
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  pinMode(trigPin, OUTPUT);

  // Ingressi
  pinMode(echoPin, INPUT);

  // Pulsante fisico e switch del joystick
  // usano le resistenze pull-up interne
  pinMode(backButtonPin, INPUT_PULLUP);
  pinMode(joystickSwitchPin, INPUT_PULLUP);

  // Inizializzazione sensori e periferiche
  dht.begin();

  lcd.begin(16, 2);

  servoMotore.attach(servoPin);
  servoMotore.write(90);

  Serial.begin(9600);

  // Inizializza l'editor con tutte O
  inizializzaGriglia();

  // Mostra il menu iniziale
  mostraMenu();
}

//=====================================================
// LOOP PRINCIPALE
//=====================================================

void loop() {

  // Legge eventuali comandi dal telecomando
  aggiornaTelecomando();

  // Controlla il vecchio pulsante fisico A0
  controllaPulsanteIndietro();

  // POWER del telecomando = torna al menu
  if (comandoIR == IR_POWER) {

    modalitaAttuale = MENU;

    // Spegne eventuale allarme
   digitalWrite(ledPin, LOW);
digitalWrite(buzzerPin, LOW);

    // Sistema il display
    lcd.noBlink();
    lcd.noCursor();
    lcd.clear();

    mostraMenu();

    // Finisce questo giro del loop
    return;
  }

  // Esegue la modalità attualmente selezionata
  switch (modalitaAttuale) {

    case MENU:
      gestisciMenu();
      break;

    case MODALITA_SERVO:
      gestisciModalitaServo();
      break;

    case MODALITA_PROSSIMITA:
      gestisciModalitaProssimita();
      break;

    case MODALITA_TEMPERATURA:
      gestisciModalitaTemperatura();
      break;

    case MODALITA_DISPLAY:
      gestisciModalitaDisplay();
      break;
  }
}