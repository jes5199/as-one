// Teensy 3.0 -- Pin 0 = RX, Pin 1 = TX
#define HWSERIAL Serial1

// Sampling related constants
const int SAMPLE_RATE_HZ = 60; // match 60fps on processing loop
const int SAMPLE_DELAY_MS = 1000 / SAMPLE_RATE_HZ;
const int FIFO_SIZE = SAMPLE_RATE_HZ / 2; // half second buffer
const int LATEST_SIZE = 2;

// Timing related constants
const int MAX_HEARTRATE_BPM = 150;
const int MIN_WINDOW_MS = 60000 / MAX_HEARTRATE_BPM;
const int MIN_HEARTRATE_BPM = 40;
const int MAX_WINDOW_MS = 60000 / MIN_HEARTRATE_BPM;

// Magic number: a large enough difference from average to trigger
// beat detection. Empirically gathered.
const int DIFFERENCE_THRESHOLD = 8;

// known-bad identifier
const int SENTINEL = 9999;

// data
struct BeatInterval {
  int startTime;
  int endTime;
};

// globals
int latest[] = { SENTINEL, SENTINEL };
int fifo[FIFO_SIZE];
int lastBeatMs = 0;
float bpm;
BeatInterval intervalBuffer[3];

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(38400);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // set the data rate for the olimex uart
  HWSERIAL.begin(115200);

  // fill interval buffer with placeholders
  struct BeatInterval placeholder;
  placeholder.startTime = SENTINEL;
  placeholder.endTime = SENTINEL;
  intervalBuffer[0] = placeholder;
  intervalBuffer[1] = placeholder;
  intervalBuffer[2] = placeholder;

  // debounce
  delay(1);
}

void enqueue(int value, int buff[], int buffSize) {
  for (int i = 1; i < buffSize; i++) {
    buff[i] = buff[i - 1];
  }
  if ((value > 250) || (value < 5)) {
    // user is disconnected or data is otherwise faulty
    buff[0] = SENTINEL;
  } else {
    buff[0] = value;
  }
}

int bufferAverage(int buff[], int buffSize) {
  int sum = 0;
  for (int i = 0; i < buffSize; i++) {
    if (buff[i] == SENTINEL) {
      return -1;
    }
    sum += buff[i];
  }
  return sum / buffSize;
}

bool detectBeat() {
  int fifoAvg = bufferAverage(fifo, FIFO_SIZE);
  int latestAvg = bufferAverage(latest, LATEST_SIZE);
  if ((latestAvg - fifoAvg) > DIFFERENCE_THRESHOLD) {
    return true;
  }
  return false;
}

// returns true if we are getting good data off the board
bool bufferAndAverage(int val) {
  enqueue(val, latest, LATEST_SIZE);
  enqueue(val, fifo, FIFO_SIZE);
  for (int i = 0; i < FIFO_SIZE; i++) {
    if (fifo[i] == SENTINEL) {
      return false;
    }
  }
  return true;
}

void loop() {
  // Get value from mod-ekg
  if (HWSERIAL.available()) {
    int bytecount = 0;
    int val = -1;
    while (HWSERIAL.available()) {
      val = HWSERIAL.read();
      bytecount++;
    }
    HWSERIAL.clear();

    // check for a detected beat
    int currentTime = millis();
    if (bufferAndAverage(val) && detectBeat()) {
      int interval = currentTime - lastBeatMs;
      lastBeatMs = currentTime;
      if (interval > MIN_WINDOW_MS && interval < MAX_WINDOW_MS) {

      }
    }

  }
  delay(SAMPLE_DELAY_MS);
}