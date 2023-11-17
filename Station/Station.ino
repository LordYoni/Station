/* brochage LCD par rapport à l'Arduino
Broche 1 ↓

GND
    5v
    Potentiomètre (contraste)
        12
    GND
    11
    NC
    NC
    NC
    NC
    10
    9
    8
    7
    5V (avec résistance 220 ohms pour le retro éclairage)
        GND

    Broche 16 ↑
        */


#include <LiquidCrystal.h>
#include <Si115X.h>


//Definition nom des broches
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


//Definition des contantes
#define rayon  0.07       //rayon tiges anémomètre en mètres
#define tolerance 9       //écart max par rapport aux valeurs tension girouette
#define debounce_delay 50 //en ms


    //Valeurs théoriques: {785,405,460,83,93,65,182,126,287,244,629,598,944,826,886,702}
    const word tension_girouette[] = {785,405,460,83,93,65,182,126,287,244,629,598,944,826,886,702}; //à étalonner

const String points_cardinaux[] = {"Nord","Nord Nord Est","Nord Est","Est Nord Est","Est","Est Sud Est","Sud Est",
                                   "Sud Sud Est","Sud","Sud Sud Ouest","Sud Ouest","Ouest Sud Ouest","Ouest",
                                   "Ouest Nord Ouest","Nord Ouest","Nord Nord Est"};


//declaration des composants
LiquidCrystal lcd(lcd_rs, lcd_en, lcd_d4, lcd_d5, lcd_d6, lcd_d7);
Si115X si1151; //Capteur lumière I2C


//Variables

//Menu
byte menu = 0; //Quel écran à afficher sur le LCD

//Capteur lumière
word ultraviolet;
word visible;

//Le boutton
boolean etat_boutton = 0;
boolean dernier_etat_boutton;
boolean dernier_etat_lu_boutton = 0;
boolean lecture_boutton;
unsigned long dernier_debounce_delay = 0;

//Anémomètre
float vitesse; //Vitesse du vent en km/h

//Girouette
byte direction_vent; //Index dans le tableaux des points cardinaux
word valeur_tension; //Tension lue girouette

//Pluviomètre
unsigned long temps_pluviometre; //temps entre deux ticks

//Variables interrupts
volatile unsigned long dernier_tick_pluviometre = 0;           //millisecondes du dernier tick
volatile unsigned long temps_anemometre_interrupt[2] = {0,0};
volatile boolean interrupt_anemometre = 0;                     //index tableau pluviomètre
volatile unsigned long temps_pluviometre_interrupt[2] = {0,0};
volatile boolean interrupt_pluviometre = 0;                    //index tableau pluviomètre


//Code interrupts

void fonction_interrupt_anemometre(){
  temps_anemometre_interrupt[interrupt_anemometre] = millis();
  interrupt_anemometre = !interrupt_anemometre; //change l'index
}


void fonction_interrupt_pluviometre(){
  temps_pluviometre_interrupt[interrupt_pluviometre] = millis();
  dernier_tick_pluviometre = temps_pluviometre_interrupt[interrupt_pluviometre];
  interrupt_pluviometre = !interrupt_pluviometre; //change l'index
}

//code principal

void setup() {
  pinMode(pin_anemometre,INPUT_PULLUP);
  pinMode(pin_pluviometre,INPUT_PULLUP);
  pinMode(pin_girouette,0);

  pinMode(pin_boutton,INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pin_anemometre),fonction_interrupt_anemometre,CHANGE);   //interruption anémomètre
  attachInterrupt(digitalPinToInterrupt(pin_pluviometre),fonction_interrupt_pluviometre,CHANGE); //interruption pluviomètre

  Wire.begin(); //démarre la communication I2C
  lcd.begin(16, 2);
  lcd.print("Capteur lumiere");
  lcd.setCursor(4, 1);
  lcd.print("pas pret");
  while (!si1151.Begin()){} //attends que le capteur de lumière soit prêt
  lcd.clear();

}

void loop() {

  //Lis le boutton
  dernier_etat_boutton = etat_boutton;
  lecture_boutton = !digitalRead(pin_boutton);

  if(etat_boutton != dernier_etat_lu_boutton){
    dernier_debounce_delay = millis();
  }

  if((millis() - dernier_debounce_delay) > debounce_delay){
    etat_boutton = lecture_boutton;
  }

  dernier_etat_lu_boutton = lecture_boutton;


  //Lis la lumière et l'ultraviolet
  ultraviolet = si1151.ReadHalfWord_UV();
  visible = si1151.ReadHalfWord();


  //Calcul l'index du tableau des points cardinaux par rapport à la direction du vent
  valeur_tension = analogRead(pin_girouette);
  for (byte a = 0; a != 16; a++) {
    if ((valeur_tension >= tension_girouette[a] - tolerance) && (valeur_tension < tension_girouette[a] + tolerance)) {
      direction_vent = a;
      break;
    }
  }


  //Mesure la vitesse du vent

  if ((temps_anemometre_interrupt[interrupt_anemometre]-temps_anemometre_interrupt[interrupt_anemometre+1])<1000) {
    vitesse = (rayon * (2 * M_PI * (1 / (temps_anemometre_interrupt[interrupt_anemometre] - temps_anemometre_interrupt[interrupt_anemometre + 1]) / 1000000))) * 3.6;
  }
  else {
    vitesse = 0;
  }

  //mesure le temps pour la pluie
  if((dernier_tick_pluviometre+60000) > millis()){ //timeout après une minute sans retour du pluviomètre
    temps_pluviometre = 0;
  }
  else {
    temps_pluviometre = temps_pluviometre_interrupt[interrupt_pluviometre] - temps_pluviometre_interrupt[interrupt_pluviometre + 1];
  }


  //Regarde si on doit changer d'écran
  if(etat_boutton != dernier_etat_boutton){
    menu = (menu++)%4;

    lcd.clear();
    switch (menu){

    case 0: //vent

      lcd.print("Vent:");
      break;

    case 1: //pluie

      lcd.setCursor(5,0);
      lcd.print("Pluie:");
      break;

    case 2: //lumière

      lcd.setCursor(4,0);
      lcd.print("Lumiere:");
      break;

    default: //indice ultraviolet

      lcd.print("Ind. Ultraviolet");
      break;
    }
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

    if(!temps_pluviometre){
      lcd.setCursor(2,1);
      lcd.print("Pas de pluie");
    }
    else{
      lcd.setCursor(4,1);
      lcd.print((3600000/temps_pluviometre)*0.2794);
      lcd.print (" mm/m2");
    }
    break;

  case 2: //lumière
    lcd.setCursor(3,1);
    lcd.print(visible);
    lcd.print(" lux");
    break;

  default: //indice ultraviolet
    lcd.setCursor(0,1);
    if (ultraviolet >= 11) {
      lcd.print("    Extreme");
    }
    else if(ultraviolet< 11 && ultraviolet >= 8) {
      lcd.print("   Tres haut");
    }
    else if(ultraviolet  < 8 && ultraviolet  >= 6) {
      lcd.print("      Haut");
    }
    else if(ultraviolet  < 6 && ultraviolet >= 3) {
      lcd.print("     Modere");
    }
    else {
      lcd.print("     Normal");
    }
    break;
  }

  //maquette écran

  //Vent
  //0123456789ABCDEF
  //Vent:Pas de vent
  //Vent: 99.99 km/h
  //Ouest Nord Ouest
  //0123456789ABCDEF

  //Pluie
  //0123456789ABCDEF
  //     Pluie:
  //    10 mm/m2
  //  Pas de pluie
  //0123456789ABCDEF

  //Lumière
  //0123456789ABCDEF
  //    Lumiere:
  //   999999 lux
  //0123456789ABCDEF

  //Ultraviolet
  //0123456789ABCDEF
  //Ind. Ultraviolet
  //    Extreme
  //   Tres haut
  //      Haut
  //     Modere
  //     Normal
  //0123456789ABCDEF

}