#include "Si115X.h"
#include <LiquidCrystal.h>

Si115X si1151;

float uv;
float vis;
float ir;
int qu;

#define rs 12
#define en 11
#define d4 5
#define d5 4
#define d6 3
#define d7 2

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup()
{
  lcd.setCursor(0, 0);
  lcd.begin(16, 2);

  pinMode(A0, 0);


  Wire.begin();
  lcd.print("pas pret");
  while (!si1151.Begin()){}
  lcd.clear();
}
void loop() {

  uv = si1151.ReadHalfWord_UV();
  ir = si1151.ReadHalfWord();
  vis = si1151.ReadHalfWord_VISIBLE();
  qu = analogRead(A0);

  lcd.setCursor(0, 0);
  lcd.print("Visible: ");
  lcd.setCursor(0, 1);
  lcd.print(vis);
  delay(2000);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("IR: ");
  lcd.setCursor(0, 1);
  lcd.print(ir);
  delay(2000);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("UV: ");
  lcd.setCursor(0, 1);
  lcd.print(uv);
  delay(2000);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Qualite de l'air");
  lcd.setCursor(0, 1);
  lcd.print(map(qu,0,1023,100,0));
  lcd.print("%");
  delay(2000);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Indice UV: ");
  lcd.setCursor(0, 1);

  if (uv >= 11) {
    lcd.print("Extrême");
  }
  else if(uv< 11 && uv >= 8) {
    lcd.print("Très haut");
  }
  else if(uv  < 8 && uv  >= 6) {
    lcd.print("Haut");
  }
  else if(uv  < 6 && uv >= 3) {
    lcd.print("Modéré");
  }
  else {
    lcd.print("Normal");
  }
  delay(2000);
  lcd.clear();
}