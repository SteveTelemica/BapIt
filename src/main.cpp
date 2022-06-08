#include <Arduino.h>
#include <WiFi.h>
#include "SPIFFS.h"

#define SOUND_ON
#ifdef SOUND_ON

#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "AudioOutputMixer.h"

// Sound Name Array
const char *sfile[] = { 
                  "/Bopit.mp3",     // 0
                  "/Spinit.mp3",    // 1
                  "/Pullit.mp3",    // 2
                  "/Twistit.mp3",   // 3
                  "/Flickit.mp3",   // 4
                  "/Endit.mp3",     // 5 - "Oh No!"
// Simple rhythm
                  "/Rythm1.mp3",    // 6
// Words stuff
                  "/Welldone.mp3",  // 7
                  "/Score.mp3",     // 8
                  "/Highscore.mp3", // 9
// Numbers
                  "/Zero.mp3",      // 10
                  "/One.mp3",       // 11
                  "/Two.mp3",       // 12
                  "/Three.mp3",     // 13
                  "/Four.mp3",      // 14
                  "/Five.mp3",      // 15
                  "/Six.mp3",       // 16
                  "/Seven.mp3",     // 17
                  "/Eight.mp3",     // 18
                  "/Nine.mp3",      // 19
                  "/Ten.mp3",       // 20
                  "/Eleven.mp3",    // 21
                  "/Twelve.mp3",    // 22
                  "/Thirteen.mp3",  // 23
                  "/Fourteen.mp3",  // 24
                  "/Fifteen.mp3",   // 25
                  "/Sixteen.mp3",   // 26
                  "/Seventeen.mp3", // 27
                  "/Eighteen.mp3",  // 28
                  "/Nineteen.mp3",  // 29
                  "/Twenty.mp3",    // 30
                  "/Thirty.mp3",    // 31
                  "/Forty.mp3",     // 32
                  "/Fifty.mp3",     // 33
                  "/Sixty.mp3",     // 34
                  "/Seventy.mp3",   // 35
                  "/Eighty.mp3",    // 36
                  "/Ninety.mp3",    // 37
// Jokes
                  "/Dosame.mp3",    // 38 "Do it the same, but better!"
                  "/Notgood.mp3",   // 39 "Not good!"
// Other
                  "/Gameover.mp3"   // 40
                };
#define SOUNDFILEMAX 7 // 41 // One more than above largest

// Sound Objects
AudioFileSourceSPIFFS *afsource[ SOUNDFILEMAX ];

// Two parallel outputs
AudioGeneratorMP3 *wav[2];
AudioOutputI2S *out;
AudioOutputMixer *mixer;
AudioOutputMixerStub *stub[2];
bool AudioRunning[2];
float gain[2];

void AudioSetup( void) {
  // Allow logging of audio errors
  audioLogger = &Serial;

  // Files
  for (int i = 0; i < SOUNDFILEMAX; i++ ) {
    afsource[i] = new AudioFileSourceSPIFFS( sfile[ i] );
  }

  // Output
  out = new AudioOutputI2S();
  mixer = new AudioOutputMixer(32, out);

  // Rhythm channel
  stub[0] = mixer->NewInput();
  gain[0] = 0.2;
  stub[0]->SetGain(gain[0]);
  wav[0] = new AudioGeneratorMP3();
  AudioRunning[0] = false;

  // Action channel
  stub[1] = mixer->NewInput();
  gain[1] = 0.2;
  stub[1]->SetGain(gain[0]);
  wav[1] = new AudioGeneratorMP3();
  AudioRunning[1] = false;
}

void AudioStart( int chan, int selection) {
  //Start output if no sound running
  if (!AudioRunning[0] && !AudioRunning[1]) {
    out->begin();   
  }
  //select file
  afsource[ selection ]->open( sfile[ selection ] );
  stub[chan]->SetGain( gain[ chan]);
  wav[chan]->begin( afsource[ selection ], stub[chan]);
  AudioRunning[chan] = true;
}

bool AudioPlay( int chan) {
  if (AudioRunning[chan]) {
    if (wav[chan]->isRunning() ) {
      if (!wav[chan]->loop() ) {
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
#endif

// Commands on mix 1, 0-4 and play 5 for others
void soundplaycommand( int action) {
  Serial.printf("Sound %d.\r\n", action );
#ifdef SOUND_ON
  if ( (action >= 0) && (action <= 4) ) {
    AudioStart( 1, action);
  } else {
    AudioStart( 1, 5);
  }
#endif
}

int switchbits() {
  // Set a bit for each input, active low
  int x = ((digitalRead(34) == 0) ?  1 : 0 ) +
          ((digitalRead(35) == 0) ?  2 : 0 ) +
          ((digitalRead(32) == 0) ?  4 : 0 ) +
          ((digitalRead(33) == 0) ?  8 : 0 ) +
          ((digitalRead(19) == 0) ? 16 : 0 );
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

unsigned long lastmillis = 0;

void setup() {
  WiFi.mode(WIFI_OFF); 
  Serial.begin(115200);

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

  for (int i = 0; i < 5; i++) {
    setLED(i);
    delay(200);
  }
  setLED(-1);

  SPIFFS.begin();

#ifdef SOUND_ON
  audioLogger = &Serial;
  AudioSetup();
#endif

  lastmillis = millis();
  randomSeed( millis());

} // setup()

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

// substate used for sound file playing
int substate = 0;

int score = 0;
int highscore = 0;

int getcycleinterval( int score) {
  if (score <= 2)
    return 2500;
  if (score <= 5)
    return 2300;
  if (score <= 10)
    return 2150;
  if (score <= 20)
    return 2050;
  if (score <= 30)
    return 2000;
  if (score <= 40)
    return 1800;
    return 1700;
}

void loop() {
  unsigned long thismillis = millis();

  // How to start
  if (state == -1) {
    Serial.printf("Press s or BapIt to Start.\r\n");
    state = 0;
    // Say bopit when preparing 
    soundplaycommand(0);
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
    int startswitch = ! (switchbits() & 1); // 0 is ON

    if (input[0] == 's' || startswitch) {
      Serial.printf("Started\r\n");
      state = 3; // Start by playing rythm and not expect action (3 = command correct)
      cyclemillis = thismillis;
      score = 0;
      setLED(-1);
      cycleinterval = getcycleinterval(score);
    }
  }

  // Next sound/request cycle
  // Either user was right or did not action
  if (state == 3 || state == 2) {
    if (thismillis - cyclemillis > cycleinterval) {
      // If right get next random 0..4
      if (state == 3) {
        nextaction = random(5);
        Serial.printf("Please do: %d\r\n", nextaction);
        soundplaycommand( nextaction);
        cycleinterval = getcycleinterval(score);
        state = 1;
        cyclemillis = thismillis;
        // lead then confuse with LEDs
        if (score <= 8) {
          setLED( nextaction);
        } else {
          setLED( random(5) );
        }
        // Flush serial
        while (Serial.available()) {
          char input[1];
          int c = Serial.readBytes( input, 1);
        }
      } else {
        // Did not respond!
        state = 4;
        substate = 0;
        Serial.printf("OUT OF TIME!\r\n");
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
      // If Correct?
      if (codeinput == nextaction) {
        // Yes, next state
        Serial.printf("OK\r\n");
        state = 3;
        score++;
      } else {
        // WRONG action!
        state = 4;
        substate = 0;
      }
    }
    // else still waiting
  }

  // Whoops!
  // substate set to 0 when entering state 4 end of round
  static bool newhigh;
  // also wait for sound chan 1 to stop
  if ( state == 4 && !AudioRunning[1] ) {
    switch (substate) {
      case 0:
        newhigh = false;
        AudioStart( 1, 5); // Endit - Oh No!

        if (score > highscore) {
          highscore = score;
          newhigh = true;
        }
        Serial.printf("WRONG! Score %d, High Score %d\r\n", score, highscore);
        if (newhigh) {
          Serial.printf("Well done!\r\n");
        }
        break;
      case 1:
        Serial.printf("Say Score\r\n");
        //prem end
        state = -1;
        //AudioStart( 1, 8); // Score
        break;
      case 2:
        // Start playing the score, no scores over 99!
        if (score <= 19) {
          AudioStart( 1, 10 + score); // zero to 19
        } else {
          AudioStart( 1, 30 + (score-20)/10) ; // 20,30,40 etc. 
        }
        break;
      case 3:
        // Next score, if > 20
        if (score > 20) {
          AudioStart( 1, (10 + score ^ 10) ); 
        }
        break;
      default:
        // Hang about to say stuff before we change state
        state = -1;
    }
    // Move to next substate
    substate++;
  }

#ifdef SOUND_ON
  // Play commands
  if (!AudioPlay( 1) ) {
    // Stopped playing a command
  }

  // Loop this for now
  if ( !AudioPlay( 0) ) {
    if (state >= 1 && state <= 3) {
      // Loop beat
      AudioStart( 0, 6);
    }
  }
#endif
  // Track delays inside/outside the loop
  lastmillis = thismillis;

  //Serial.printf("State: %d, Substate: %d\n\r", state, substate);

} // loop()
