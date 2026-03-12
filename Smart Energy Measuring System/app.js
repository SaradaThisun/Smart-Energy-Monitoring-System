// ── Firebase Configuration ─────────────────────────────────────────
const firebaseConfig = {
    apiKey: "AIzaSyAXRuermsU80P5qGt8bVIv7Jg-e8FQ0KxY",
    authDomain: "project-c6ce3.firebaseapp.com",
    databaseURL: "https://project-c6ce3-default-rtdb.asia-southeast1.firebasedatabase.app",
    projectId: "project-c6ce3",
    storageBucket: "project-c6ce3.firebasestorage.app",
    messagingSenderId: "442112857688",
    appId: "1:442112857688:web:e5459755241cd28f0b5c2d"
};

// Initialize Firebase
firebase.initializeApp(firebaseConfig);
const db = firebase.database();

// ── Global Variables ─────────────────────────────────────────────
const labels = [];
const powers = [];

// ── Chart Configuration ─────────────────────────────────────────────
const chart = new Chart(document.getElementById('myChart'), {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'Power (W)', 
            data: powers,
            borderColor: '#38bdf8', 
            backgroundColor: 'rgba(56,189,248,0.1)',
            borderWidth: 2, 
            tension: 0.4, 
            fill: true
        }]
    },
    options: {
        responsive: true,
        plugins: { 
            legend: { display: false } 
        },
        scales: {
            x: { 
                ticks: { color: '#64748b', maxTicksLimit: 8 }, 
                grid: { color: '#1e293b' } 
            },
            y: { 
                ticks: { color: '#64748b' }, 
                grid: { color: '#1e293b' },
                beginAtZero: true
            }
        }
    }
});

// ── Helper Functions ─────────────────────────────────────────────
function calculatePower(voltage, current) {
    return (voltage * current).toFixed(1);
}

function calculateEfficiency(voltage, current, power) {
    if (voltage * current === 0) return 0;
    const apparentPower = voltage * current;
    const efficiency = (power / apparentPower) * 100;
    return Math.min(efficiency, 100).toFixed(1);
}

function addChartPoint(power, timestamp) {
    const time = timestamp ? new Date(parseInt(timestamp)).toLocaleTimeString() : new Date().toLocaleTimeString();
    labels.push(time);
    powers.push(power);
    if (labels.length > 20) { 
        labels.shift(); 
        powers.shift(); 
    }
    chart.update();
}

// ── Listen for data at /current/readings ──
db.ref('/current/readings').on('value', (snapshot) => {
    const data = snapshot.val();
    console.log("Current readings data:", data);
    
    if (!data) {
        console.log("No data at /current/readings");
        return;
    }

    // Extract data
    const voltage = data.voltage || 0;
    const current = data.current || 0;
    const temperature = data.temperature || 25;
    const status = data.status || 'NORMAL';
    const last_update = data.last_update;
    
    console.log("Voltage:", voltage, "Current:", current, "Temp:", temperature);
    
    // Update main cards
    const power = calculatePower(voltage, current);
    const efficiency = calculateEfficiency(voltage, current, parseFloat(power));
    
    document.getElementById('cPower').textContent = power + ' W';
    document.getElementById('cEff').textContent = efficiency + ' %';
    
    if (last_update) {
        const updateTime = new Date(parseInt(last_update)).toLocaleTimeString();
        document.getElementById('lastUpdate').textContent = 'Last updated: ' + updateTime;
    }
    
    addChartPoint(parseFloat(power), last_update);
    
    // Update device cards with this data
    updateDeviceCards({
        voltage: voltage,
        current: current,
        temperature: temperature,
        status: status,
        last_update: last_update
    });
});

// ── Update Device Cards ──
function updateDeviceCards(currentData) {
    db.ref('/devices').once('value', (snapshot) => {
        const devices = snapshot.val();
        const deviceList = document.getElementById('deviceList');
        
        if (!deviceList) return;
        deviceList.innerHTML = '';
        
        if (!devices) {
            deviceList.innerHTML = '<div style="color:white; padding:20px;">No devices found</div>';
            return;
        }
        
        let onlineCount = 0;
        let totalDevices = 0;
        
        Object.keys(devices).forEach(deviceId => {
            const device = devices[deviceId];
            totalDevices++;
            
            // Use the device's online status for counting
            const isOnline = device.online === true;
            if (isOnline) onlineCount++;
            
            // Use current data for sensor values
            const temperature = currentData.temperature || 25;
            const voltage = currentData.voltage || 0;
            const current = currentData.current || 0;
            const status = currentData.status || 'NORMAL';
            
            const power = calculatePower(voltage, current);
            const efficiency = calculateEfficiency(voltage, current, parseFloat(power));
            
            const isOverheating = temperature >= 55;
            const isWarning = temperature >= 45 && temperature < 55;
            
            const div = document.createElement('div');
            div.className = 'device' + (isOverheating ? ' hot' : (isWarning ? ' warm' : ''));
            
            // REMOVED: Online badge from the device card
            div.innerHTML = `
                <div style="margin-bottom: 15px;">
                    <h3 style="margin:0; font-size: 22px; font-weight: bold;">${device.name || 'ESP32 Device'}</h3>
                    <small style="color: #64748b; font-size: 14px;">${deviceId}</small>
                </div>

                <div class="temp-row" style="margin-bottom: 18px;">
                    <span style="font-size: 16px; color:#94a3b8;">🌡️ Temperature</span>
                    <div class="temp-bar-track" style="height: 12px;">
                        <div class="temp-bar-fill ${isOverheating ? 'hot' : (isWarning ? 'warm' : '')}" 
                             style="width:${Math.min((temperature / 60) * 100, 100)}%; height: 12px;"></div>
                    </div>
                    <span class="temp-val ${isOverheating ? 'hot' : (isWarning ? 'warm' : '')}" style="font-size: 24px;">
                        ${temperature.toFixed(1)}°C
                    </span>
                </div>

                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin: 18px 0;">
                    <div style="background: #0f172a; padding: 15px; border-radius: 10px;">
                        <div style="font-size: 14px; color: #94a3b8; margin-bottom: 5px;">Voltage</div>
                        <div style="font-size: 28px; font-weight: bold; color: #60a5fa;">${voltage.toFixed(1)}<span style="font-size: 16px;">V</span></div>
                    </div>
                    <div style="background: #0f172a; padding: 15px; border-radius: 10px;">
                        <div style="font-size: 14px; color: #94a3b8; margin-bottom: 5px;">Current</div>
                        <div style="font-size: 28px; font-weight: bold; color: #34d399;">${current.toFixed(2)}<span style="font-size: 16px;">A</span></div>
                    </div>
                </div>

                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin: 18px 0;">
                    <div style="background: #0f172a; padding: 15px; border-radius: 10px;">
                        <div style="font-size: 14px; color: #94a3b8; margin-bottom: 5px;">Power</div>
                        <div style="font-size: 28px; font-weight: bold; color: #fbbf24;">${power}<span style="font-size: 16px;">W</span></div>
                    </div>
                    <div style="background: #0f172a; padding: 15px; border-radius: 10px;">
                        <div style="font-size: 14px; color: #94a3b8; margin-bottom: 5px;">Efficiency</div>
                        <div style="font-size: 28px; font-weight: bold; color: #a78bfa;">${efficiency}<span style="font-size: 16px;">%</span></div>
                    </div>
                </div>

                <div style="background: #0f172a; padding: 12px 15px; border-radius: 10px; margin: 15px 0;">
                    <p style="font-size: 16px; color: #64748b; margin: 0;">
                        📊 Status: <span style="color: ${status === 'CRITICAL' ? '#ef4444' : (status === 'WARNING' ? '#f97316' : '#4ade80')}; font-weight: bold; font-size: 18px;">
                            ${status}
                        </span>
                    </p>
                </div>

                ${isOverheating ? 
                    '<p style="color:#ef4444;font-size:16px;margin-top:12px; padding: 10px; background: rgba(239,68,68,0.1); border-radius: 8px;">🔴 CRITICAL: Temperature exceeded 55°C - Relay OFF!</p>' : ''}
                ${isWarning && !isOverheating ? 
                    '<p style="color:#f97316;font-size:16px;margin-top:12px; padding: 10px; background: rgba(249,115,22,0.1); border-radius: 8px;">⚠️ WARNING: High temperature</p>' : ''}
            `;
            
            deviceList.appendChild(div);
        });
        
        // Update online count display at the top of the page only
        document.getElementById('cOnline').textContent = onlineCount + ' / ' + totalDevices;
        console.log(`Online devices: ${onlineCount}/${totalDevices}`);
    });
}

// ── Historical Data Listener ──
db.ref('/realtime_data').orderByKey().limitToLast(100).on('value', (snapshot) => {
    const data = snapshot.val();
    if (!data) return;
    
    let totalEnergy = 0;
    let readings = [];
    
    Object.keys(data).forEach(key => {
        const reading = data[key];
        if (reading.timestamp && reading.voltage && reading.current) {
            const power = reading.voltage * reading.current;
            readings.push({
                timestamp: parseInt(reading.timestamp),
                power: power
            });
        }
    });
    
    readings.sort((a, b) => a.timestamp - b.timestamp);
    
    if (readings.length > 1) {
        for (let i = 1; i < readings.length; i++) {
            const timeDiff = (readings[i].timestamp - readings[i-1].timestamp) / 1000 / 3600;
            const avgPower = (readings[i].power + readings[i-1].power) / 2;
            totalEnergy += avgPower * timeDiff / 1000;
        }
    }
    
    document.getElementById('cDaily').textContent = totalEnergy.toFixed(2) + ' kWh';
    updateBar(totalEnergy);
});

function updateBar(kwh) {
    const limit = 3.0;
    const pct = Math.min((kwh / limit) * 100, 100);
    const fill = document.getElementById('barFill');
    
    if (fill) {
        fill.style.width = pct + '%';
        fill.className = 'bar-fill' + (kwh >= limit * 0.8 ? ' danger' : '');
    }
    
    const barLabel = document.getElementById('barLabel');
    if (barLabel) {
        barLabel.textContent = 'Daily Usage: ' + parseFloat(kwh).toFixed(2) + ' / ' + limit + ' kWh';
    }

    const warn = document.getElementById('warning');
    if (warn) {
        if (kwh >= limit) {
            warn.style.display = 'block';
            warn.textContent = '⚠️ ALERT: Daily usage ' + parseFloat(kwh).toFixed(2) + ' kWh exceeded 3 kWh limit!';
        } else if (kwh >= limit * 0.8) {
            warn.style.display = 'block';
            warn.textContent = '⚠️ WARNING: Approaching limit — ' + parseFloat(kwh).toFixed(2) + ' kWh (' + pct.toFixed(0) + '%)';
        } else {
            warn.style.display = 'none';
        }
    }
}

console.log("Fixed: Online status removed from device cards - only shown in top summary");