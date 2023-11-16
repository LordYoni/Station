// Initialise la variable de temps de démarrage pour le calcul de l'uptime.
var uptimeStartTime = Math.floor(Date.now() / 1000);

// Fonction pour mettre à jour l'affichage de l'uptime.
function updateUptime() {
    // Récupère l'élément d'affichage de l'uptime.
    var uptimeCounter = document.getElementById('uptimeCounter');

    // Obtient le temps actuel en secondes.
    var currentTime = Math.floor(Date.now() / 1000);

    // Calcule la durée d'uptime en secondes.
    var uptimeValue = currentTime - uptimeStartTime;

    // Convertit la durée en jours, heures, minutes et secondes.
    var days = Math.floor(uptimeValue / (24 * 3600));
    var hours = Math.floor((uptimeValue % (24 * 3600)) / 3600);
    var minutes = Math.floor((uptimeValue % 3600) / 60);
    var seconds = uptimeValue % 60;

    // Met à jour l'élément d'affichage de l'uptime.
    uptimeCounter.innerText = days + ' jours ' + hours + ' heures ' + minutes + ' minutes ' + seconds + ' secondes';
}

function updateTemperature() {
    var slider = document.getElementById('temperatureSlider');
    var temperatureValue = document.getElementById('temperatureValue');

    var value = parseInt(slider.value);

    temperatureValue.innerText = value + ' °C';
}

function updateHumidity() {
    var slider = document.getElementById('humiditySlider');
    var humidityValue = document.getElementById('humidityValue');

    var value = parseInt(slider.value);

    humidityValue.innerText = value + ' %';
}

function updatePressure() {
    var slider = document.getElementById('pressureSlider');
    var pressureValue = document.getElementById('pressureValue');

    var value = parseInt(slider.value);

    pressureValue.innerText = value + ' hPa';
}

function updateWindSpeed() {
    var slider = document.getElementById('windSpeedSlider');
    var windSpeedValue = document.getElementById('windSpeedValue');

    var value = parseInt(slider.value);
    var kmPerHour = value * 3.6;

    windSpeedValue.innerText = kmPerHour.toFixed(2) + ' km/h';
}

var temperatureSlider = document.getElementById('temperatureSlider');
temperatureSlider.addEventListener('input', updateTemperature);
updateTemperature();

var humiditySlider = document.getElementById('humiditySlider');
humiditySlider.addEventListener('input', updateHumidity);
updateHumidity();

var pressureSlider = document.getElementById('pressureSlider');
pressureSlider.addEventListener('input', updatePressure);
updatePressure();

var windSpeedSlider = document.getElementById('windSpeedSlider');
windSpeedSlider.addEventListener('input', updateWindSpeed);
updateWindSpeed();

setInterval(updateUptime, 1000);

// Met à jour l'heure actuelle dans l'élément d'affichage de l'horloge.
function updateClock() {
    var now = new Date();
    var hours = now.getHours();
    var minutes = now.getMinutes();
    var seconds = now.getSeconds();

    document.getElementById('clock').innerHTML = hours + ' h ' + minutes + ' min ' + seconds + ' s ';
}

setInterval(updateClock, 1000);

// Initialise les données et les options du graphique de température.
var temperatureData = {
    labels: [],
    datasets: [{
        label: 'Température (°C)',
        borderColor: '#3498db',
        data: [],
        fill: false,
    }]
};

// Crée un objet Chart pour afficher le graphique de température.
var temperatureChart = new Chart(document.getElementById('temperatureChart').getContext('2d'), {
    type: 'line',
    data: temperatureData,
    options: {
        scales: {
            // Configuration de l'axe x (horizontal) du graphique.
            x: [{
                type: 'time',
                time: {
                    unit: 'hour',
                    displayFormats: {
                        hour: 'H:mm'
                    }
                },
                title: {
                    display: true,
                    text: 'Heures'
                }
            }],
            // Configuration de l'axe y (vertical) du graphique.
            y: {
                type: 'linear',
                position: 'left',
                title: {
                    display: true,
                    text: 'Température (°C)'
                }
            }
        }
    }
});

// Fonction pour mettre à jour le graphique de température.
function updateTemperatureChart() {
    var now = new Date();
    var currentHour = now.getHours();
    var label = moment(now).format('H:mm');
    var value = parseInt(document.getElementById('temperatureSlider').value);

    // Vérifie si une nouvelle heure a commencé et ajuste les données en conséquence.
    if (temperatureData.labels.length === 0 || currentHour !== moment(temperatureData.labels[temperatureData.labels.length - 1], 'H:mm').hours()) {
        temperatureData.labels = [label];
        temperatureData.datasets[0].data = [value];
    } else {
        temperatureData.labels.push(label);
        temperatureData.datasets[0].data.push(value);
    }

    // Limite le nombre d'éléments affichés dans le graphique à 12.
    if (temperatureData.labels.length > 12) {
        temperatureData.labels.shift();
        temperatureData.datasets[0].data.shift();
    }

    // Met à jour le graphique.
    temperatureChart.update();
}

// Appelle la fonction de mise à jour du graphique de température au chargement et toutes les 10 minutes.
updateTemperatureChart();
setInterval(updateTemperatureChart, 2000);
