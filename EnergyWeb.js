const ctx = document.getElementById("currentChart").getContext("2d");

let labels = [];
let dataPoints = [];

const currentChart = new Chart(ctx, {
    type: "line",
    data: {
        labels: labels,
        datasets: [{
            label: "Current (A)",
            data: dataPoints,
            borderColor: "#3aa0c8",
            fill: false,
            tension: 0.2
        }]
    },
    options: {
        responsive: true,
        scales: {
            y: {
                beginAtZero: true
            }
        }
    }
});

function fetchCurrentData() {
    fetch("http://localhost:5000/api/current")
        .then(response => response.json())
        .then(data => {
            const current = data.current;

            // Update big number
            document.getElementById("currentValue").innerText = current.toFixed(2);

            // Update chart
            const now = new Date().toLocaleTimeString();

            labels.push(now);
            dataPoints.push(current);

            if (labels.length > 20) {
                labels.shift();
                dataPoints.shift();
            }

            currentChart.update();
        })
        .catch(error => console.error("Error:", error));
}

// Fetch every 5 seconds
setInterval(fetchCurrentData, 5000);

// Initial load
fetchCurrentData();