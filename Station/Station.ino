// A faire

//mettre à jour certains commentaires (genre changer le quart de tour de l'anémomètre en demi-tour)
//Détailler le calcul de la pluie
//Détailler le calcul de la lumière
//aérer le code ?
//remplacer tous les "index" par "indice" pour françiser un peu la chose pour le rapport
//de même pour debounce (anti rebond)

//--------------------------------------------------------------

/* Station météo (qui n'en est pas vraiment une)
 *
 *
 * Écran LCD 16*2
 * (Potentiomètre)
 * (Résistance 10 kilo ohms)
 * Bouton poussoir
 * Anémomètre
 * Girouette
 * Pluviomètre
 * Capteur lumière
 *
 * Reference capteur lumière groove si1151
 * https://wiki.seeedstudio.com/Grove-Sunlight_Sensor/
 *
 * Reference anémomètre, girouette, pluviomètre : SEN-15901
 * Fiche technique :
 * https://cdn.sparkfun.com/assets/d/1/e/0/6/DS-15901-Weather_Meter.pdf
 */

/* Brochage LCD par rapport à l'Arduino
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
5V (Avec résistance 220 ohms pour le retro éclairage)
GND

Broche 16 ↑
*/


//Bibliothèques
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Si115X.h>

//Définition nom des broches
const byte lcd_rs = 12;
const byte lcd_en = 11;
const byte lcd_d4 = 10;
const byte lcd_d5 = 9;
const byte lcd_d6 = 8;
const byte lcd_d7 = 7;
//La deuxième broche de ces composants est relié à la masse.
const byte pin_bouton = 6;
const byte pin_anemometre = 2;
const byte pin_pluviometre = 3;
const byte pin_girouette = A0;  //Relié à une resistance de tirage vers 5v de 10 kilos ohms

//Définition des constantes
const float rayon = 7;           //rayon tiges anémomètre en centimètres
const byte tolerance = 9;       //écart max par rapport aux valeurs tension girouette
const byte debounce_delay = 50; //en ms
const float pre_calcul_vitesse_vent = (rayon/100) * 2 * M_PI * 3.6;
//rayon est donné en centimètres, on le divise par 100 pour l'obtenir en mètres
//La multiplication par 3.6 permet de convertir la valeur de m/s à km/h.

//rayon est donné en centimètres, on le divise par 100 pour l'obtenir en mètres
//La multiplication par 3.6 permet de convertir la valeur de m/s à km/h.


//Définition de toutes les valeurs possibles lors de la lecture de la valeur de la girouette
/*
 * La girouette donne la direction du vent grâce à la valeur de la résistance entre ses deux broches de sortie.
 * Pour connaitre cette direction, il faut transformer la valeur de la résistance en une valeur de tension afin
 * de pouvoir la lire avec l'Arduino. Pour ce faire, un pont diviseur de tension doit être utilisé.
 *
 * L'une des broches de la girouette est reliée à la masse, tandis que l'autre est relié à une résistance
 * de 10 kilos ohms de tirage au 5v, tout cela relié à la broche d'entrée analogique Arduino.
 *
 * Grace aux valeurs fournies par la fiche technique, il est possible de calculer pour chaque angle la tension
 * entre la masse et la résistance grace au calcul suivant :
 *
 * tension = (Umax * résistance girouette) / (résistance girouette + résistance de tirage)
 *
 * Avec :
 * Umax = 5v
 * résistance girouette = résistance en fonction de l'angle sur la fiche technique
 * résistance de tirage = 10000 (10 kilos ohms)
 *
 * Une fois les valeurs des tensions obtenues, il suffit de faire un produit en croix pour obtenir toutes les
 * valeurs théoriques lisible par l'Arduino :
 *
 * valeur = ( tension * 1023) / 5
 *
 *(voir tableau Excel pour toutes les valeurs)
 *
 *(elles sont pratiquement toutes fausses, il faut tout étalonner à la main)
 */












//Valeurs théoriques : {785,405,460,83,93,65,184,126,287,244,629,598,944,826,886,702}
const word tableau_valeurs_girouette[16][2] = {{787,36}, //N   751 823
                                               {405,15}, //NNE 420 460
                                               {470,15}, //NE  460 500
                                               {88,7},  //ENE 90  96
                                               {95,5},  //E   97  103
                                               {67,10}, //ESE 61  89
                                               {190,25}, //SE  183 249
                                               {130,25}, //SSE 125 175
                                               {290,20}, //S   290 340
                                               {249,15}, //SSO 241 291
                                               {638,15}, //SO  622 654
                                               {606,15}, //OSO 590 622
                                               {945,15}, //O   916 974
                                               {830,15}, //ONO 808 852
                                               {888,15}, //NO  859 917
                                               {710,15}};//NNO 674 746
//à étalonner peut être encore une fois

/*
75 est sud est
93 est nord est
100 est
150 sud sud est
216 sud est
266 sud sud ouest
315 sud
440 nord nord est
480 nord est
606 ouest sud ouest
638 sud ouest
710 nord nord ouest
787 nord
830 ouest nord ouest
888 nord ouest
945 ouest
*/


const String tableau_direction_vent[] = { //signification des valeurs
    "nord",  "nord nord est",    "nord est",   "est nord est",
    "est",   "est sud est",      "Sud est",    "sud sud est",
    "sud",   "sud sud ouest",    "sud ouest",  "ouest sud ouest",
    "ouest", "ouest nord ouest", "nord ouest", "nord nord ouest"};


//Déclaration LCD
LiquidCrystal lcd(lcd_rs, lcd_en, lcd_d4, lcd_d5, lcd_d6, lcd_d7);

//Déclaration capteur lumière/UV I2C
Si115X si1151;

//Déclaration des variables

//Menu
unsigned long temps_rafraichissement = 0;
byte menu = 0;
//Définit l'écran à afficher
//0 = Vent
//1 = Pluie
//2 = Lumière
//3 = Indice UV
//4 = étalonnage

//Variables capteur lumière/UV
byte ultraviolet;
unsigned long visible;

//Variables bouton
boolean etat_bouton = 0;                  //état du bouton après débouncage
boolean dernier_etat_bouton;              //le dernier état du bouton
boolean etat_lu_bouton;                   //état lu du bouton
boolean dernier_etat_lu_bouton = 0;       //dernier état lu du bouton
unsigned long dernier_debounce_delay = 0; //est égal au nombre de millisecondes écoulées lorsque la valeur lue du bouton change d'état

//Anémomètre
float vitesse_vent; //Vitesse du vent en km/h
unsigned long tableau_temps_anemometre[2] = {0,0};    // tableau qui répertorie les millisecondes écoulées lors de chaque interrupts de l'anémomètre
boolean index_tableau_anemometre = 0;                 // index tableau anémomètre

//Girouette
byte index_tableau_direction_vent; //Index dans le tableau de la direction du vent
word valeur_lu_girouette;          //Valeur lue de la girouette
boolean pas_trouve;

//Pluviomètre
float valeur_pluie;        //valeur de pluie
unsigned long tableau_temps_pluviometre[2] = {0, 0};  // tableau qui répertorie les millisecondes écoulées lors de chaque interrupts du pluviomètre
boolean index_tableau_pluviometre = 0;                // index tableau pluviomètre


//Variables interrupts
volatile unsigned long dernier_debounce_vent = 0;
volatile boolean tick_anemometre = 0;
volatile unsigned long dernier_debounce_pluie = 0;
volatile boolean tick_pluviometre = 0;














//Code interrupts

//Est éxecuté au moment où l'anémomètre effectue un quart de tour (d'après nos tests, oui, mais pas selon la fiche technique)
void interrupt_anemometre() {
  dernier_debounce_vent = millis();
  tick_anemometre = 1;
}

//Est éxecuté à chaque 0.2794mm de pluie.
void interrupt_pluviometre() {
  dernier_debounce_pluie = millis();
  tick_pluviometre = 1;
  //while (1){}
}

void cherche_index_tableau_vent() {
  pas_trouve =1;
  // Cherche l'index du tableau des directions du vent par rapport à la valeur de la tension lue de la girouette
  for (byte a = 0; a != 16; a++) {
    if ((valeur_lu_girouette >= tableau_valeurs_girouette[a][0] - tableau_valeurs_girouette[a][1]) && (valeur_lu_girouette < tableau_valeurs_girouette[a][0] + tableau_valeurs_girouette[a][1])) {
      index_tableau_direction_vent = a;
      pas_trouve = 0;
      break;
    }
  }
  //S'il n'y a pas de vent, la valeur sera lue de la girouette sera égale à 0 et l'index dans le tableau ne changera pas,
  //mais ne seras pas utilisé pour afficher de direction
}









// Code de démarrage
void setup() {
  //déclare les broches comme devant être tiré au 5v par l'Arduino
  pinMode(pin_anemometre,INPUT_PULLUP);
  pinMode(pin_bouton, INPUT_PULLUP);
  pinMode(pin_pluviometre,0);// entrée
  pinMode(pin_girouette, 0); // entrée

  attachInterrupt(digitalPinToInterrupt(pin_anemometre), interrupt_anemometre, FALLING);    // interruption anémomètre
  attachInterrupt(digitalPinToInterrupt(pin_pluviometre), interrupt_pluviometre, CHANGE); // interruption pluviomètre

  Wire.begin(); //démarre la communication I2C

  lcd.begin(16, 2);
  lcd.print("Capteur lumiere");
  lcd.setCursor(4, 1);
  lcd.print("pas pret");
  while (!si1151.Begin()){} //attends que le capteur de lumière soit prêt


  lcd.clear();

  valeur_lu_girouette = analogRead(pin_girouette);
  cherche_index_tableau_vent();
  if(pas_trouve && digitalRead(pin_bouton)){
    lcd.print("Check val. gir.");
    lcd.setCursor(0,1);
    lcd.print("val. lue inconnue");
    delay(2000);
    lcd.clear();
  }
  lcd.print("Vent:       km/h");

  Serial.begin(500000);
}











// Boucle principale
void loop() {

  // Lis la lumière et l'ultraviolet
  ultraviolet = si1151.ReadHalfWord_UV();
  visible = si1151.ReadHalfWord();

  // Lis l'état du bouton
  dernier_etat_bouton = etat_bouton;
  dernier_etat_lu_bouton = etat_lu_bouton;
  etat_lu_bouton = !digitalRead(pin_bouton);
  // Quand le bouton est pressé, la valeur lue est 0 d'où la nécessité d'inverser la valeur lue pour refléter la valeur du bouton

  // Lis la valeur de la girouette
  valeur_lu_girouette = analogRead(pin_girouette);



  //debounce vent
  if (dernier_debounce_vent + 10 <= millis() && tick_anemometre) {
    // stocke dans le tableau les millisecondes écoulées
    tableau_temps_anemometre[index_tableau_anemometre] = dernier_debounce_vent;
    index_tableau_anemometre = !index_tableau_anemometre; // Change l'index du tableau
    tick_anemometre = 0;
  }

  if (dernier_debounce_pluie + 50 < millis() && tick_pluviometre){
    // stocke dans le tableau les millisecondes écoulées
    tableau_temps_pluviometre[index_tableau_pluviometre] = dernier_debounce_pluie;
    index_tableau_pluviometre = !index_tableau_pluviometre; // Change l'index du tableau
    tick_pluviometre = 0;
  }

  //Mesure la vitesse du vent

  //Si l'écart entre les deux temps dans le tableau est supérieur à 1 seconde (l'anémomètre a pris plus d'une
  //seconde à faire un quart de tour), on considère qu'il n'y a pas de vent.


  if (tableau_temps_anemometre[!index_tableau_anemometre] - tableau_temps_anemometre[index_tableau_anemometre] < 1000 && tableau_temps_anemometre[1]) {
    //la deuxième condition sert à vérifier s'il y a bien deux valeurs dans le tableau depuis le début du programme et non pas qu'une seule
    vitesse_vent = pre_calcul_vitesse_vent * (1.0 / (( float(tableau_temps_anemometre[!index_tableau_anemometre] - tableau_temps_anemometre[index_tableau_anemometre]) / 500.0 ) ) );
  }
  else {
    vitesse_vent = 0;
  }
  /* Calcul vitesse du vent :
     *
     * Vitesse [en km/h] = rayon [en m] * vitesse de rotation [en rad/s] * 3.6
     *
     * Vitesse de rotation [en rad/s] = 2 * pi * N [en tours/seconde]
     *
     * Donc le calcul complet est : Vitesse = rayon * 2 * pi * N * 3.6
     *
     *
     * On sait qu'à chaque quart de tour, le temps écoulé est stocké dans le tableau.
     *
     * Si on fait la différence des deux valeurs stockées dans le tableau, on obtient
     * le temps qu'a pris l'anémomètre pour parcourir un quart de tour [en ms].
     *
     * On la multiplie par 4 pour avoir le temps pris pour faire un tour [en ms].
     * On la divise par 1000 pour avoir le temps pris pour faire un tour [en s].
     *
     * (On simplifie dans le calcul : 4/1000 = 0.004. On multiplie le temps pris pour faire
     * un quart de tour [en ms] par cette valeur pour avoir le même résultat.)
     *
     * On calcule l'inverse de cette valeur pour avoir le nombre de tours par seconde
     *
     * Au début du programme, on a pré calculé sous forme de constante "rayon * 2 * pi * 3.6"
     *
     * Il nous reste donc plus qu'à multiplier le nombre de tours/seconde par la constante
     * et on obtient la vitesse du vent en km/h.
   */

  /* Selon la fiche technique de l'anémomètre, s'il y a un contact par seconde entre les deux broches, le vent souffle à 2.4km/h
     *
     * Donc en théorie, il faut faire la difference des millisecondes écoulées entre deux fronts déscendants (falling edge),
     *
     * diviser par 1000 pour l'avoir en seconde, l'inverser pour avoir le nombre de tours/seconde
     * et multiplier par 2.4 pour avoir la vitesse de vent en km/h.
     *
     * Notre calcul ne respecte pas cette méthode, car nos tests indiquent que l'anémomètre change d'état tous les
     * quarts de tour. On se sert du temps pris pour faire un quart de tour pour calculer la vitesse du vent à la place.
     *
   */

  Serial.println(tick_pluviometre);

  //mesure le temps pour la pluie
  if ((tableau_temps_pluviometre[!index_tableau_pluviometre] + 60000) > millis()) {
    //Serial.println();
    valeur_pluie = (1.0 / float(tableau_temps_pluviometre[!index_tableau_pluviometre] - tableau_temps_pluviometre[index_tableau_pluviometre]))*3600.0 * 0.2794;
    //Serial.println(float(tableau_temps_pluviometre[!index_tableau_pluviometre] - tableau_temps_pluviometre[index_tableau_pluviometre]));
    //Serial.println(valeur_pluie);
  }
  else {
    //aucune nouvelle donnée du pluviomètre au bout d'une minute, on considère qu'il ne pleut pas
    valeur_pluie = 0;
  }








  //Debounce bouton
  if (etat_lu_bouton != dernier_etat_lu_bouton) {
    //met la variable au nombre de millisecondes écoulées lorsque la valeur lue du bouton change d'état
    dernier_debounce_delay = millis();
  }

  if ((millis() - dernier_debounce_delay) > debounce_delay) {
    //si la valeur lue du bouton n'a pas changé pendant (constante) ms, on considère que l'état du bouton
    //est stabilisé, on peut utliliser sa valeur lue pour changer d'écran ou pas.
    etat_bouton = etat_lu_bouton;
  }


  //Regarde si on doit changer d'écran
  if (etat_bouton && !dernier_etat_bouton) {
    //si le bouton vient d'être pressé,
    //on change d'écran

    temps_rafraichissement = 0;

    if (menu == 4) {
      //pour si on était dans l'écran d'étalonnage
      menu = 0;
    }
    else {
      //va à l'écran suivant et rollback à 4.
      menu = (menu + 1) % 4;
    }

    lcd.clear();
    switch (menu) {
    case 0: //vent

      lcd.print("Vent:       km/h");
      break;

    case 1: //pluie

      lcd.setCursor(5, 0);
      lcd.print("Pluie:");
      break;

    case 2: //lumière

      lcd.setCursor(4, 0);
      lcd.print("Lumiere:");
      break;

    default: //indice ultraviolet

      lcd.print("Ind. Ultraviolet");
      break;
    }
  }









  if (etat_bouton && dernier_debounce_delay + 2000 < millis() && menu == 1) {
    menu = 4;
    temps_rafraichissement = 0;
  }








  if (temps_rafraichissement <= millis()) {

    cherche_index_tableau_vent();
    temps_rafraichissement = 100 + millis();
    lcd.setCursor(0, 1);
    lcd.print("                "); // efface la deuxième ligne

    // affiche les valeurs à l'écran
    switch (menu) {
    case 0: // vent
      lcd.setCursor(6, 0);
      lcd.print("     ");
      if (vitesse_vent > 0) {
        lcd.setCursor(6, 0);
        lcd.print(vitesse_vent);
        lcd.setCursor(0, 1);
        lcd.print(tableau_direction_vent[index_tableau_direction_vent]);
      }
      else {
        lcd.setCursor(7, 0);
        lcd.print("---");
        lcd.setCursor(2, 1);
        lcd.print("Pas de vent");
      }
      break;

    case 1: // pluie

      if (valeur_pluie > 0) {
        lcd.setCursor(4, 1);
        lcd.print(valeur_pluie);
        lcd.print(" mm/m2"); // millimètres par mètre carré par heure
        lcd.setCursor(2, 1);
        lcd.print("Pas de pluie");
      }
      else {
        lcd.setCursor(2, 1);
        lcd.print("Pas de pluie");
      }
      break;

    case 2: // lumière
      lcd.setCursor(3, 1);
      lcd.print(map(visible, 0, 65535, 0, 128000));
      lcd.print(" lux");
      break;

    case 3: // indice ultraviolet
      lcd.setCursor(0, 1);
      if (ultraviolet >= 11) {
        lcd.print("    Extreme");
      }
      else if (ultraviolet < 11 && ultraviolet >= 8) {
        lcd.print("   Tres haut");
      }
      else if (ultraviolet < 8 && ultraviolet >= 6) {
        lcd.print("      Haut");
      }
      else if (ultraviolet < 6 && ultraviolet >= 3) {
        lcd.print("     Modere");
      }
      else {
        lcd.print("     Normal");
      }
      break;

    default: // étalonnage
      lcd.setCursor(0, 0);
      lcd.print("                 ");
      lcd.setCursor(0, 0);
      lcd.print(valeur_lu_girouette);
      if (pas_trouve) {
        lcd.setCursor(5, 0);
        lcd.print("------------");
        lcd.setCursor(0, 1);
        lcd.print("Pas trouve");
      }
      else {
        lcd.setCursor(7, 0);
        lcd.print(tableau_valeurs_girouette[index_tableau_direction_vent][0]);
        lcd.setCursor(12, 0);
        lcd.print(tableau_valeurs_girouette[index_tableau_direction_vent][1]);
        lcd.setCursor(0, 1);
        lcd.print(tableau_direction_vent[index_tableau_direction_vent]);
      }
      break;
    }
  }
}

// Maquettes écran

//Vent
//0123456789ABCDEF
//Vent:  ---  km/h
//Vent: 10.42 km/h
//Vent: 5.89  km/h
//Ouest Nord Ouest
//  Pas de Vent
//0123456789ABCDEF

//étalonnage
//0123456789ABCDEF
//1023 -----> 1023
//ouest nord ouest
//0123456789ABCDEF

//Pluie
//0123456789ABCDEF
//     Pluie:
//    10 mm/m2
//  Pas de pluie
//0123456789ABCDEF

//Lumière
//0123456789ABCDEF
//    Lumiere :
//   999999 lux
//   120 lux
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