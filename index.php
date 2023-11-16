<?php
?>
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="css/style.css">
    <link rel="icon" href="img/icon.png">
    <title>Connexion</title>
</head>
<body>
<div class="login-container">
    <form id="loginForm" autocomplete="off">
        <h2>Connexion</h2>
        <div class="input-group">
            <label for="username">Identifiant</label>
            <input type="text" id="username" name="username" required>
        </div>
        <div class="input-group">
            <label for="password">Mot de passe</label>
            <input type="password" id="password" name="password" required>
        </div>
        <p id="errorMessage" class="error-message"></p>
        <button type="button" onclick="validateLogin()">Connexion</button>
    </form>
</div>
<script src="js/script.js"></script>
</body>
</html>
