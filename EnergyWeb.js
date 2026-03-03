const powerValue = document.getElementById("powerValue");
const energyValue = document.getElementById("energyValue");
const wasteValue = document.getElementById("wasteValue");

let totalEnergy = 0;
let wasteEnergy = 0;

const powerChart = new Chart(document.getElementById("powerChart"), {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Power (W)',
            data: [],
            borderColor: '#3aa0c8',
            fill: false
        }]
    }
});

const wasteChart = new Chart(document.getElementById("wasteChart"), {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Wastage (W)',
            data: [],
            borderColor: 'red',
            fill: false
        }]
    }
});

function fetchData() {

    fetch("http://localhost:5000/api/data")
        .then(res => res.json())
        .then(data => {

            const power = data.power;
            const current = data.current;
            const voltage = data.voltage;

            powerValue.innerText = power.toFixed(2);

            // Energy Calculation (simplified)
            totalEnergy += power / 3600000;
            energyValue.innerText = totalEnergy.toFixed(4);

            // Waste Logic
            let waste = 0;

            if(power < 10 && current > 0) {
                waste = power;
                wasteEnergy += waste / 3600000;
            }

            wasteValue.innerText = waste.toFixed(2);

            const time = new Date().toLocaleTimeString();

            powerChart.data.labels.push(time);
            powerChart.data.datasets[0].data.push(power);
            powerChart.update();

            wasteChart.data.labels.push(time);
            wasteChart.data.datasets[0].data.push(waste);
            wasteChart.update();

        });
}

setInterval(fetchData, 5000);
