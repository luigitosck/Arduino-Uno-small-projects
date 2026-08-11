//=====================================================
// PIN
//=====================================================

// Uscita analogica AO del sensore MQ-2
const int gasPin = A0;

// LED verde = sistema pronto
const int ledVerdePin = 7;

// LED rosso = allarme
const int ledRossoPin = 8;

// Buzzer passivo
const int buzzerPin = 9;


//=====================================================
// TEMPI
//=====================================================

// 10 minuti di warm-up
const unsigned long tempoWarmup = 600000UL;

// 30 secondi di calibrazione
const unsigned long tempoCalibrazione = 30000UL;

// Il valore deve restare sopra soglia per 5 secondi
// prima di far partire l'allarme
const unsigned long tempoPersistenzaAllarme = 5000UL;


//=====================================================
// CALIBRAZIONE
//=====================================================

// Somma di tutte le letture durante i 30 secondi
unsigned long sommaCalibrazione = 0;

// Numero totale di letture fatte
unsigned long numeroLetture = 0;

// Valore medio dell'MQ-2 in aria normale
int baseline = 0;

// Soglia calcolata automaticamente
int sogliaGas = 0;

// Margine sopra il baseline
// Esempio:
// baseline = 330
// soglia = 330 + 150 = 480
const int margineSoglia = 150;


//=====================================================
// STATO DEL SISTEMA
//=====================================================

// Diventa true quando i 30 secondi di calibrazione
// sono stati completati
bool calibrazioneCompletata = false;

// Serve per il filtro temporale dell'allarme
bool conteggioSuperamentoAttivo = false;

// Istante in cui il valore ha iniziato
// a restare sopra soglia
unsigned long inizioSuperamento = 0;


//=====================================================
// SETUP
//=====================================================

void setup() {

  // Avvia il Monitor Seriale
  Serial.begin(9600);

  // Configura LED e buzzer come uscite
  pinMode(ledVerdePin, OUTPUT);
  pinMode(ledRossoPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Stato iniziale:
  // tutti spenti
  digitalWrite(ledVerdePin, LOW);
  digitalWrite(ledRossoPin, LOW);
  noTone(buzzerPin);

  Serial.println("MQ-2 AVVIATO");
  Serial.println("Warm-up di 10 minuti...");
}


//=====================================================
// LOOP PRINCIPALE
//=====================================================

void loop() {

  // Tempo trascorso dall'accensione di Arduino
  unsigned long tempoAttuale = millis();

  // Legge l'uscita analogica del sensore
  // Risultato da 0 a 1023
  int valoreGas = analogRead(gasPin);


  //===================================================
  // FASE 1 - WARM-UP
  //===================================================

  if (tempoAttuale < tempoWarmup) {

    // Durante il warm-up non vogliamo allarmi
    digitalWrite(ledVerdePin, LOW);
    digitalWrite(ledRossoPin, LOW);
    noTone(buzzerPin);

    Serial.print("WARM-UP | MQ-2 = ");
    Serial.println(valoreGas);

    delay(500);

    // Termina questo giro del loop
    return;
  }


  //===================================================
  // FASE 2 - CALIBRAZIONE
  //===================================================

  if (!calibrazioneCompletata) {

    // Quanto tempo è passato dalla fine del warm-up
    unsigned long tempoCalib =
        tempoAttuale - tempoWarmup;

    // Per i primi 30 secondi accumula le letture
    if (tempoCalib < tempoCalibrazione) {

      sommaCalibrazione += valoreGas;
      numeroLetture++;

      // Sistema ancora non pronto
      digitalWrite(ledVerdePin, LOW);
      digitalWrite(ledRossoPin, LOW);
      noTone(buzzerPin);

      Serial.print("CALIBRAZIONE | MQ-2 = ");
      Serial.println(valoreGas);

      delay(200);

      return;
    }


    //=================================================
    // CALCOLO DEL BASELINE
    //=================================================

    // Evita divisione per zero per sicurezza
    if (numeroLetture > 0) {

      baseline =
          sommaCalibrazione / numeroLetture;

    } else {

      baseline = valoreGas;
    }


    // Crea la soglia automaticamente
    sogliaGas =
        baseline + margineSoglia;

    // L'ADC arriva massimo a 1023
    if (sogliaGas > 1023) {
      sogliaGas = 1023;
    }

    calibrazioneCompletata = true;

    Serial.println();
    Serial.println("==========================");
    Serial.println("CALIBRAZIONE COMPLETATA");

    Serial.print("Baseline = ");
    Serial.println(baseline);

    Serial.print("Soglia = ");
    Serial.println(sogliaGas);

    Serial.println("Sistema pronto!");
    Serial.println("==========================");
    Serial.println();

    delay(1000);
  }


  //===================================================
  // FASE 3 - MONITORAGGIO
  //===================================================

  // Nuova lettura
  valoreGas = analogRead(gasPin);

  Serial.print("MQ-2 = ");
  Serial.print(valoreGas);

  Serial.print(" | Baseline = ");
  Serial.print(baseline);

  Serial.print(" | Soglia = ");
  Serial.println(sogliaGas);


  //===================================================
  // CONTROLLO DEL SUPERAMENTO SOGLIA
  //===================================================

  bool allarmeAttivo = false;

  if (valoreGas >= sogliaGas) {

    // Prima lettura sopra soglia:
    // parte il conteggio dei 5 secondi
    if (!conteggioSuperamentoAttivo) {

      inizioSuperamento = millis();

      conteggioSuperamentoAttivo = true;
    }


    // Se è rimasto sopra soglia per almeno 5 secondi
    if (millis() - inizioSuperamento
        >= tempoPersistenzaAllarme) {

      allarmeAttivo = true;
    }

  } else {

    // Se il valore torna sotto soglia prima dei 5 secondi,
    // il timer viene annullato
    conteggioSuperamentoAttivo = false;

    inizioSuperamento = 0;
  }


  //===================================================
  // USCITE
  //===================================================

  if (allarmeAttivo) {

    // GAS SOPRA SOGLIA DA ALMENO 5 SECONDI

    // Spegne il verde
    digitalWrite(ledVerdePin, LOW);

    // Accende il rosso
    digitalWrite(ledRossoPin, HIGH);

    // Buzzer passivo a 1000 Hz
    tone(buzzerPin, 1000);

  } else {

    // SISTEMA PRONTO E NESSUN ALLARME

    digitalWrite(ledVerdePin, HIGH);

    digitalWrite(ledRossoPin, LOW);

    noTone(buzzerPin);
  }


  // Nuova lettura ogni 200 ms
  delay(200);
}