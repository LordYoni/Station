// Définition des informations d'identification autorisées.
var credentials = [
    { username: 'Yoni', password: 'Taine' },
    { username: 'Eliot', password: 'Hauet' },
    { username: 'Admin', password: 'Admin' },
];

// Fonction pour valider les informations de connexion.
function validateLogin() {
    // Récupère les valeurs des champs de saisie.
    var username = document.getElementById("username").value;
    var password = document.getElementById("password").value;

    // Récupère l'élément d'affichage des messages d'erreur.
    var errorMessage = document.getElementById("errorMessage");

    // Vérifie si les informations d'identification fournies sont valides.
    var isValid = credentials.some(function (cred) {
        return cred.username === username && cred.password === password;
    });

    // Redirige vers le tableau de bord si les informations d'identification sont valides.
    if (isValid) {
        window.location.href = "dashboard.php";
    } else {
        // Affiche un message d'erreur si les informations d'identification sont incorrectes.
        errorMessage.innerHTML = "Nom d'utilisateur ou mot de passe incorrect. Veuillez réessayer.";
    }
}
