#include <SD.h>
#include <PS2Keyboard.h>
#include <SDWavePlayer.h>

// pins (SD includes SPI pins)
#define SDCS 14
#define PS2CLK 3
#define PS2DAT 4

#define LEDPIN 8

#define PRESSANDPLAY 1
#define LISTENANDPRESS 2

PS2Keyboard keyboard;
SDWavePlayer wavePlayer;
File soundFile, logFile;
char sFileName[6] = "a.wav";
char lFileName[8] = "cat.wav";
char lastKey, newKey;
uint8_t i, mode;

void setup(){
  pinMode(LEDPIN, OUTPUT);
  lastKey = 0;
  mode = PRESSANDPLAY;
  
  // init SD stuff
  pinMode(SDCS, OUTPUT);
  pinMode(10, OUTPUT);
  digitalWrite(10, LOW);
  if (!SD.begin(SDCS)){
    errorStop(1); // couldn't initiate SD card
  }
  // open a file
  if(SD.exists(sFileName)){
    soundFile = SD.open(sFileName);
  }else{
    errorStop(2); // file doesn't exist
  }
  if(!soundFile){
    errorStop(3); // problem opening file
  }
  soundFile.close();
  
  // create log file
  logFile = SD.open("log.txt", FILE_WRITE);
  if(!logFile){
    errorStop(5); // problem opening log
  }
  logFile.println(":");
  logFile.close();
  
  // init keyboard stuff
  delay(1000); //why?
  keyboard.begin(PS2DAT, PS2CLK);
  while(keyboard.available()){
    keyboard.read();
  }
  
  // init wave player
  wavePlayer.begin();
  wavePlayer.enableSixteenBit(false);
}

void errorStop(char e){
  char tmp = e;
  while(e){
    tmp = e;
    while(tmp){
      digitalWrite(LEDPIN, HIGH);
      delay(300);
      digitalWrite(LEDPIN, LOW);
      delay(300);
      tmp--;
    }
    delay(700);
  }
}

void loop(){
  if(mode == PRESSANDPLAY){
    pressAndPlay();
  }else if(mode == LISTENANDPRESS){
    listenAndPress();
  }
}

// press a key and hear a sound (and log keys)
void pressAndPlay(){
  // Continually check for keyboard input.
  // When a key is pressed, start playing the corresponding sound.
  if(keyboard.available()){
    newKey = keyboard.read();
    
    if(!wavePlayer.isPlaying()){
      digitalWrite(LEDPIN, HIGH);
      logFile = SD.open("log.txt", FILE_WRITE);
      if(newKey == PS2_ENTER){
        logFile.println();
      }else if((newKey > 31 && newKey < 127)){
        logFile.write(newKey);
      }else{
        // if it is a special character, write '.'
        logFile.write('.');
      }
      logFile.close();
      // If it has a corresponding sound, play the new key
      if((newKey>47 && newKey<58) || (newKey>64 && newKey<91) || (newKey>96 && newKey<123)){
        sFileName[0] = newKey;
        lastKey = newKey;
        wavePlayer.play(sFileName);
      }else if(newKey == ';'){
        lastKey = newKey;
        lFileName[0] = 'd';
        lFileName[1] = 'a';
        lFileName[2] = 'd';
        wavePlayer.play(lFileName);
      }else if(newKey == 39){
        lastKey = newKey;
        lFileName[0] = 'm';
        lFileName[1] = 'o';
        lFileName[2] = 'm';
        wavePlayer.play(lFileName);
      }else if(newKey == 92){
        lastKey = newKey;
        lFileName[0] = 't';
        lFileName[1] = 'e';
        lFileName[2] = 'o';
        wavePlayer.play(lFileName);
      }else if(newKey == ' '){
        lastKey = newKey;
        lFileName[0] = 'h';
        lFileName[1] = 'l';
        lFileName[2] = 'o';
        wavePlayer.play(lFileName);
      }else if(newKey == ','){
        lastKey = newKey;
        lFileName[0] = 'c';
        lFileName[1] = 'a';
        lFileName[2] = 't';
        wavePlayer.play(lFileName);
      }else if(newKey == '.'){
        lastKey = newKey;
        lFileName[0] = 'd';
        lFileName[1] = 'o';
        lFileName[2] = 'g';
        wavePlayer.play(lFileName);
      }else if(newKey == '/'){
        lastKey = newKey;
        lFileName[0] = 'c';
        lFileName[1] = 'o';
        lFileName[2] = 'w';
        wavePlayer.play(lFileName);
      }else if(newKey == PS2_BACKSPACE){
        lastKey = newKey;
        lFileName[0] = 's';
        lFileName[1] = 'h';
        lFileName[2] = 'p';
        wavePlayer.play(lFileName);
      }else if(newKey == '['){
        lastKey = newKey;
        lFileName[0] = 'h';
        lFileName[1] = 'r';
        lFileName[2] = 's';
        wavePlayer.play(lFileName);
      }else if(newKey == ']'){
        lastKey = newKey;
        lFileName[0] = 'f';
        lFileName[1] = 'r';
        lFileName[2] = 'g';
        wavePlayer.play(lFileName);
      }else if(newKey == '-'){
        lastKey = newKey;
        lFileName[0] = 'r';
        lFileName[1] = 's';
        lFileName[2] = 't';
        wavePlayer.play(lFileName);
      }else if(newKey == '='){
        lastKey = newKey;
        lFileName[0] = 'b';
        lFileName[1] = 'r';
        lFileName[2] = 'd';
        wavePlayer.play(lFileName);
      }else if(newKey == PS2_ENTER){
        lastKey = newKey;
        lFileName[0] = 'e';
        lFileName[1] = 'l';
        lFileName[2] = 'e';
        wavePlayer.play(lFileName);
      }
      digitalWrite(LEDPIN, LOW);
    }
  }
}

// listen to a prompt, "press x", and wait for input
// respond to input
void listenAndPress(){
  //choose a letter at random
  char letter = millis()%26 + 'a';
  // clear keyboard buffer
  while(keyboard.available()){
    keyboard.read();
  }
  //play "press" and letter
  lFileName[0] = 'p';
  lFileName[1] = 'r';
  lFileName[2] = 's';
  wavePlayer.play(lFileName);
  sFileName[0] = letter;
  wavePlayer.play(sFileName);
  // wait for input, then check and play response
  while(!keyboard.available());
  char guess = keyboard.read();
  if(guess == letter){
    //correct
    lFileName[0] = 'c';
    lFileName[1] = 'o';
    lFileName[2] = 'r';
    wavePlayer.play(lFileName);
  }else{
    lFileName[0] = 'i';
    lFileName[1] = 'o';
    lFileName[2] = 'r';
    wavePlayer.play(lFileName);
  }
  // a little delay between turns
  delay(1000);
}
