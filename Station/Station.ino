/*
*   ____ _____  _  _____ ___ ___  _   _     __  __ _____ _____ _____ ___
*  / ___|_   _|/ \|_   _|_ _/ _ \| \ | |   |  \/  | ____|_   _| ____/ _ \
*  \___ \ | | / _ \ | |  | | | | |  \| |   | |\/| |  _|   | | |  _|| | | |
*   ___) || |/ ___ \| |  | | |_| | |\  |   | |  | | |___  | | | |__| |_| |
*  |____/ |_/_/   \_\_| |___\___/|_| \_|   |_|  |_|_____| |_| |_____\___/
*
*/



//section Composants

/*
 * Carte Arduino UNO
 * Écran LCD 16*2
 * (potentiomètre)
 * Résistance 10 kilo ohms et Résistance 220 ohms
 * Bouton poussoir
 * Anémomètre
 * Girouette
 * Pluviomètre
 * Capteur lumière
 * Capteur de température
 * (Bientôt module Xbee)
 *
 * Reference capteur lumière : groove si1151
 * Fiche technique :
 * https://wiki.seeedstudio.com/Grove-Sunlight_Sensor/
 *
 * Reference anémomètre, girouette, pluviomètre : SEN-15901
 * Fiche technique :
 * https://cdn.sparkfun.com/assets/d/1/e/0/6/DS-15901-Weather_Meter.pdf
 *
 * Reference capteur température : TMP36
 * Fiche technique :
 * https://www.gotronic.fr/pj-883.pdf
 */





// section Bibliothèques

//#include <Arduino.h> //ne sert techniquement à rien si on n'arrive pas à faire fonctionner Arduino sur Clion
#include <LiquidCrystal.h>
#include <Si115X.h>





// section Définition nom des broches

const byte lcd_rs                     = 12;
const byte lcd_en                     = 11;
const byte lcd_d4                     = 10;
const byte lcd_d5                     = 9;
const byte lcd_d6                     = 8;
const byte lcd_d7                     = 7;
const byte broche_bouton              = 6;
const byte broche_pluviometre         = 3;
const byte broche_anemometre          = 2;
const byte broche_girouette           = A0;
const byte broche_capteur_temperature = A1;





//section Définition des constantes

//délais en ms
const byte delai_anti_rebond_bouton         = 50;
const byte delai_anti_rebond_anemometre     = 10;
const byte delai_anti_rebond_pluviometre    = 50;
const byte rafraichissement_par_secondes    = 10; //nombre de fois max où l'affichage est rafraichi
const float longueur_tige_anemometre        = 7;  //longueur des tiges de l'anémomètre en centimètres
const byte temps_mode_etalonnage            = 2;  //temps en secondes pour rentrer en mode étalonnage
const byte temps_abandon_pluviometre        = 1;  //temps timeout pluviomètre en minute(s)

const byte delai_rafraichissement           = rafraichissement_par_secondes * 10;
const unsigned int temps_abandon_pluviometre_ms = temps_abandon_pluviometre * 60000;
const unsigned int temps_mode_etalonnage_ms = temps_mode_etalonnage * 1000;
const float pre_calcul_vitesse_vent         = (longueur_tige_anemometre/100) * 2 * M_PI * 3.6;
//la longueur est donné en centimètres, on le divise par 100 pour l'obtenir en mètres
//La multiplication par 3.6 permet de convertir la valeur de m/s à km/h.


//Définition de toutes les valeurs possibles lors de la lecture de la valeur de la girouette
/*
 * La girouette donne la direction du vent grâce à la valeur de la résistance entre ses deux broches de sortie.
 * Pour connaitre cette direction, il faut transformer la valeur de la résistance en une valeur de tension afin
 * de pouvoir la lire avec l'Arduino. Pour ce faire, un pont diviseur de tension doit être utilisé.
 *
 * L'une des broches de la girouette est reliée à la masse, tandis que l'autre est relié à une résistance
 * de 10 kilos ohms de tirage au 5v, tout cela relié à une broche d'entrée analogique Arduino.
 *
 * Grace aux valeurs fournies par la fiche technique, il est possible de calculer pour chaque angle la tension
 * entre la masse et la résistance grace au calcul suivant :
 *
 * tension = (Umax * valeur résistance girouette) / (résistance girouette + valeur résistance de tirage)
 *
 * Avec :
 * Umax = 5v
 * valeur résistance girouette = valeur de la résistance en fonction de l'angle sur la fiche technique
 * valeur résistance de tirage = 10000 (10 kilos ohms)
 *
 * Une fois les valeurs des tensions obtenues, il suffit de faire un produit en croix pour obtenir toutes les
 * valeurs théoriques lisible par l'Arduino :
 *
 * valeur = ( tension * 1023) / 5
 *
 *(voir tableau Excel pour toutes les valeurs)
 *
 *(elles sont pratiquement toutes fausses, il faut tout étalonner manuellement)
 */





//section Valeurs girouette

//Valeurs théoriques : {785,405,460,83,93,65,184,126,287,244,629,598,944,826,886,702}

//                                           valeur, tolérance
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
unsigned long temps_rafraichissement = 0; //nombre de millisecondes écoulées lors du dernier rafraîchissement
boolean doit_rafraichir = 0; //Est vraie lorsque l'afficheur doit être rafraîchi
byte menu = 0;
//Définit l'écran à afficher
//0 = Vent
//1 = Pluie
//2 = température
//3 = Lumière
//4 = Indice UV
//5 = étalonnage


//Variables capteur lumière/UV
byte valeur_ultraviolet;
unsigned long visible;


//Variables bouton
boolean etat_bouton = 0;                             //état du bouton après l'anti rebond
boolean dernier_etat_bouton;
boolean etat_lu_bouton;
boolean dernier_etat_lu_bouton = 0;
unsigned long temps_changement_etat_bouton = 0;      //est égal au nombre de millisecondes écoulées lorsque la valeur lue du bouton change d'état


//Anémomètre
float vitesse_vent;                                  //Vitesse du vent en km/h
unsigned long tableau_temps_anemometre[2] = {0,0};   //Tableau qui répertorie les millisecondes écoulées lors de chaque demi-tour de l'anémomètre après l'anti rebond
boolean indice_tableau_anemometre = 0;


//Girouette
byte indice_tableau_direction_vent;
word valeur_lu_girouette;
boolean pas_trouve;


//Pluviomètre
float valeur_pluie;                                  //valeur de pluie
unsigned long tableau_temps_pluviometre[2] = {0,0};  //Tableau qui répertorie les millisecondes écoulées lors de chaque déclenchement du pluviomètre après l'anti rebond
boolean indice_tableau_pluviometre = 0;


//Capteur de température
int valeur_termometre;
float temperature;


//Variables interrupts
volatile unsigned long dernier_temps_declanchement_anemometre  = 0; //est égal au nombre de millisecondes écoulées lors du dernier déclenchement de l'anémomètre
volatile boolean anemometre_declanche = 0;                          //est vraie lorsque l'anémomètre a été déclanché
volatile unsigned long dernier_temps_declanchement_pluviometre = 0; //est égal au nombre de millisecondes écoulées lors du dernier déclenchement du pluviomètre
volatile boolean pluviometre_declanche = 0;                         //est vraie lorsque le pluviomètre a été déclanché





//section Code interrupts

//Est éxecuté dès lors que l'anémomètre effectue un demi-tour (d'après nos tests, oui, mais pas selon la fiche technique)
void interrupt_anemometre() {
    dernier_temps_declanchement_anemometre = millis();
    anemometre_declanche = 1;
}


//Est éxecuté à chaque 0.2794mm de pluie.
void interrupt_pluviometre() {
    dernier_temps_declanchement_pluviometre = millis();
    pluviometre_declanche= 1 ;
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
    //Si la valeur de la girouette n'est pas trouvé dans le tableau, l'indice ne changera pas.
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
    //déclare ces broches comme devant être tiré au 5v par l'Arduino
    pinMode(broche_anemometre,INPUT_PULLUP);
    pinMode(broche_bouton, INPUT_PULLUP);
    pinMode(broche_pluviometre,INPUT_PULLUP);

    pinMode(broche_girouette, 0);           // entrée
    pinMode(broche_capteur_temperature, 0); // entrée

    attachInterrupt(digitalPinToInterrupt(broche_anemometre), interrupt_anemometre, FALLING);   //déclaration interruption anémomètre
    attachInterrupt(digitalPinToInterrupt(broche_pluviometre), interrupt_pluviometre, FALLING); //déclaration interruption pluviomètre

    lcd.begin(16, 2);
    Wire.begin(); //démarre la communication I2C

    if(!si1151.Begin()) {
        lcd.print("Capteur lumiere");
        lcd.setCursor(4, 1);
        lcd.print("pas pret");
        while (!si1151.Begin()) {} //attends que le capteur de lumière soit prêt
        lcd.clear();
    }

    //vérifie s'il y a besoin d'étalonner la girouette
    valeur_lu_girouette = analogRead(broche_girouette);
    cherche_indice_tableau_vent();
    if(pas_trouve){
        lcd.print("Verif val. gir.");
        lcd.setCursor(0,1);
        lcd.print("val. lue inconnue");
        delay(3000);
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
    //section lecture

    //Lis la lumière et l'ultraviolet
    valeur_ultraviolet = si1151.ReadHalfWord_UV();
    visible = si1151.ReadHalfWord();

    //Lis la valeur de la girouette
    valeur_lu_girouette = analogRead(broche_girouette);


    //Lis la valeur du thermomètre
    valeur_termometre = analogRead(broche_capteur_temperature);


    //Lis l'état du bouton
    dernier_etat_bouton = etat_bouton;
    dernier_etat_lu_bouton = etat_lu_bouton;
    etat_lu_bouton = !digitalRead(broche_bouton);
    //Quand le bouton est pressé, la valeur lue est 0 d'où la nécessité d'inverser la valeur lue pour refléter la valeur du bouton










    //section Debounce bouton

    if (etat_lu_bouton != dernier_etat_lu_bouton) {
        //met la variable au nombre de millisecondes écoulées lorsque la valeur lue du bouton change d'état
        temps_changement_etat_bouton = millis();
    }


    if ((millis() - temps_changement_etat_bouton) > delai_anti_rebond_bouton) {
        //si la valeur lue du bouton n'a pas changé pendant (constante) ms, on considère que l'état du bouton
        //est stabilisé, on peut utliliser sa valeur lue pour changer d'écran ou pas.
        etat_bouton = etat_lu_bouton;
    }








    //section vent

    if (dernier_temps_declanchement_anemometre + delai_anti_rebond_anemometre <= millis() && anemometre_declanche) {
        //stocke dans le tableau les millisecondes écoulées
        tableau_temps_anemometre[indice_tableau_anemometre] = dernier_temps_declanchement_anemometre;
        indice_tableau_anemometre = !indice_tableau_anemometre; //Change l'indice du tableau
        anemometre_declanche = 0;
    }

    //Si l'écart entre les deux temps dans le tableau est supérieur à 1 seconde (l'anémomètre a pris plus d'une
    //seconde à faire un demi-tour), on considère qu'il n'y a pas de vent.

    if (tableau_temps_anemometre[!indice_tableau_anemometre] - tableau_temps_anemometre[indice_tableau_anemometre] < 1000 && tableau_temps_anemometre[1]) {
        //la deuxième condition sert à vérifier s'il y a bien deux valeurs dans le tableau depuis le début du programme et non pas qu'une seule
        vitesse_vent = pre_calcul_vitesse_vent * (1.0 / ( float(tableau_temps_anemometre[!indice_tableau_anemometre] - tableau_temps_anemometre[indice_tableau_anemometre]) * 0.002 ) );
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
     * quarts de tour. On se sert du temps pris pour faire un demi-tour pour calculer la vitesse du vent à la place.
     *
     */









    //section pluie
    if (dernier_temps_declanchement_pluviometre + delai_anti_rebond_pluviometre <= millis() && pluviometre_declanche){
        //stocke dans le tableau les millisecondes écoulées
        tableau_temps_pluviometre[indice_tableau_pluviometre] = dernier_temps_declanchement_pluviometre;
        indice_tableau_pluviometre = !indice_tableau_pluviometre; // Change l'indice du tableau
        pluviometre_declanche = 0;
    }



    if ((tableau_temps_pluviometre[!indice_tableau_pluviometre] + temps_abandon_pluviometre_ms) > millis() && tableau_temps_pluviometre[0,1]) {
        valeur_pluie = (1.0 / float(tableau_temps_pluviometre[!indice_tableau_pluviometre] - tableau_temps_pluviometre[indice_tableau_pluviometre]))*3600.0 * 0.2794;
    }
    else {
        //aucune nouvelle donnée du pluviomètre au bout d'une minute, on considère qu'il ne pleut pas
        valeur_pluie = 0;
    }
    /* Calcul pluie :
     *
     * Pluie/h [en mm/m²] = nombre d'activations du pluviomètre par heure (moyenne) * volume pour une activation
     *
     * Donc pour le nombre d'activations par seconde, il faut faire la difference des millisecondes écoulées entre deux fronts déscendants (falling edge),
     * l'inverser pour avoir le nombre de d'activations/seconde, puis ont le multiplient par 3600 pour savoir pour une heure le nombre d'activations théorique
     * et multiplier par 0.2794 pour avoir le volume de pluie par heure en mm/m².
     *
     */










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
            //va à l'écran suivant et rollback à 5.
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

    if (etat_bouton && temps_changement_etat_bouton + temps_mode_etalonnage_ms < millis() && menu == 1) {
        menu = 5;
        temps_rafraichissement = 0;
    }



    //section Changement d'écran pour les valeurs

    if (temps_rafraichissement <= millis()) {

        cherche_indice_tableau_vent();
        temps_rafraichissement = delai_rafraichissement + millis();

        // affiche les valeurs à l'écran
        switch (menu) {
            case 0: // vent
                if (vitesse_vent > 0) {
                    lcd.setCursor(6, 0);
                    lcd.print(vitesse_vent);
                    lcd.setCursor(0, 1);
                    lcd.print(tableau_direction_vent[indice_tableau_direction_vent]);
                }
                else {
                    lcd.setCursor(6, 0);
                    lcd.print(" --- ");
                    lcd.setCursor(2, 1);
                    lcd.print("Pas de vent");
                }
                break;

            case 1: //pluie

                lcd.setCursor(0, 1);
                lcd.print("    ");
                lcd.print(valeur_pluie);
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
                if (valeur_ultraviolet >= 11) {
                    lcd.print("    Extreme");
                }
                else if (valeur_ultraviolet < 11 && valeur_ultraviolet >= 8) {
                    lcd.print("   Tres haut");
                }
                else if (valeur_ultraviolet < 8 && valeur_ultraviolet >= 6) {
                    lcd.print("      Haut");
                }
                else if (valeur_ultraviolet < 6 && valeur_ultraviolet >= 3) {
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
        lcd.print("                ");
    }
}
