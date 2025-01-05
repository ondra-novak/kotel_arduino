void setup() {
  Serial.begin(115200);
  pinMode(A0, INPUT);
}

int v = 0;

void loop() {
  int w = analogRead(A0);
  if (w != v) {
    v = w;
    Serial.println(w);
  }
}
