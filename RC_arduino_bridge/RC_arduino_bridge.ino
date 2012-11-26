#define START_CHAR '^'
#define END_CHAR '$'
#define SIZE 8

char serialMessage[SIZE];
unsigned int readChar;
unsigned int count;
boolean readingSerial;
//--------------------

//define your pins
int forward = 8;

void setup() {
  Serial.begin(9600);
  pinMode(forward, OUTPUT); 
  digitalWrite(forward, LOW);
}

void loop() {
  if (Serial.available() > 0 &&  !readingSerial) {
    if (Serial.read() == START_CHAR) {
      serialRead();
    }
  }
}

void serialRead() {
  readingSerial = true;
  count = 0;
  
  iniReading:
  if (Serial.available() > 0) {
    readChar = Serial.read();
    if (readChar == END_CHAR || count == SIZE) {
      goto endReading;
    } else {
      serialMessage[count++] = readChar;
      goto iniReading;
    }
  }
  goto iniReading;
  
  endReading:
  readingSerial = false;
  serialMessage[count] = '\0';
  setRelay(serialMessage);
}

void setRelay(char* value)
{
  int a = atoi(value); 
  //Serial.println(value);
  //Serial.println(a);
  if (a == 1) {
    digitalWrite(forward, HIGH);  
  } else   if (a == 2) {
    digitalWrite(forward, LOW);  
  }
}
