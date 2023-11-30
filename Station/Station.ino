/*
*   ____ _____  _  _____ ___ ___  _   _     __  __ _____ _____ _____ ___
*  / ___|_   _|/ \|_   _|_ _/ _ \| \ | |   |  \/  | ____|_   _| ____/ _ \
*  \___ \ | | / _ \ | |  | | | | |  \| |   | |\/| |  _|   | | |  _|| | | |
*   ___) || |/ ___ \| |  | | |_| | |\  |   | |  | | |___  | | | |__| |_| |
*  |____/ |_/_/   \_\_| |___\___/|_| \_|   |_|  |_|_____| |_| |_____\___/
*
*/





//section Composants

/* Station météo
 *
 * Écran LCD 16*2
 * (Potentiomètre)
 * (Résistance 10 kilo ohms)
 * Bouton poussoir
 * Anémomètre
 * Girouette
 * Pluviomètre
 * Capteur lumière
 * Capteur de température
 *
 * Reference capteur lumière : groove si1151
 * Datasheet :
 * https://wiki.seeedstudio.com/Grove-Sunlight_Sensor/
 *
 * Reference anémomètre, girouette, pluviomètre : SEN-15901
 * Datasheet :
 * https://cdn.sparkfun.com/assets/d/1/e/0/6/DS-15901-Weather_Meter.pdf
 *
 * Reference capteur température : TMP36
 * Datasheet :
 * https://www.gotronic.fr/pj-883.pdf
 */





// section Bibliothèques

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Si115X.h>





// section Définition nom des broches

const byte lcd_rs = 12;
const byte lcd_en = 11;
const byte lcd_d4 = 10;
const byte lcd_d5 = 9;
const byte lcd_d6 = 8;
const byte lcd_d7 = 7;
const byte pin_bouton = 6;
const byte pin_anemometre = 2;
const byte pin_pluviometre = 3;
const byte pin_girouette = A0;     //Relié à une resistance de tirage vers 5v de 10 kilos ohms
const byte pin_tmp36 = A1;





//section Définition des constantes

const float rayon = 7;             //rayon tiges anémomètre en centimètres
const byte anti_rebond_delay = 50; //en ms
const float pre_calcul_vitesse_vent = (rayon/100) * 2 * M_PI * 3.6;
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





//section Valeurs girouette

//Valeurs théoriques : {785,405,460,83,93,65,184,126,287,244,629,598,944,826,886,702}

const word tableau_valeurs_girouette[16][2] = {{787,36}, //N
                                               {405,15}, //NNE
                                               {470,15}, //NE
                                               {83,5},   //ENE
                                               {93,5},   //E
                                               {67,10},  //ESE
                                               {190,25}, //SE
                                               {130,25}, //SSE
                                               {290,20}, //S
                                               {249,15}, //SSO
                                               {638,15}, //SO
                                               {606,15}, //OSO
                                               {945,15}, //O
                                               {830,15}, //ONO
                                               {888,15}, //NO
                                               {710,15}};//NNO

//à étalonner à chaque nouveau jour de lancement

const String tableau_direction_vent[] = { //signification des valeurs
    "nord",  "nord nord est",    "nord est",   "est nord est",
    "est",   "est sud est",      "Sud est",    "sud sud est",
    "sud",   "sud sud ouest",    "sud ouest",  "ouest sud ouest",
    "ouest", "ouest nord ouest", "nord ouest", "nord nord ouest"};





//section Déclaration LCD

LiquidCrystal lcd(lcd_rs, lcd_en, lcd_d4, lcd_d5, lcd_d6, lcd_d7);





//section Déclaration capteur lumière/UV I2C

Si115X si1151;





//section Déclaration des variables

//Menu
unsigned long temps_rafraichissement = 0;
byte menu = 0;
//Définit l'écran à afficher
//0 = Vent
//1 = Pluie
//2 = température
//3 = Lumière
//4 = Indice UV
//5 = étalonnage


//Variables capteur lumière/UV
byte ultraviolet;
unsigned long visible;


//Variables bouton
boolean etat_bouton = 0;                  //état du bouton après débouncage
boolean dernier_etat_bouton;              //le dernier état du bouton
boolean etat_lu_bouton;                   //état lu du bouton
boolean dernier_etat_lu_bouton = 0;       //dernier état lu du bouton
unsigned long dernier_anti_rebond_delay = 0; //est égal au nombre de millisecondes écoulées lorsque la valeur lue du bouton change d'état


//Anémomètre
float vitesse_vent; //Vitesse du vent en km/h
unsigned long tableau_temps_anemometre[2] = {0,0};    // tableau qui répertorie les millisecondes écoulées lors de chaque interrupts de l'anémomètre
boolean indice_tableau_anemometre = 0;                 // indice tableau anémomètre


//Girouette
byte indice_tableau_direction_vent; //Index dans le tableau de la direction du vent
word valeur_lu_girouette;          //Valeur lue de la girouette
boolean pas_trouve;


//Pluviomètre
float valeur_pluie;        //valeur de pluie
unsigned long tableau_temps_pluviometre[2] = {0, 0};  // tableau qui répertorie les millisecondes écoulées lors de chaque interrupts du pluviomètre
boolean indice_tableau_pluviometre = 0;                // indice tableau pluviomètre


//Capteur de température
int valeur_termometre;
float temperature;


//Variables interrupts
volatile unsigned long dernier_anti_rebond_vent = 0;
volatile boolean tick_anemometre = 0;
volatile unsigned long dernier_anti_rebond_pluie = 0;
volatile boolean tick_pluviometre = 0;





//section Code interrupts

//Est éxecuté au moment où l'anémomètre effectue un demi-tour (d'après nos tests, oui, mais pas selon la fiche technique)
void interrupt_anemometre() {
  dernier_anti_rebond_vent = millis();
  tick_anemometre = 1;
}


//Est éxecuté à chaque 0.2794mm de pluie.
void interrupt_pluviometre() {
  dernier_anti_rebond_pluie = millis();
  tick_pluviometre= 1 ;
}


void cherche_indice_tableau_vent() {
  pas_trouve =1;
  // Cherche l'indice du tableau des directions du vent par rapport à la valeur de la tension lue de la girouette
  for (byte a = 0; a != 16; a++) {
    if ((valeur_lu_girouette >= tableau_valeurs_girouette[a][0] - tableau_valeurs_girouette[a][1]) && (valeur_lu_girouette < tableau_valeurs_girouette[a][0] + tableau_valeurs_girouette[a][1])) {
      indice_tableau_direction_vent = a;
      pas_trouve = 0;
      break;
    }
  }
  //S'il n'y a pas de vent, la valeur sera lue de la girouette sera égale à 0 et l'indice dans le tableau ne changera pas,
  //mais ne seras pas utilisé pour afficher de direction
}





/*
*    ____          _              _              _   __                                           
*   / ___|___   __| | ___      __| | ___      __| | /_/ _ __ ___   __ _ _ __ _ __ __ _  __ _  ___ 
*  | |   / _ \ / _` |/ _ \    / _` |/ _ \    / _` |/ _ \ '_ ` _ \ / _` | '__| '__/ _` |/ _` |/ _ \
*  | |__| (_) | (_| |  __/   | (_| |  __/   | (_| |  __/ | | | | | (_| | |  | | | (_| | (_| |  __/
*   \____\___/ \__,_|\___|    \__,_|\___|    \__,_|\___|_| |_| |_|\__,_|_|  |_|  \__,_|\__, |\___|
*                                                                                    |___/      
*/
//section Code de démarrage


void setup() {
  //déclare les broches comme devant être tiré au 5v par l'Arduino
  pinMode(pin_anemometre,INPUT_PULLUP);
  pinMode(pin_bouton, INPUT_PULLUP);
  pinMode(pin_pluviometre,INPUT_PULLUP);
  pinMode(pin_girouette, 0);  // entrée
  pinMode(pin_tmp36, 0);  // entrée

  attachInterrupt(digitalPinToInterrupt(pin_anemometre), interrupt_anemometre, FALLING);    // interruption anémomètre
  attachInterrupt(digitalPinToInterrupt(pin_pluviometre), interrupt_pluviometre, FALLING); // interruption pluviomètre

  Wire.begin(); //démarre la communication I2C

  lcd.begin(16, 2);
  lcd.print("Capteur lumiere");
  lcd.setCursor(4, 1);
  lcd.print("pas pret");
  while (!si1151.Begin()){} //attends que le capteur de lumière soit prêt
  
  lcd.clear();
  valeur_termometre = analogRead(pin_tmp36);
  valeur_lu_girouette = analogRead(pin_girouette);
  cherche_indice_tableau_vent();
  if(pas_trouve && digitalRead(pin_bouton)){
    lcd.print("Check val. gir.");
    lcd.setCursor(0,1);
    lcd.print("val. lue inconnue");
    delay(2000);
    lcd.clear();
  }
  lcd.print("Vent:       km/h");
}





/*
*   ____                   _                    _            _             _      
*  | __ )  ___  _   _  ___| | ___    _ __  _ __(_)_ __   ___(_)_ __   __ _| | ___ 
*  |  _ \ / _ \| | | |/ __| |/ _ \  | '_ \| '__| | '_ \ / __| | '_ \ / _` | |/ _ \
*  | |_) | (_) | |_| | (__| |  __/  | |_) | |  | | | | | (__| | |_) | (_| | |  __/
*  |____/ \___/ \__,_|\___|_|\___|  | .__/|_|  |_|_| |_|\___|_| .__/ \__,_|_|\___|
*                                   |_|                       |_|                 
*/
//section Boucle principale


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

  
  // Lis la valeur du thermomètre
  valeur_termometre = analogRead(pin_tmp36);

  
  // section anti_rebond vent
  
  if (dernier_anti_rebond_vent + 10 <= millis() && tick_anemometre) {
    // stocke dans le tableau les millisecondes écoulées
    tableau_temps_anemometre[indice_tableau_anemometre] = dernier_anti_rebond_vent;
    indice_tableau_anemometre = !indice_tableau_anemometre; // Change l'indice du tableau
    tick_anemometre = 0;
  }

  
  // section anti_rebond pluie
  
  if (dernier_anti_rebond_pluie + 50 <= millis() && tick_pluviometre){

    // stocke dans le tableau les millisecondes écoulées
    tableau_temps_pluviometre[indice_tableau_pluviometre] = dernier_anti_rebond_pluie;
    indice_tableau_pluviometre = !indice_tableau_pluviometre; // Change l'indice du tableau
    tick_pluviometre = 0;
  }

  
  //section Mesure la vitesse du vent

  //Si l'écart entre les deux temps dans le tableau est supérieur à 1 seconde (l'anémomètre a pris plus d'une
  //seconde à faire un demi-tour), on considère qu'il n'y a pas de vent.
  
  if (tableau_temps_anemometre[!indice_tableau_anemometre] - tableau_temps_anemometre[indice_tableau_anemometre] < 1000 && tableau_temps_anemometre[1]) {
    //la deuxième condition sert à vérifier s'il y a bien deux valeurs dans le tableau depuis le début du programme et non pas qu'une seule
    vitesse_vent = pre_calcul_vitesse_vent * (1.0 / (( float(tableau_temps_anemometre[!indice_tableau_anemometre] - tableau_temps_anemometre[indice_tableau_anemometre]) / 500.0 ) ) );
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
     * On sait qu'à chaque demi-tour, le temps écoulé est stocké dans le tableau.
     *
     * Si on fait la différence des deux valeurs stockées dans le tableau, on obtient
     * le temps qu'a pris l'anémomètre pour parcourir un demi-tour [en ms].
     *
     * On la multiplie par 4 pour avoir le temps pris pour faire un tour [en ms].
     * On la divise par 1000 pour avoir le temps pris pour faire un tour [en s].
     *
     * (On simplifie dans le calcul : 4/1000 = 0.004. On multiplie le temps pris pour faire
     * un demi-tour [en ms] par cette valeur pour avoir le même résultat.)
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
     * demi-tour. On se sert du temps pris pour faire un demi-tour pour calculer la vitesse du vent à la place.
     *
   */


  //section Mesure la pluie
  
  if ((tableau_temps_pluviometre[!indice_tableau_pluviometre] + 60000) > millis()) {
    valeur_pluie = (1.0 / float(tableau_temps_pluviometre[!indice_tableau_pluviometre] - tableau_temps_pluviometre[indice_tableau_pluviometre]))*3600.0 * 0.2794;
  }
  else {
    //aucune nouvelle donnée du pluviomètre au bout d'une minute, on considère qu'il ne pleut pas
    valeur_pluie = 0;
  }
    /* Calcul vitesse du vent :
     *
     * Pluie/h [en mm/m²] = nombre d'activations du pluviomètre par heure (moyenne)  * volume pour une activation
     *
     */

    /*
     *
     * Donc pour le nombre d'activations par seconde, il faut faire la difference des millisecondes écoulées entre deux fronts déscendants (falling edge),
     * l'inverser pour avoir le nombre de d'activations/seconde, puis ont le multiplient par 3600 pour savoir pour une heure le nombre d'activations théorique
     * et multiplier par 0.2794 pour avoir le volume de pluie par heure en mm/m².
     *
     */



  
  
  //section Debounce bouton
  
  if (etat_lu_bouton != dernier_etat_lu_bouton) {
    //met la variable au nombre de millisecondes écoulées lorsque la valeur lue du bouton change d'état
    dernier_anti_rebond_delay = millis();
  }

  
  if ((millis() - dernier_anti_rebond_delay) > anti_rebond_delay) {
    //si la valeur lue du bouton n'a pas changé pendant (constante) ms, on considère que l'état du bouton
    //est stabilisé, on peut utliliser sa valeur lue pour changer d'écran ou pas.
    etat_bouton = etat_lu_bouton;
  }


  
  
  
  //section Changement d'écran pour le texte
  
  if (etat_bouton && !dernier_etat_bouton) {
    //si le bouton vient d'être pressé,
    //on change d'écran

    temps_rafraichissement = 0;

    if (menu == 5) {
      //pour si on était dans l'écran d'étalonnage
      menu = 0;
    }
    else {
      //va à l'écran suivant et rollback à 4.
      menu = (menu + 1) % 5;
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

    case 2: //température

      lcd.setCursor(2, 0);
      lcd.print("Temperature:");
      break;

    case 3: //lumière

      lcd.setCursor(4, 0);
      lcd.print("Lumiere:");
      break;

    default: //indice ultraviolet

      lcd.print("Ind. Ultraviolet");
      break;
    }
  }




  //section Étalonnage
  
  if (etat_bouton && dernier_anti_rebond_delay + 2000 < millis() && menu == 1) {
    //Si sur l'écran du vent et que l'on appuie sur le bouton 2 secondes ou plus, rentrer dans le mode étalonnage
    menu = 5;
    temps_rafraichissement = 0;
  }



  //section Changement d'écran pour les valeurs

  if (temps_rafraichissement <= millis()) {

    cherche_indice_tableau_vent();
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
        lcd.print(tableau_direction_vent[indice_tableau_direction_vent]);
      }
      else {
        lcd.setCursor(7, 0);
        lcd.print("---");
        lcd.setCursor(2, 1);
        lcd.print("Pas de vent");
      }
      break;

    case 1: //pluie

      lcd.setCursor(4, 1);
      lcd.print(valeur_pluie, 2);
      lcd.print(" mm/m2");  // millimètres par mètre carré par heure

      if (valeur_pluie == 0){
        lcd.setCursor(2, 1);
        lcd.print("Pas de pluie");
      }
      break;

    case 2: // Température
      lcd.setCursor(3, 1);
      lcd.print((valeur_termometre * 5.0 / 1024.0 - 0.5) * 100);
      lcd.print(" C ");
      break;

    case 3: // lumière
      lcd.setCursor(3, 1);
      lcd.print(visible);
      lcd.print(" lumen");
      break;

    case 4: // indice ultraviolet
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
        lcd.print(tableau_valeurs_girouette[indice_tableau_direction_vent][0]);
        lcd.setCursor(12, 0);
        lcd.print(tableau_valeurs_girouette[indice_tableau_direction_vent][1]);
        lcd.setCursor(0, 1);
        lcd.print(tableau_direction_vent[indice_tableau_direction_vent]);
      }
      break;
    }
  }
}