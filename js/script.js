var credentials = [
    { username: 'Yoni', password: 'taine' },
    { username: 'Eliot', password: 'hauet' },
];

function validateLogin() {
    var username = document.getElementById("username").value;
    var password = document.getElementById("password").value;
    var errorMessage = document.getElementById("errorMessage");

    var isValid = credentials.some(function (cred) {
        return cred.username === username && cred.password === password;
    });

    if (isValid) {
        window.location.href = "dashboard.php";
    } else {
        errorMessage.innerHTML = "Nom d'utilisateur ou mot de passe incorrect. Veuillez réessayer.";
    }
}
