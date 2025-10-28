const int PIN_VRX = A1;
const int PIN_VRY = A2;
const int PIN_SW  = 2;

const int ADC_MID = 3050; //mid assumption

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  pinMode(PIN_SW, INPUT_PULLUP);

  Serial.println("Columns: xRaw  yRaw  xC  yC  pressed");
}

void loop() {
  int xRaw = analogRead(PIN_VRX);
  int yRaw = analogRead(PIN_VRY);

  int xCenter = xRaw - ADC_MID; //invert the signal so its more intuitive
  int yCenter = ADC_MID - yRaw;

  bool pressed = (digitalRead(PIN_SW) == LOW);

  Serial.print(xRaw);
  Serial.print("\t");
  Serial.print(yRaw);
  Serial.print("\t");
  Serial.print(xCenter);
  Serial.print("\t");
  Serial.print(yCenter);
  Serial.print("\t");

  if (pressed) {
    Serial.println("PRESSED");
  } else {
    Serial.println("released");
  }

  delay(20);
}
