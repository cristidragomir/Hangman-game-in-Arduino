#include <LiquidCrystal_I2C.h>
#include <LedControl.h>

#define MAX_ROWS 3
#define MAX_COLS 3
#define MAX_CHRS 16
#define MAX_WORDS 6
#define MAX_CHANCES 6
#define DELAY_TIME 2000
#define ENTER 8
#define NOTE_D2  73
#define NOTE_A7  3520
#define NOTE_C7  2093
#define NOTE_G4  392
#define NOTE_B1  62

LiquidCrystal_I2C lcd(0x27,16,2);

int indxWordToGuess = random(0, MAX_WORDS);
int guessedLetters = 0;
int chancesLeft = MAX_CHANCES;
int row, col, i, j, k;
int foundColumn = 0;
int debounce = 300;
int cntPressedBtns[9];
int rowsPins[MAX_ROWS];
int colsPins[MAX_COLS];

char initChars[8];
int pressedButton;
int prevPressedBtn;
int game_status;
int discoveredLetters[MAX_CHRS];
String words[MAX_WORDS + 1];

int DIN = 13;
int CS = 12;
int CLK = 11;
LedControl lc=LedControl(DIN, CLK, CS,0);
byte player[MAX_CHANCES + 1][8] = {
  {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000}, // head cut
  {B01100000,B10010000,B10010000,B01100000,B00000000,B00000000,B00000000,B00000000}, // torso cut
  {B01100000,B10010000,B10010000,B01110000,B00001000,B00000100,B00000000,B00000000}, // the other arm cut
  {B01100000,B10010000,B10010000,B01111100,B00001000,B00000100,B00000000,B00000000}, // arm cut
  {B01100000,B10010000,B10010000,B01111100,B00011000,B00010100,B00000000,B00000000}, // the other leg cut
  {B01100000,B10010000,B10010000,B01111100,B00011000,B00010111,B00000000,B00000000}, // leg cut
  {B01100000,B10010000,B10010000,B01111100,B00011000,B00010111,B00000100,B00000100}  // full-body
};

String substrToDisplay(String msg, int start) {
  String substrToDisplay;
  int cntCh = 0;
  for (int i = start; i < msg.length(); i++) {
    if (msg.charAt(i) == ' ') {
      cntCh = i - start;
    }
    if (i - start + 1 > MAX_CHRS) {
      break;
    }
    if (i == msg.length() - 1) {
      int j = i;
      while (j > start && msg.charAt(j) != ' ') {
        cntCh++;
        j--;
      }
      cntCh++;
    }
  }
  substrToDisplay = msg.substring(start, start + cntCh);
  return substrToDisplay;
}

void displayMessage(String msg, int delayMsg){
  int strLen = msg.length();
  int chsDisplayed = 0;
  while(chsDisplayed < strLen){
    String returnedSubstr = substrToDisplay(msg, chsDisplayed);
    chsDisplayed += returnedSubstr.length();
    chsDisplayed++;
    lcd.setCursor(0,0);
    lcd.print(returnedSubstr);
    if (chsDisplayed < strLen) {
      String returnedSubstr = substrToDisplay(msg, chsDisplayed);
      chsDisplayed += returnedSubstr.length();
      chsDisplayed++;
      
      lcd.setCursor(0,1);
      lcd.print(returnedSubstr);
    }
    if (delayMsg == 1) {
      unsigned long initTime = millis();
      while (millis() - initTime < DELAY_TIME) {}
      lcd.clear();
    }
  }
}

void setRowColPins() {
  rowsPins[0] = 2;
  rowsPins[1] = 3;
  rowsPins[2] = 4;
    
  colsPins[0] = 8;
  colsPins[1] = 9;
  colsPins[2] = 10;

  for (row = 0; row < MAX_ROWS; row++) {
    pinMode(rowsPins[row], OUTPUT);
  }
  for (col = 0; col < MAX_COLS; col++) {
    pinMode(colsPins[col], INPUT_PULLUP);
  }
  for(row = 0; row < MAX_ROWS; row++){
    digitalWrite(rowsPins[row], HIGH);
  }
}

void setKeypadStartLetters() {
  initChars[0] = 'A';
  initChars[1] = 'D';
  initChars[2] = 'G';
  initChars[3] = 'J';
  initChars[4] = 'M';
  initChars[5] = 'P';
  initChars[6] = 'S';
  initChars[7] = 'V';
}

void setWordsList() {
  words[0] = "TRIAJ";
  words[1] = "ARAHNIDA";
  words[2] = "OBSCUR";
  words[3] = "MICROPROCESOR";
  words[4] = "SCULA";
  words[5] = "FANTASTIC";
}

void displayStartingGameMessages() {
  lcd.init();
  lcd.clear();         
  lcd.backlight();
  displayMessage("Bine ati venit!", 1);
  displayMessage("Acest proiect simuleaza jocul de SPANZURATOAREA", 1);
  displayMessage("Poti gresi o litera de maxim 6 ori", 1);
  displayMessage("Mult succes!", 1);
}

void setup() {
  Serial.begin(9600);
  int aRead = analogRead(0);
  indxWordToGuess = aRead % MAX_WORDS;

  setRowColPins();
  displayStartingGameMessages();
  setKeypadStartLetters();
  setWordsList();

  prevPressedBtn = 10;
  pressedButton = 0;
  game_status = 0;
  for (i = 0; i < words[indxWordToGuess].length(); i++) {
    discoveredLetters[i] = 0;
  }

  lc.shutdown(0,false);
  lc.setIntensity(0,0);
  lc.clearDisplay(0);
}

void printGameFstLCDLine() {
  lcd.clear();
  for (i = 0; i < words[indxWordToGuess].length(); i++) {
    lcd.setCursor(i,0);
    if (discoveredLetters[i] == 1) {
      lcd.print(words[indxWordToGuess][i]);
    }
    else{
      lcd.print('_');
    }
  }
}

void printGameSndLCDLine(char chrToPrint) {
  lcd.setCursor(0,1);
  lcd.print("=== ");
  lcd.setCursor(4,1);
  lcd.print(chrToPrint);
  lcd.setCursor(5,1);
  lcd.print(" ===");
}

void printChosenChrToTheLCD(char chrToPrint) {
    printGameFstLCDLine();
    printGameSndLCDLine(chrToPrint);
}

char selectLetter(){
  char chrToPrint = '\0';
  if (pressedButton != prevPressedBtn) {
    for (int k = 0; k < 8; k++) {
      cntPressedBtns[k] = 0;
    }
  }
  if (prevPressedBtn != 8) {
      prevPressedBtn = pressedButton;
    
      chrToPrint = initChars[pressedButton];
      chrToPrint += cntPressedBtns[pressedButton];
      printChosenChrToTheLCD(chrToPrint);
  } else {
      prevPressedBtn = 10;
      printChosenChrToTheLCD('_');
  }
  
  cntPressedBtns[pressedButton]++;
  // Penultima tasta permite selectarea literelor VWXYZ
  if ((cntPressedBtns[pressedButton] == 3 && pressedButton != 7) || 
      (cntPressedBtns[pressedButton] == 5 && pressedButton == 7)) {
    cntPressedBtns[pressedButton] = 0;
  }
  unsigned long initTime = millis();
  while (millis() - initTime < debounce) {}
  return chrToPrint;
}

bool checkIfLetterGood(char chosenLetter) {
  bool isLetterGood = false;
  for (i = 0; i < words[indxWordToGuess].length(); i++) {
    if (chosenLetter == words[indxWordToGuess][i]) {
      discoveredLetters[i] = 1;
      guessedLetters++;
      isLetterGood = true;
    }
  }
  return isLetterGood;
}

void gameOverResetParams() {
  game_status = 2;
  for (int i = 0; i < MAX_CHRS; i++) {
    discoveredLetters[i] = 0;
  }
  guessedLetters = 0;
  indxWordToGuess = random(0, MAX_WORDS);
  prevPressedBtn = 10;
  chancesLeft = MAX_CHANCES;
}

void gameOverGoodEnding() {
  gameOverResetParams();
  displayMessage("Felicitari! Ai salvat omuletul de la executare!", 1);
}

void gameOverBadEnding() {
  gameOverResetParams();
  displayMessage("Ce ghinion! Omuletul a fost spanzurat!", 1);
}

void playSound(int melody[], float noteDurations[], int len) {
  for (int thisNote = 0; thisNote < len; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    float noteDuration = 1000 / noteDurations[thisNote];
    tone(5, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    float pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(8);
  } 
}

void playWrongLetterSound() {
  int melody[] = {
    NOTE_D2, NOTE_D2
  };

  float noteDurations[] = {
    4, 1.5
  };
  playSound(melody, noteDurations, 2);
}

void playCorrectLetterSound() {
  int melody[] = {
    NOTE_A7
  };

  float noteDurations[] = {
    4
  };
  playSound(melody, noteDurations, 1);
}

void playDeathSound() {
  int melody[] = {
    NOTE_C7, NOTE_G4, NOTE_B1
  };
  
  float noteDurations[] = {
    4, 4, 2
  };
  playSound(melody, noteDurations, 3);
}

int letterSubmittedCases(char chosenLetter) {
  lcd.clear();
  for (int k = 0; k < 8; k++) {
    cntPressedBtns[k] = 0;
  }
  displayMessage("Se verifica...", 1);
  bool isLetterGood = checkIfLetterGood(chosenLetter);
  if (guessedLetters == words[indxWordToGuess].length()) {
    gameOverGoodEnding();
    return -1;
  }
  if (isLetterGood) {
    playCorrectLetterSound();
    displayMessage("Ai ghicit o litera", 1);
  } else {
    chancesLeft--;
    if (chancesLeft == 0) {
      for (int j = 0; j < 8; j++) {
       lc.setRow(0, j, player[chancesLeft][j]);
      }
      playDeathSound();
      gameOverBadEnding();
      return -1;
    }
    playWrongLetterSound();
    displayMessage("Ti se va taia o parte a corpului", 1);
  }
  prevPressedBtn = 8;
  selectLetter();
  return 0;
}

void loop() {
   if (game_status == 0) {
     displayMessage("Apasa ENTER pt joc", 0);
   } else if (game_status == 2) {
     displayMessage("Apasa ENTER pt a juca din nou", 0);
   }
   char chosenLetter;
  for (row = 0; row < MAX_ROWS; row++) {
      digitalWrite(rowsPins[row], LOW);
      for (col = 0; col < MAX_COLS; col++) {
         foundColumn = digitalRead(colsPins[col]);
         pressedButton = 3 * row + col;
         if (foundColumn == LOW && pressedButton != ENTER && game_status > 0){
            chosenLetter = selectLetter();
            Serial.println(chosenLetter);
            delay(10);
         } else if (foundColumn == LOW && pressedButton == ENTER && (game_status == 0 || game_status == 2)) {
            switch (game_status) {
              case 0: game_status = 1; break;
              case 2: game_status = 3; break;
            }
            printChosenChrToTheLCD('_');

            unsigned long initTime = millis();
            while (millis() - initTime < debounce) {}
         } else if (foundColumn == LOW && pressedButton == ENTER && (game_status == 1 || game_status == 3)) {
            int chkIfBreak;
            if (prevPressedBtn != ENTER) {
              chkIfBreak = letterSubmittedCases(chosenLetter);
            }
            if (chkIfBreak) {
              break;
            }
         }
      }
      digitalWrite(rowsPins[row], HIGH);
   }
   for (int j = 0; j < 8; j++) {
      lc.setRow(0, j, player[chancesLeft][j]);
   }
}
