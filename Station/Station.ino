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
// l'autre broche des composants ci dessous est relié à la masse.
#define pin_bouton 6
#define pin_anemometre 2
#define pin_pluviometre 3
#define pin_girouette A0 // relié à une restistance de tirage au 5v de 10 kilo ohms

//Definition des contantes
#define rayon  0.07       //rayon tiges anémomètre en mètres
#define tolerance 9       //écart max par rapport aux valeurs tension girouette
#define debounce_delay 50 //en ms

// Valeurs théoriques:
// {785,405,460,83,93,65,182,126,287,244,629,598,944,826,886,702}
const word tableau_valeurs_girouette[] = {785, 405, 460, 83,  93,  65, 182, 126, 287, 244, 629, 598, 944, 826, 886, 702}; // à étalonner

const String tableau_direction_vent[] = {
    "Nord",  "Nord Nord Est",    "Nord Est",   "Est Nord Est",
    "Est",   "Est Sud Est",      "Sud Est",    "Sud Sud Est",
    "Sud",   "Sud Sud Ouest",    "Sud Ouest",  "Ouest Sud Ouest",
    "Ouest", "Ouest Nord Ouest", "Nord Ouest", "Nord Nord Est"};

//declaration des composants
LiquidCrystal lcd(lcd_rs, lcd_en, lcd_d4, lcd_d5, lcd_d6, lcd_d7);
Si115X si1151; //Capteur lumière I2C

// Déclaration des variables

//Menu
byte menu = 0; //Quel écran à afficher sur le LCD

//Capteur lumière
byte ultraviolet;
unsigned long visible;

// Le bouton
boolean etat_bouton = 0;
boolean dernier_etat_bouton;
boolean dernier_etat_lu_bouton = 0;
boolean lecture_bouton;
unsigned long dernier_debounce_delay = 0;

//Anémomètre
float vitesse; //Vitesse du vent en km/h

//Girouette
byte index_tableau_direction_vent; // Index dans le tableaux des points
                                   // cardinaux
word valeur_lu_girouette;          // Tension lue girouette

//Pluviomètre
unsigned long temps_pluviometre; //temps entre deux ticks

//Variables interrupts
volatile unsigned long dernier_tick_pluviometre = 0; // millisecondes écoulées au dernier tick du pluviomètre
volatile unsigned long tableau_temps_anemometre[2] = {0, 0}; // tableau qui répertorie les millisecondes écoulées depuis chaques
           // interrupts de l'anémomètre
volatile boolean index_tableau_anemometre = 0; // index tableau pluviomètre
volatile unsigned long tableau_temps_pluviometre[2] = {0, 0}; // tableau qui répertorie les millisecondes écoulées depuis chaques
           // interrupts du pluviomètre
volatile boolean index_tableau_pluviometre = 0; // index tableau pluviomètre

//Code interrupts

void interrupt_anemometre() {
  // stocke dans le tableau les millisecondes écoulées au moment ou l'anémometre
  // effectue un quart de tour
  tableau_temps_anemometre[index_tableau_anemometre] = millis();
  index_tableau_anemometre = !index_tableau_anemometre; // Change l'index du tableau
}

void interrupt_pluviometre() {
  // stocke dans le tableau les millisecondes écoulées au moment ou y'a de l'eau
  // dans le pluviomètre, je sais pas exactement ce qui le déclanche
  tableau_temps_pluviometre[index_tableau_pluviometre] = millis();
  dernier_tick_pluviometre = tableau_temps_pluviometre[index_tableau_pluviometre];
  index_tableau_pluviometre = !index_tableau_pluviometre; // Change l'index du tableau
}

// Code de démarrage
void setup() {
  pinMode(pin_anemometre,INPUT_PULLUP);
  pinMode(pin_pluviometre,INPUT_PULLUP);
  pinMode(pin_girouette, 0); // input

  pinMode(pin_bouton, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pin_anemometre), interrupt_anemometre, CHANGE); // interruption anémomètre
  attachInterrupt(digitalPinToInterrupt(pin_pluviometre), interrupt_pluviometre, CHANGE); // interruption pluviomètre

  Wire.begin(); //démarre la communication I2C
  lcd.begin(16, 2);
  lcd.print("Capteur lumiere");
  lcd.setCursor(4, 1);
  lcd.print("pas pret");
  while (!si1151.Begin()){} //attends que le capteur de lumière soit prêt
  lcd.clear();

}

// Boucle principale
void loop() {

  // Lis la lumière et l'ultraviolet
  ultraviolet = si1151.ReadHalfWord_UV();
  visible = map(si1151.ReadHalfWord(), 0, 65535, 0, 128000);

  // Lis le bouton
  dernier_etat_bouton = etat_bouton;
  lecture_bouton = !digitalRead(pin_bouton);
  // Quand le bouton est pressé, la valeur lue est 0 d'ou la nécesité d'inverser
  // la valeur lue pour refleter la valeur du bouton

  // lis la valeur de la girouette
  valeur_lu_girouette = analogRead(pin_girouette);

  //Mesure la vitesse du vent
  if ((tableau_temps_anemometre[index_tableau_anemometre] - tableau_temps_anemometre[index_tableau_anemometre + 1]) < 1000) {
    vitesse = (rayon * (2 * M_PI *(1 /(tableau_temps_anemometre[index_tableau_anemometre] -tableau_temps_anemometre[index_tableau_anemometre + 1]) /1000000))) * 3.6;
  }
  else {
    // si en une seconde, l'anémomètre n'a pas fait un quart de tour, on
    // considère qu'il n'y a pas de vent
    vitesse = 0;
  }

  //mesure le temps pour la pluie
  if ((dernier_tick_pluviometre + 60000) < millis()) { // timeout après une minute sans retour du pluviomètre
    temps_pluviometre = tableau_temps_pluviometre[index_tableau_pluviometre] - tableau_temps_pluviometre[index_tableau_pluviometre + 1];
  }
  else {
    temps_pluviometre = 0;
  }

  // Calcul l'index du tableau des points cardinaux par rapport à la direction
  // du vent
  for (byte a = 0; a != 16; a++) {
    if ((valeur_lu_girouette >= tableau_valeurs_girouette[a] - tolerance) &&
        (valeur_lu_girouette < tableau_valeurs_girouette[a] + tolerance)) {
      index_tableau_direction_vent = a;
      break;
    }
  }

  // debounce bouton
  if (etat_bouton != dernier_etat_lu_bouton) {
    dernier_debounce_delay = millis();
  }
  if ((millis() - dernier_debounce_delay) > debounce_delay) {
    etat_bouton = lecture_bouton;
  }
  dernier_etat_lu_bouton = lecture_bouton;

  //Regarde si on doit changer d'écran
  if (etat_bouton && !dernier_etat_bouton) {
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
      lcd.print(tableau_direction_vent[index_tableau_direction_vent]);
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

  // Maquettes écran

  //Vent
  //0123456789ABCDEF
  //Vent:Pas de vent
  //    Vent: 10.42 km/h
  //    Vent: 5.89  km/h
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
  //       120 lux
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

  // Capteur lumière pas prêt
  // 0123456789ABCDEF
  // Capteur lumiere
  //     pas pret
  // 0123456789ABCDEF
}