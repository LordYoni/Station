// A faire

//Est ce qu'il ne faudrait pas débouncer la pluie ???
//Détailler le calcul de la pluie
//Détailler le calcul de la lumière
//changer l'affichage quand il n'y a pas de vent par:

//0123456789ABCDEF
//Vent:  ---  km/h
//  Pas de Vent
//0123456789ABCDEF

//pour réduire le clignotement de "km/h"

//--------------------------------------------------------------

/* Station météo (c'est pas vraiment une station météo, si ? Y'a pas de capteur de temperature et pour l'humidité,
 * on sais juste s'il pleut ou pas.
 *
 * Ecran LCD 16*2
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
 * Fiche technique:
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
const byte pin_girouette = A0;  //Relié à une restistance de tirage vers 5v de 10 kilo ohms

//Définition des contantes
const byte rayon = 6;           //rayon tiges anémomètre en centimètres
const byte tolerance = 9;       //écart max par rapport aux valeurs tension girouette
const byte debounce_delay = 50; //en ms
//précalcule "rayon * 2 * pi * km/h" pour le calcul de la vitesse du vent
const float pre_calcul_vitesse_vent = (rayon/100) * 2 * M_PI * 3.6;
//rayon est donné en centimètres, on le divise par 100 pour l'obtenir en mètres
//La multiplication par 3.6 permet de convertir la valeur de m/s à km/h.


//Définition de toutes les valeures possibles lors de la lecture de la valeur de la girouette
/*
 * La girouette donne la direction du vent grace à la valeur de la résistance entre ses deux broches de sortie.
 * Pour connaitre cette direction, il faut transformer la valeur de la résistance en une valeur de tension afin
 * de pouvoir la lire avec l'Arduino. Pour ce faire, un pont diviseur de tension doit être utilisé.
 *
 * L'une des broche de la girouette est reliée à la masse, tandis que l'autre est relié à une résistance
 * de 10 kilo ohms de tirage au 5v, tout cela relié à la broche d'entrée analogique Arduino.
 *
 * Grace aux valeurs fournies par la fiche technique, il est possible de calculer pour chaque angle la tension
 * entre la masse et la résistance grace au calcul suivant:
 *
 * tension = ( Umax * résistance girouette ) / ( résistance girouette + résistance de tirage )
 *
 * Avec:
 * Umax = 5v
 * résistance girouette = résistance en fonction de l'angle sur la fiche technique
 * résistance de tirage = 10000 (10 kilo ohms)
 *
 * Une fois les valeurs des tensions obtenues, il suffit de faire un produit en croix pour obtenir toutes les
 * valeures théoriques lisible par l'Arduino:
 *
 * valeur = ( tension * 1023 ) / 5
 *
 *(voir tableau Excel pour toutes les valeurs)
 */

//Valeurs théoriques: {785,405,460,83,93,65,184,126,287,244,629,598,944,826,886,702}
const word tableau_valeurs_girouette[] = {785, 405, 460, 83,  93,  65, 184, 126, 287, 244, 629, 598, 944, 826, 886, 702}; //à étalonner

const String tableau_direction_vent[] = { //signification des valeurs
        "Nord",  "Nord Nord Est",    "Nord Est",   "Est Nord Est",
        "Est",   "Est Sud Est",      "Sud Est",    "Sud Sud Est",
        "Sud",   "Sud Sud Ouest",    "Sud Ouest",  "Ouest Sud Ouest",
        "Ouest", "Ouest Nord Ouest", "Nord Ouest", "Nord Nord Est"};

//Déclaration LCD
LiquidCrystal lcd(lcd_rs, lcd_en, lcd_d4, lcd_d5, lcd_d6, lcd_d7);

//Déclaration capteur lumière/UV I2C
Si115X si1151;

//Déclaration des variables

//Menu
byte menu = 0;
//Définit l'écran à afficher
//0 = Vent
//1 = Pluie
//2 = Lumière
//3 = Indice UV

//Variables capteur lumière/UV
byte ultraviolet;
unsigned long visible;

//Variables bouton
boolean etat_bouton = 0;                  //état du bouton après débouncage
boolean dernier_etat_bouton;              //le dernier état du bouton
boolean etat_lu_bouton;                   //état du bouton lu
boolean dernier_etat_lu_bouton = 0;       //dernier etat du bouton lu
unsigned long dernier_debounce_delay = 0; //est égal au nombre de millisecondes écoulées lorsque la valeur du bouton lue change d'état

//Anémomètre
float vitesse; //Vitesse du vent en km/h

//Girouette
byte index_tableau_direction_vent; //Index dans le tableaux de la direction du vent
word valeur_lu_girouette;          //Valeur lue de la girouette

//Pluviomètre
unsigned long temps_pluviometre; //temps entre deux ticks

//Variables interrupts
volatile unsigned long tableau_temps_anemometre[2] = {1000,0}; // tableau qui répertorie les millisecondes écoulées lors de chaques interrupts de l'anémomètre
volatile boolean index_tableau_anemometre = 0;                 // index tableau anémomètre
volatile unsigned long tableau_temps_pluviometre[2] = {0, 0};  // tableau qui répertorie les millisecondes écoulées lors de chaques interrupts du pluviomètre
volatile boolean index_tableau_pluviometre = 0;                // index tableau pluviomètre

//Code interrupts

//Est éxecuté au moment ou l'anémometre effectue un quart de tour (d'après nos tests, oui, mais pas selon la fiche technique)
void interrupt_anemometre() {
    // stocke dans le tableau les millisecondes écoulées
    tableau_temps_anemometre[index_tableau_anemometre] = millis();
    index_tableau_anemometre = !index_tableau_anemometre; // Change l'index du tableau
}

//Est éxecuté à chaque 0.2794mm de pluie.
void interrupt_pluviometre() {
    // stocke dans le tableau les millisecondes écoulées
    tableau_temps_pluviometre[index_tableau_pluviometre] = millis();
    index_tableau_pluviometre = !index_tableau_pluviometre; // Change l'index du tableau
}

// Code de démarrage
void setup() {
    //délclare les broches comme devant être tiré au 5v par l'Arduino
    pinMode(pin_anemometre,INPUT_PULLUP);
    pinMode(pin_bouton, INPUT_PULLUP);
    pinMode(pin_pluviometre,0);// input
    pinMode(pin_girouette, 0); // input

    attachInterrupt(digitalPinToInterrupt(pin_anemometre), interrupt_anemometre, CHANGE);    // interruption anémomètre
    attachInterrupt(digitalPinToInterrupt(pin_pluviometre), interrupt_pluviometre, FALLING); // interruption pluviomètre

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
    visible = si1151.ReadHalfWord();

    // Lis l'état du bouton
    dernier_etat_bouton = etat_bouton;
    etat_lu_bouton = !digitalRead(pin_bouton);
    // Quand le bouton est pressé, la valeur lue est 0 d'ou la nécesité d'inverser la valeur lue pour refleter la valeur du bouton

    // Lis la valeur de la girouette
    valeur_lu_girouette = analogRead(pin_girouette);

    //Mesure la vitesse du vent

    //Si l'écart entre les deux temps dans le tableau est superieur à 1 seconde (l'anémomètre à pris plus d'une
    //seconde a faire un quart de tour), on considère qu'il n'y a pas de vent.
    if ((tableau_temps_anemometre[index_tableau_anemometre] - tableau_temps_anemometre[index_tableau_anemometre + 1]) < 1000) {

        vitesse = pre_calcul_vitesse_vent * (1 / ((tableau_temps_anemometre[index_tableau_anemometre] - tableau_temps_anemometre[index_tableau_anemometre + 1]) * 0.004));

        /* Calcul vitesse du vent:
         *
         * Vitesse [en km/h] = rayon [en m] * vitesse de rotation [en rad/s] * 3.6
         *
         * Vitesse de rotation [en rad/s] = 2 * pi * N [en tours/seconde]
         *
         * Donc le calcul complet est: Vitesse = rayon * 2 * pi * N * 3.6
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
         * (On simplifie dans le calcul: 4/1000 = 0.004. On multiplie le temps pris pour faire
         * un quart de tour [en ms] par cette valeur pour avoir le même résultat.)
         *
         * On calcule l'inverse de cette valeur pour avoir le nombre de tours par seconde
         *
         * Au début du programme, on a précalculé sous forme de constante "rayon * 2 * pi * 3.6"
         *
         * Il nous reste donc plus qu'à multiplier le nombre de tours/seconde par la constante
         * et on obtiens la vitesse du vent en km/h.
         */

        /* Selon la fiche de l'anémomètre, s'il y a un contact par seconde entre les deux broches, le vent soufle à 2.4km/h
         *
         * Donc en théorie, il faut faire la difference des millisecondes écoulées entre deux front déscendants (falling edge),
         *
         * diviser par 1000 pour l'avoir en seconde, l'inverser pour avoir le nombre de tours/seconde
         * et multiplier par 2.4 pour avoir la vitesse de vent en km/h.
         *
         * Notre calcul ne respecte pas cette méthode car nos tests indiquent que l'anémomètre change d'état tout les
         * quarts de tour. On se sert du temps pris pour faire un quart de tour pour calculer la vitesse du vent à la palce.
         *
         */

    }
    else {
        //pas de vent
        vitesse = 0;
    }

    //mesure le temps pour la pluie
    if ((tableau_temps_pluviometre[index_tableau_pluviometre + 1 ] + 60000) < millis()) {

        temps_pluviometre = (3600000/(tableau_temps_pluviometre[index_tableau_pluviometre] - tableau_temps_pluviometre[index_tableau_pluviometre + 1]))*0.2794;
    }
    else {
        //aucune nouvelle donnée du pluviomètre au bout d'une minute, on considère qu'il ne pleut pas
        temps_pluviometre = 0;
    }

    // Cherche l'index du tableau des direction du vent par rapport à la valeur de la tension lue de la girouette
    for (byte a = 0; a != 16; a++) {
        if ((valeur_lu_girouette >= tableau_valeurs_girouette[a] - tolerance) && (valeur_lu_girouette < tableau_valeurs_girouette[a] + tolerance)) {
            index_tableau_direction_vent = a;
            break;
        }
    }
    //S'il n'y a pas de vent, la valeur sera lue de la girouette sera égale à 0 et l'index dans le tableau ne changera pas
    //mais ne seras pas utilisé pour afficher de direction

    //Debounce bouton
    if (etat_lu_bouton != dernier_etat_lu_bouton) {
        //met la variable au nombre de millisecondes écoulées lorsque la valeur lue du bouton change d'état
        dernier_debounce_delay = millis();
    }

    if ((millis() - dernier_debounce_delay) > debounce_delay) {
        //si la valeur lu du bouton n'a pas changé pendant (constante) ms, on considère que l'état du bouton
        //est stabilisé, on peut utliliser sa valeur lu pour changer d'écran ou pas.
        etat_bouton = etat_lu_bouton;
    }
    dernier_etat_lu_bouton = etat_bouton;

    //Regarde si on doit changer d'écran
    if (etat_bouton && !dernier_etat_bouton) {
        //si le bouton viens d'être pressé
        //on change d'écran

        //va à l'écran suivant et rollback à 4
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
    lcd.print("                "); //efface la deuxième ligne

    //affiche les valeurs à l'écran
    switch (menu){
        case 0: //vent
            lcd.setCursor(5,0);
            lcd.print("           ");
            if(!vitesse){
                lcd.setCursor(6,0);
                lcd.print(vitesse);
                lcd.setCursor(12,0);
                lcd.print("km/h");
                lcd.setCursor(0,1);
                lcd.print(tableau_direction_vent[index_tableau_direction_vent]);
            }
            else{
                lcd.setCursor(5,0);
                lcd.print("Pas de vent");
            }
            break;

        case 1: //pluie

            if(!temps_pluviometre){
                lcd.setCursor(2,1);
                lcd.print("Pas de pluie");
            }
            else{
                lcd.setCursor(4,1);
                lcd.print(temps_pluviometre);
                lcd.print (" mm/m2"); //millimètres par mètre carré par heure
            }
            break;

        case 2: //lumière
            lcd.setCursor(3,1);
            lcd.print(map(visible, 0, 65535, 0, 128000));
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
    //Vent: 10.42 km/h
    //Vent: 5.89  km/h
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
}
