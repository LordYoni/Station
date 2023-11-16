<?php
session_start();

if (isset($_SESSION['username'])) {
    header("Location: dashboard.php");
    exit();
}
?>

<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="css/dashboard.css">
    <script src="https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.29.1/moment.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <link rel="icon" href="img/icon.png">
    <title>Dashboard</title>
</head>
<body>
<a href="index.php" class="logout-button">Déconnexion</a>
<div class="dashboard-container">
    <div class="item card-container">
        <h2>Heure</h2>
        <p id="clock"></p>
    </div>

    <div class="item card-container">
        <h2>Température</h2>
        <div class="slider-container">
            <input type="range" id="temperatureSlider" min="-20" max="60" value="20">
            <p id="temperatureValue">20 °C</p>
        </div>
    </div>

    <div class="item card-container">
        <h2>Uptime</h2>
        <p id="uptimeCounter">0 seconds</p>
    </div>

    <div class="item card-container">
        <h2>Pression Atmosphérique</h2>
        <div class="slider-container">
            <input type="range" id="pressureSlider" min="800" max="1200" value="1000">
            <p id="pressureValue">1000 hPa</p>
        </div>
    </div>

    <div class="item card-container">
        <h2>Humidité</h2>
        <div class="slider-container">
            <input type="range" id="humiditySlider" min="0" max="100" value="50">
            <p id="humidityValue">50 %</p>
        </div>
    </div>

    <div class="item card-container">
        <h2>Vitesse du Vent</h2>
        <div class="slider-container">
            <input type="range" id="windSpeedSlider" min="0" max="50" value="10">
            <p id="windSpeedValue">10 m/s</p>
        </div>
    </div>

    <div class="item big-card chart">
        <h2>Évolution de la Température</h2>
        <canvas id="temperatureChart" width="400" height="200"></canvas>
    </div>
</div>

<script src="js/dashboard.js"></script>
</body>
</html>
