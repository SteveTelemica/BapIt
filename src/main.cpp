// ***********************************************************************************************************************
// BOP IT GAME
// Steve Beadle
// June 2022
// ***********************************************************************************************************************
#include <Arduino.h>
#include <WiFi.h>
#include "SPIFFS.h"

// ***********************************************************************************************************************
// PREFERENCES
// ***********************************************************************************************************************
#include "Preferences.h"
#define EEPROMSET 100
// String length for some settings - 20 char and a 0 terminator
#define SETTINGLENGTH 21
Preferences prefs;

#define STASSID ""
#define STAPSK  ""
char ssid[SETTINGLENGTH] = STASSID;
char password[SETTINGLENGTH] = STAPSK;

byte bootcount = 0;
int highscore = 0;

void setupSettings() {
  prefs.begin("Ver" );
  prefs.begin("HighScore" );
  prefs.begin("ssid" );
  prefs.begin("password" );
  prefs.begin("bootcount" );
  Serial.printf("AppPrefs Created\n\r");
}

void loadSettings() {
  byte v = prefs.getChar("Ver", 0 );
  if (v >= 100) {
  highscore = prefs.getInt("HighScore", 0 );
  prefs.getBytes("ssid", ssid, SETTINGLENGTH );
  prefs.getBytes("password", password, SETTINGLENGTH );
  bootcount = prefs.getUShort("bootcount", bootcount );
  Serial.printf("AppPrefs Loaded. Version %d. High Score %d\n\r", v, highscore);
  } else {
    Serial.printf("AppPrefs never saved\n\r");
  }
}

void saveSettings() {
  prefs.putChar("Ver", EEPROMSET );
  prefs.putInt("HighScore", highscore );
  prefs.putBytes("ssid", ssid, SETTINGLENGTH );
  prefs.putBytes("password", password, SETTINGLENGTH );
  prefs.putUShort("bootcount", bootcount );
  Serial.printf("AppPrefs Saved\n\r");
}


// ***********************************************************************************************************************
// SOUND SECTION
// ***********************************************************************************************************************
bool AudioRunning[2] = {false, false};

#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "AudioOutputMixer.h"

// Sound Name Array
const char *sfile[] = { 

"/00s.mp3", // Start game
"/01s.mp3", // Get ready
"/02s.mp3", // 3 - 2 - 1 
"/03s.mp3", // On your marks - get set
"/04s.mp3", // Stand by
"/05s.mp3", // Let's go
"/06s.mp3", // 
"/07s.mp3", // 
"/08s.mp3", // 
"/09s.mp3", // 
"/10m.mp3", // Bop It
"/11m.mp3", // Spin It
"/12m.mp3", // Pull It
"/13m.mp3", // Twist It
"/14m.mp3", // Flick It
"/15m.mp3", // 
"/16m.mp3", // 
"/17m.mp3", // 
"/18m.mp3", // 
"/19m.mp3", // 
"/20c.mp3", // Keep going
"/21c.mp3", // Well Played
"/22c.mp3", // Nice Moves
"/23c.mp3", // Oh yeah
"/24c.mp3", // 
"/25c.mp3", // 
"/26c.mp3", // 
"/27c.mp3", // 
"/28c.mp3", // 
"/29c.mp3", // 
"/30e.mp3", // Time's up
"/31e.mp3", // Ohhhh
"/32e.mp3", // Uh oh
"/33e.mp3", // Oh dear MISSING
"/34e.mp3", // Whoops
"/35e.mp3", // Game over
"/36e.mp3", // Listen to me MISSING
"/37e.mp3", // 
"/38e.mp3", // 
"/39e.mp3", // 
"/40s.mp3", // Score
"/41s.mp3", // You scored
"/42s.mp3", // You got
"/43s.mp3", // 
"/44s.mp3", // 
"/45s.mp3", // 
"/46s.mp3", // 
"/47s.mp3", // 
"/48s.mp3", // 
"/49s.mp3", // 
"/50h.mp3", // Current High Score
"/51h.mp3", // That's a new high score
"/52h.mp3", // You're in the lead
"/53h.mp3", // That puts you in front
"/54h.mp3", // That's the best score of the day
"/55h.mp3", // 
"/56h.mp3", // 
"/57h.mp3", // 
"/58h.mp3", // 
"/59h.mp3", // 
"/60n.mp3", // Zero
"/61n.mp3", // One
"/62n.mp3", // Two
"/63n.mp3", // Three
"/64n.mp3", // Four
"/65n.mp3", // Five
"/66n.mp3", // Six
"/67n.mp3", // Seven
"/68n.mp3", // Eight
"/69n.mp3", // Nine
"/70n.mp3", // Ten
"/71n.mp3", // Eleven
"/72n.mp3", // Twelve
"/73n.mp3", // Thirteen
"/74n.mp3", // Fourteen
"/75n.mp3", // Fifteen
"/76n.mp3", // Sixteen
"/77n.mp3", // Seventeen
"/78n.mp3", // Eighteen
"/79n.mp3", // Nineteen
"/80n.mp3", // Twenty
"/81n.mp3", // Thirty
"/82n.mp3", // Forty
"/83n.mp3", // Fifty
"/84n.mp3", // Sixty
"/85n.mp3", // Seventy
"/86n.mp3", // Eighty
"/87n.mp3", // Ninety
"/88n.mp3", // ... Hundred
"/89n.mp3", // ... and ...
"/beats1.mp3", // HighTone = 90
"/beats2.mp3" // LowerTone = 91
};

#define SOUNDFILEMAX 92 // 90 // 41 // One more than above largest
bool fileexists[ SOUNDFILEMAX];

// Sound Objects
AudioFileSourceSPIFFS *afsource[2];

// Two parallel outputs
AudioGeneratorMP3 *wav[2];
AudioOutputI2S *out;
AudioOutputMixer *mixer;
AudioOutputMixerStub *stub[2];

int gain[2];

void AudioSetup( void) {
  // Allow logging of audio errors
  audioLogger = &Serial;

  // Open the Files we can find
  for (int i = 0; i < SOUNDFILEMAX; i++ ) {
    // Check file exists
    if (SPIFFS.exists( sfile[ i])) {
      printf("File exists %s\n\r", sfile[ i] );
      fileexists[i] = true;
    } else {
      fileexists[i] = false;
    }
  }

  // Output
  out = new AudioOutputI2S();
  mixer = new AudioOutputMixer(32, out);

  // Rhythm channel
  afsource[0] = new AudioFileSourceSPIFFS( sfile[ 0] ); // Any file
  stub[0] = mixer->NewInput();
  gain[0] = 2;
  stub[0]->SetGain(((float)(gain[0]))/10.0);
  wav[0] = new AudioGeneratorMP3();
  AudioRunning[0] = false;

  // Action channel
  afsource[1] = new AudioFileSourceSPIFFS( sfile[ 1] ); // Any file
  stub[1] = mixer->NewInput();
  gain[1] = 2;
  stub[1]->SetGain(((float)(gain[1]))/10.0);
  wav[1] = new AudioGeneratorMP3();
  AudioRunning[1] = false;
}

// Play on channel a given filename
void AudioStart( int chan, int selection) {
  if ( fileexists[ selection ]) {
    //Start output if no sound running
    if (!AudioRunning[0] && !AudioRunning[1]) {
      out->begin();
    }
    //select file and open file handle
    afsource[chan]->open( sfile[ selection ] );
    stub[chan]->SetGain( ((float)(gain[chan]))/10.0);
    wav[chan]->begin( afsource[ chan ], stub[chan]);
    AudioRunning[chan] = true;
    Serial.printf("Start Sound %d.\r\n", selection );
  } else {
    Serial.printf("INVALID Sound %d.\r\n", selection );
  }
}

// Play next bit of the channel
bool AudioPlay( int chan) {
  if (AudioRunning[chan]) {
    if (wav[chan]->isRunning() ) {
      if (!wav[chan]->loop() ) {
        afsource[chan]->close(); // Close file handle
        wav[chan]->stop();
        stub[chan]->stop();
        Serial.printf("stopped chan %d\r\n", chan);
        AudioRunning[chan] = false;
        stub[chan]->SetGain( 0);

        //If both chan stopped
        if (!AudioRunning[0] && !AudioRunning[1]) {
          out->stop();
        }

        return false; // stopped
      } else {
        return true; // running
      }
    } else {
      AudioRunning[chan] = false;
      return false; // not running
    }
  } else {
    return false; // not started
  }
}

void AudioStop( int chan) {
  afsource[chan]->close(); // Close file handle
  wav[chan]->stop();
  stub[chan]->stop();
  Serial.printf("Early stopped chan %d\r\n", chan);
  AudioRunning[chan] = false;
  stub[chan]->SetGain( 0);
}

// Start game - before 
void AudioPlayStart() {
  AudioStart( 1, 0 );
}

// Game got going - get ready etc
void AudioPlayBegin() {
  // 1,2,3,4,5 = 1 + 0..4
  AudioStart( 1, 1 + random( 5) );
}

// Tone beat, 0 or 1
void AudioPlayBeat(int select) {
  AudioStart( 0, 90 + select);
}

// Commands on mix 1, 0-4
void AudioPlayCommand( int action) {
  if ( (action >= 0) && (action <= 4) ) {
    AudioStart( 1, 10 + action);
  }
}

// Encouragement Interval
void AudioPlayInterval() {
  // 20,22,23 
  int r[3] = {20,22,23};
  AudioStart( 1, r[random( 3)] );
}

// Oh no etc!
void AudioPlayWrong() {
  // 31,32,34 
  int r[4] = {31,32,34};
  AudioStart( 1, r[random( 3)] );
}

// Game Over
void AudioPlayGameOver() {
  AudioStart( 1, 35 );
}

// You scored
void AudioPlayScore() {
  // 40,41,42 = 40 + 0..2
  AudioStart( 1, 40 + random( 3) );
}

// High Score
void AudioPlayHighScore() {
  // 51-54 = 51 + 0..3
  AudioStart( 1, 51 + random( 4) );
}

// Current High Score
void AudioPlayCurrentHighScore() {
  AudioStart( 1, 50 );
}

// Volume check
void AudioPlayOhYeah() {
  AudioStart( 1, 23 );
}

// Numbers
void AudioPlayNumber(int n) {
  Serial.printf("apnum %d\n\r", n);
  if (n <= 20) {   // 0-20, 0->60
    Serial.printf("<=21\n\r");
    AudioStart( 1, 60 + n );
    return;
  }
  if (n < 100) {  // 20-90, 20->80
    Serial.printf("<100\n\r");
    AudioStart( 1, 78 + n/10);
    return;
  }
  if (n == 100) {
    Serial.printf("100\n\r");
    AudioStart( 1, 88);
    return;
  }
  AudioStart( 1, 89); // AND
  Serial.printf("AND\n\r");
}


// ***********************************************************************************************************************
// SWITCH & LED IO
// ***********************************************************************************************************************
int switchbits() {
  // Set a bit for each input, active low
  int x = ((digitalRead(34) == 0) ?  1 : 0 ) + // Bop
          ((digitalRead(35) == 0) ?  2 : 0 ) + // Spin
          ((digitalRead(32) == 0) ?  4 : 0 ) + // Pull
          ((digitalRead(33) == 0) ?  8 : 0 ) + // Twist
          ((digitalRead(19) == 0) ? 16 : 0 );  // Flick
  return x;
}

void setLED( int action) {
  digitalWrite(13, false);
  digitalWrite(12, false);
  digitalWrite(14, false);
  digitalWrite(27, false);
  digitalWrite(18, false);
  switch( action) {
    case 0:
      digitalWrite(13, true);
      break;
    case 1:
      digitalWrite(12, true);
      break;
    case 2:
      digitalWrite(14, true);
      break;
    case 3:
      digitalWrite(27, true);
      break;
    case 4:
      digitalWrite(18, true); // was 26
      break;
  }
}

int switchstate = -1;

// ***********************************************************************************************************************
// SPIFFS
// ***********************************************************************************************************************
bool fsOK;
void SetupSPIFFS() {
  if ( !SPIFFS.begin() ) {
    printf("Error mounting SPIFFS\n\r");
    fsOK = false;
  } else {
    printf( "SPIFFS Files:\n\r");
    fsOK = true;
    File root = SPIFFS.open( "/", "r");
    File file = root.openNextFile();
    while ( file ) {
        String fileName = file.name();
        size_t fileSize = file.size();
        printf( "%s = %d\n\r", fileName.c_str(), fileSize );
        file = root.openNextFile();
    }
  }
}

// ***********************************************************************************************************************
// SETUP
// ***********************************************************************************************************************
unsigned long lastmillis = 0;

void setup() {
  WiFi.mode(WIFI_OFF); 
  Serial.begin(115200);
  Serial.printf("Start\r\n");

  setupSettings();
  loadSettings();
  // I2S
  // DIN  22
  // BCLK 26
  // LRC  25

  // IO, 34-39 are input only
  // LEDs
  pinMode( 13, OUTPUT); //Bap
  pinMode( 12, OUTPUT); //Spin
  pinMode( 14, OUTPUT); //Pull
  pinMode( 27, OUTPUT); //Twist
  pinMode( 18, OUTPUT); //Flick (was 26)

  // Inputs
  pinMode( 34, INPUT_PULLUP); //Bap - no pullup, add R
  pinMode( 35, INPUT_PULLUP); //Spin - no pullup, add R
  pinMode( 32, INPUT_PULLUP); //Pull
  pinMode( 33, INPUT_PULLUP); //Twist
  pinMode( 19, INPUT_PULLUP); //Flick (was 25)

  // Swirl the LEDs
  for (int i = 0; i < 5; i++) {
    setLED(i);
    delay(200);
  }
  setLED(-1);

  // Reset of high score - switch on and hold Pull It down
  if ( switchbits() & 4) {
    highscore = 0;
    Serial.printf("High Score Reset\r\n");
    // Flash LEDS a bit more
    for (int i = 0; i < 5; i++) {
      setLED(i);
      delay(500);
    }
    setLED(-1);
  }

  // Setup SPIFFS
  SetupSPIFFS();

  // audioLogger = &Serial;
  AudioSetup();

  lastmillis = millis();
  randomSeed( millis());

} // setup()

// ***********************************************************************************************************************
// GAME CONTROL
// ***********************************************************************************************************************

int nextaction = 0;
int state = -1;
// -1 == Show how-to-start message
//  0 == Waiting to start
//  1 == Announce action, wait for guard interval
int guardinterval = 200;
//  2 == guard done, wait for switch or timeout
//  3 == got action right, wait for next cycle
unsigned long cyclemillis = 0;
int cycleinterval = 2000;
//  4 == got action wrong
//  5 == wait to start
//  6 == speak high score
//  7 == change volume

// substate used for sound file playing
int substate = 0;
// Player has a minute to play repeatedly.
unsigned long timedgameends = 0;
int score = 0;

// BeatTime
unsigned long BeatTime = 0;

int getcycleinterval( int score) {
  if (score <= 2)
    return 2500;
  if (score <= 5)
    return 2200; //-300
  if (score <= 10)
    return 2000; //-200
  if (score <= 15)
    return 1850; //-150
  if (score <= 20)
    return 1750; //-100
  if (score <= 30)
    return 1650; //-100
  if (score <= 40)
    return 1550; //-100
  if (score <= 50)
    return 1450; //-100
    return 1350; //-100
}

void ResetScoreTimerState() {
  state = 3; // Start by playing rythm and not expect action (3 = command correct)
  cyclemillis = millis();
  score = 0;
  setLED(-1);
  cycleinterval = getcycleinterval(score);
  // Play get ready etc.
  AudioPlayBegin();
}

// ***********************************************************************************************************************
// LOOP
// ***********************************************************************************************************************
void loop() {
  unsigned long thismillis = millis();

  // How to start, start in state -1 but wait for last sound if playing
  // Consider adding a time delay after the sound stops, or blank sound?
  if (state == -1 && !AudioRunning[1]) {
    Serial.printf("Press s or BapIt to Start.\r\n");
    setLED( -1);
    state = 0;
    // Wait one second, not elegant but simple and direct, only do it in this mode
    delay(1000);
    // Say Start Game when preparing 
    AudioPlayStart();
  }

  // Wait to start
  if (state == 0) {
    // Check Serial
    char input[1];
    input[0] = 0;
    if (Serial.available()) {
      int c = Serial.readBytes( input, 1);
      Serial.printf("Pressed: %c\r\n", input[0] );
    }
    int startswitch = switchbits();

    // Start a game - s or BopIt
    if (input[0] == 's' || (startswitch & 1)) {
      Serial.printf("Started\r\n");
      ResetScoreTimerState(); // Start from state 3

      // Player has a minute to play repeatedly.
      timedgameends = thismillis + 60000L;
    }

    // Read the high score - h or Pull
    if (input[0] == 'h' || (startswitch & 4)) {
      Serial.printf("Request high score\r\n");
      state = 6;
      substate = 0;
    }

    // Change volume - v or Flick
    if (input[0] == 'v' || (startswitch & 16)) {
      Serial.printf("Vol change\r\n");
      state = -1; // Same state
      // Gain 0.2, 0.4, 0.6, 0.8, 1.0
      gain[ 0] += 2;
      if (gain[0] > 10) gain[0] = 2;
      gain[ 1] += 2;
      if (gain[1] > 10) gain[1] = 2;
      // Say volume number
      AudioPlayNumber( gain[1] );
    }

  }

  // Next sound/request cycle
  // Either user was right or did not action
  if (state == 3 || state == 2) {
    if (thismillis - cyclemillis > cycleinterval) {
      // If right get next random 0..4
      if (state == 3) {
        // Could be an Interval or a normal play
        // If interval then we expect no switches by expecting action 5
        if ( ( score + 1 ) % 10 ) {
          nextaction = random(5);
          Serial.printf("Please do: %d\r\n", nextaction);
          AudioPlayCommand( nextaction);
          // lead then blank then confuse with LEDs
          if (score <= 8) {
            setLED( nextaction);
          } else {
            if (score >= 15) {
              setLED( random(5) );
            }
          }
        } else {
          nextaction = 5;
          Serial.printf("Interval\r\n");
          AudioPlayInterval();
        }
        cycleinterval = getcycleinterval(score);
        state = 1;
        cyclemillis = thismillis;
        // Flush serial
        while (Serial.available()) {
          char input[1];
          int c = Serial.readBytes( input, 1);
        }
        // play first beat
        AudioPlayBeat(0);
        // next beat is half the period after this one
        BeatTime = thismillis + cycleinterval/2;
      } else {
        // Did not respond, either it was an Interval
        if ( nextaction == 5) {
          state = 3;
          Serial.printf("Interval End\r\n");
          score++; // Must add to score - well done for not actioning!
        } else {
          // Or ... Did not respond!
          state = 4;
          substate = 0;
          Serial.printf("OUT OF TIME!\r\n");
        }
      }
    }
  }
  // If waiting for guard interval
  if (state == 1) {
    if (thismillis - cyclemillis > guardinterval) {
      // now start watching for switches
      state = 2;
      // clear LED
      setLED( -1);
      // record last switch state
      switchstate = switchbits();
    }
  }
  // play beat and stop
  if ( (state == 3 || state == 2) && (thismillis > BeatTime) ) {
    AudioPlayBeat(1);
    BeatTime = thismillis + cycleinterval; // Don't beat again
  }
  // Waiting for switches
  if (state == 2) {
    int codeinput = -1; // Nothing
    // Check switches
    int newswitchstate = switchbits();
    if (switchstate != newswitchstate) {
      // Something changed
      int switchchange = newswitchstate ^ switchstate;
      // Which bit flipped
      codeinput =  switchchange &  1 ? 0 : 
                  (switchchange &  2 ? 1 : 
                  (switchchange &  4 ? 2 : 
                  (switchchange &  8 ? 3 : 
                  (switchchange & 16 ? 4 : -1 ) ) ) );
      // show workings for debug
      Serial.printf("Switch: %d %d %d So: %d\r\n", switchstate, newswitchstate, switchchange, codeinput );
      //setLED( codeinput); // to test what was pressed
      // Save last switch state
      switchstate = newswitchstate;
    }
    // Check serial
    if (Serial.available()) {
      char input[1];
      int c = Serial.readBytes( input, 1);
      Serial.printf("Pressed: %c\r\n", input[0] );
      codeinput = input[0] - '0';
    }
    // If a code was entered
    if (codeinput != -1) {
      // If Correct? (will be wrong if in interval)
      if (codeinput == nextaction) {
        // Yes, next state
        Serial.printf("OK\r\n");
        state = 3;
        score++;
      } else {
        // WRONG action!
        state = 4;
        substate = 0;
        // Light the right action
        setLED( nextaction);
      }
    }
    // else still waiting
  }

  // Whoops!
  // substate set to 0 when entering state 4 end of round
  static bool newhigh;
  // also wait for sound chan 1 to stop
  if ( state == 4 && !AudioRunning[1] ) {
    // substate for which part of score to say
    // increment substate on leaving.
    switch (substate) {
      case 0:
        newhigh = false;

        // Play wrong sound
        AudioPlayWrong();

        if (score > highscore) {
          highscore = score;
          newhigh = true;
        }
        Serial.printf("WRONG! Score %d, High Score %d\r\n", score, highscore);
        if (newhigh) {
          Serial.printf("Well done!\r\n");
          saveSettings();
        }
        break;
      case 1:
        if (newhigh) {
          Serial.printf("Say High Score\r\n");
          AudioPlayHighScore();
        } else {
          Serial.printf("Say Score\r\n");
          AudioPlayScore();
        }
        break;
        // Start playing the score
        // 2    3        4    5      6
        // 1-9  hundred  and  30-90  0-20
      case 2:
        if (score >= 100) {
          AudioPlayNumber( score / 100); // "One" to "Nine"
        }
        break;
      case 3:
        if (score >= 100) {
          AudioPlayNumber( 100); // "Hundred"
        }
        break;
      case 4:
        if (score > 100) {
          AudioPlayNumber( 101); // "AND"
        }
        break;
      case 5:
        if ((score % 100) >= 20) {
          AudioPlayNumber( score % 100 ); // "twenty" to "ninety" 
        }
        break;
      case 6:
        // 10..19
        if ( ( (score % 100) >= 10) && ( (score % 100) < 20) ) {
          AudioPlayNumber( score % 100 ); // "ten" to "nineteen" 
        } else {
          // 1..9, only if not 10-19
          if ((score % 10) >= 1)  {
            AudioPlayNumber( score % 10 ); // "one" to "nine" 
          }
        }
        if (score == 0) {
          AudioPlayNumber( 0 ); // "zero" 
        }
        break;
      default:
        // If a whole minute is up, game ends outright
        if (timedgameends < thismillis) {
          Serial.printf("GAME OVER 1 MIN END\r\n");
          //AudioStop(0); // Stop the beat
          AudioPlayGameOver();
          // state -1 Hangs about to say stuff before restarting
          state = -1;
        } else {
          Serial.printf("GAME CONTINUE\r\n");
          ResetScoreTimerState(); // Carry on
        }
    }
    // Move to next substate
    substate++;
  }

  // Read high score
  if ( state == 6 && !AudioRunning[1] ) {
    // substate for which part of score to say
    // increment substate on leaving.
    switch (substate) {
      case 0:
        Serial.printf("High Score %d\r\n", highscore);
        break;
      case 1:
        AudioPlayCurrentHighScore();
        break;
        // Start playing the score
        // 2    3        4    5      6
        // 1-9  hundred  and  30-90  0-20
      case 2:
        if (highscore >= 100) {
          Serial.printf("100s\r\n");
          AudioPlayNumber( highscore / 100); // "One" to "Nine"
        }
        break;
      case 3:
        if (highscore >= 100) {
          Serial.printf("100\r\n");
          AudioPlayNumber( 100); // "Hundred"
        }
        break;
      case 4:
        if (highscore > 100) {
          Serial.printf("and\r\n");
          AudioPlayNumber( 101); // "AND"
        }
        break;
      case 5:
        if ((highscore % 100) >= 20) {
          Serial.printf("20-90\r\n");
          AudioPlayNumber( highscore % 100 ); // "twenty" to "ninety" 
        }
        break;
      case 6:
        // 10..19
        if ( ( (highscore % 100) >= 10) && ( (highscore % 100) < 20) ) {
          Serial.printf("10-20\r\n");
          AudioPlayNumber( highscore % 100 ); // "ten" to "nineteen" 
        } else {
          // 1..9, only if not 10-19
          if ((highscore % 10) >= 1)  {
            Serial.printf("units\r\n");
            AudioPlayNumber( highscore % 10 ); // "one" to "nine" 
          }
        }
        if (highscore == 0) {
          AudioPlayNumber( 0 ); // "zero" 
        }
        break;
      default:
        // state -1 Hangs about to say stuff before restarting
        state = -1;
    }
    // Move to next substate
    substate++;
  }

  // Play commands
  if (!AudioPlay( 1) ) {
    // Stopped playing a command
  }

  // Play background
  if ( !AudioPlay( 0) ) {
    //if (state >= 1 && state <= 3) {
    //  AudioPlayBeat();
    //  // Loop beat
    //}
  }
  // Track delays inside/outside the loop
  lastmillis = thismillis;

  //Serial.printf("State: %d, Substate: %d\n\r", state, substate);

} // loop()

// ***********************************************************************************************************************
