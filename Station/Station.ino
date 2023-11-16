#include <LiquidCrystal.h>
#include <Si115X.h>

/* pinout LCD
GND 1
5v
pot
12
GND
11
nc
nc
nc
nc
10
9
8
7
5V (resistance pour retro éclairage)
GND 16
*/

//definition des pins
#define lcd_rs 12
#define lcd_en 11
#define lcd_d4 10
#define lcd_d5 9
#define lcd_d6 8
#define lcd_d7 7
#define pin_boutton 6
#define pin_anemometre 2
#define pin_pluviometre 3
#define pin_girouette A0

//definition des contantes
#define rayon  0.07 //rayon tige anemometre
#define tolerance 9
#define debounce_delay 50 //en ms

//valeurs théoriques {785,405,460,83,93,65,182,126,287,244,629,598,944,826,886,702}
const word tension_girouette[] = {785,405,460,83,93,65,182,126,287,244,629,598,944,826,886,702}; //à étalonner

const String points_cardinaux[] = {"Nord","Nord Nord Est","Nord Est","Est Nord Est","Est","Est Sud Est","Sud Est",
                                   "Sud Sud Est","Sud","Sud Sud Ouest","Sud Ouest","Ouest Sud Ouest","Ouest",
                                   "Ouest Nord Ouest","Nord Ouest","Nord Nord Est"," "};

//declaration des capteurs
LiquidCrystal lcd(lcd_rs, lcd_en, lcd_d4, lcd_d5, lcd_d6, lcd_d7);
Si115X si1151; //I2C capteur lumière


//variables

//menu
byte menu = 0; //quel truc à afficher sur le LCD


//capteur lumière
word ultraviolet;
word visible;
//word infrarouge;

//le boutton
boolean etat_boutton = 0;
boolean dernier_etat_boutton = 0;
boolean dernier_etat_lu_boutton = 0;
boolean lecture_boutton = 0;
unsigned long dernier_debounce_delay = 0;

//anemometre
float vitesse; //vitesse du vent en km/h
//float vitesse_angulaire;

//girouette
byte direction_vent; //index dans le tableaux des points cardinaux
word valeur_tension; //tension lue de girouette

//capteur pluie
unsigned long temps_pluviometre; //temps entre deux ticks

//variables interrupts
volatile unsigned long dernier_tick_pluviometre = 0;
volatile unsigned long temps_anemometre_interrupt[2] = {0,0};
volatile boolean interrupt_anemometre = 0;
volatile unsigned long temps_pluviometre_interrupt[2] = {0,0};
volatile boolean interrupt_pluviometre = 0;


//code interrupts

void fonction_interrupt_anemometre(){
  temps_anemometre_interrupt[interrupt_anemometre] = millis();
  interrupt_anemometre = !interrupt_anemometre;
}


void fonction_interrupt_pluviometre(){
  temps_pluviometre_interrupt[interrupt_pluviometre] = millis();
  dernier_tick_pluviometre = temps_pluviometre_interrupt[interrupt_pluviometre];
  interrupt_pluviometre = !interrupt_pluviometre;
}

//code principal

void setup() {
  pinMode(pin_anemometre,INPUT_PULLUP);
  pinMode(pin_pluviometre,INPUT_PULLUP);
  pinMode(pin_girouette,0);

  pinMode(pin_boutton,INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pin_anemometre),fonction_interrupt_anemometre,CHANGE);
  attachInterrupt(digitalPinToInterrupt(pin_pluviometre),fonction_interrupt_pluviometre,CHANGE);


  Wire.begin();
  lcd.begin(16, 2);
  lcd.print("Capteur lumiere");
  lcd.setCursor(4, 1);
  lcd.print("pas pret");
  while (!si1151.Begin()){}
  lcd.clear();

}

void loop() {

  //lire le boutton
  lecture_boutton = !digitalRead(pin_boutton);
  if(etat_boutton != dernier_etat_lu_boutton){
    dernier_debounce_delay = millis();
  }
  if((millis() - dernier_debounce_delay) > debounce_delay){
    etat_boutton = lecture_boutton;
  }
  dernier_etat_lu_boutton = lecture_boutton;


  //lire la lumière et l'ultraviolet
  ultraviolet = si1151.ReadHalfWord_UV();
  visible = si1151.ReadHalfWord();
  //infrarouge = si1151.ReadHalfWord_VISIBLE();


  //calcul l'index du tableau des points cardinaux par rapport à la direction du vent
  valeur_tension = analogRead(pin_girouette);
  for (byte a = 0; a != 16; a++) {
    if ((valeur_tension >= tension_girouette[a] - tolerance) && (valeur_tension < tension_girouette[a] + tolerance)) {
      direction_vent = a;
      break;
    }
  }


  //mesure la vitesse du vent

  if ((temps_anemometre_interrupt[interrupt_anemometre]-temps_anemometre_interrupt[interrupt_anemometre+1])<1000) {
    vitesse = (rayon * (2 * M_PI * (1 / (temps_anemometre_interrupt[interrupt_anemometre] - temps_anemometre_interrupt[interrupt_anemometre + 1]) / 1000000))) * 3.6;
  }
  else {
    vitesse = 0;
    direction_vent = 16;
  }





  //regarde si on doit changer d'écran
  if(etat_boutton != dernier_etat_boutton){
    menu = (menu++)%4;

    lcd.clear();
    switch (menu){

    case 0: //vent

      lcd.print("Vent:");
      break;

    case 1: //pluie

      lcd.setCursor(5,0);
      lcd.print("Pluie;");
      break;

    case 2: //lumière

      lcd.setCursor(4,0);
      lcd.print("Lumiere:");
      break;

    default: //indice ultraviolet

      lcd.setCursor(7,0);
      lcd.print("ultraviolet;");
      break;
    }
  }

  dernier_etat_boutton = etat_boutton;

  if((dernier_tick_pluviometre+60000) > millis()){
    temps_pluviometre = 0;
  }
  else {
    temps_pluviometre = temps_pluviometre_interrupt[interrupt_pluviometre] - temps_pluviometre_interrupt[interrupt_pluviometre + 1];
  }

  lcd.setCursor(0,1);
  lcd.print("                ");

  //affiche les valeurs à l'écran
  switch (menu){
  case 0: //vent
    lcd.setCursor(5,0);
    lcd.print("           ");
    if(!vitesse){
      lcd.setCursor(5,0);
      lcd.print("Pas de vent");
    }
    else{
      lcd.setCursor(6,0);
      lcd.print(vitesse);
      lcd.setCursor(12,0);
      lcd.print("km/h");
      lcd.setCursor(0,1);
      lcd.print(points_cardinaux[direction_vent]);
    }
    break;

  case 1: //pluie
    lcd.setCursor(4,1);
    lcd.print((3600000/temps_pluviometre)*0.2794);
    lcd.print ("mm/m2");
    break;

  case 2: //lumière
    lcd.setCursor(3,1);
    lcd.print(visible);
    lcd.print(" lux");
    break;

  default: //indice ultraviolet
    lcd.setCursor(3,1);
    if (ultraviolet >= 11) {
      lcd.print("Extrême");
    }
    else if(ultraviolet< 11 && ultraviolet >= 8) {
      lcd.print("Très haut");
    }
    else if(ultraviolet  < 8 && ultraviolet  >= 6) {
      lcd.print("Haut");
    }
    else if(ultraviolet  < 6 && ultraviolet >= 3) {
      lcd.print("Modéré");
    }
    else {
      lcd.print("Normal");
    }
    break;
  }

  //maquette écran

  //maquette vent
  //0123456789ABCDEF
  //Vent:Pas de vent
  //Vent: 99.99 km/h
  //Ouest Nord Ouest
  //0123456789ABCDEF

  //maquette pluie
  //0123456789ABCDEF
  //     Pluie:
  //    10 mm/m²
  //0123456789ABCDEF

  //maquette lumière
  //0123456789ABCDEF
  //    Lumiere:
  //   999999 lux
  //0123456789ABCDEF

  //maquette ultraviolet
  //0123456789ABCDEF
  //      ultraviolet:
  //   Tres haut
  //0123456789ABCDEF

}