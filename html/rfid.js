// WebSocket variables
let socket;
let isConnected = false;
const RECONNECT_INTERVAL = 5000;
const HEARTBEAT_INTERVAL = 10000;
let heartbeatTimer = null;
let lastHeartbeatResponse = Date.now();
const HEARTBEAT_TIMEOUT = 20000;
let reconnectTimer = null;
let spoolDetected = false;

// WebSocket functions
function startHeartbeat() {
    if (heartbeatTimer) clearInterval(heartbeatTimer);

    heartbeatTimer = setInterval(() => {
        // Check if no response for too long
        if (Date.now() - lastHeartbeatResponse > HEARTBEAT_TIMEOUT) {
            isConnected = false;
            updateConnectionStatus();
            if (socket) {
                socket.close();
                socket = null;
            }
            return;
        }

        if (!socket || socket.readyState !== WebSocket.OPEN) {
            isConnected = false;
            updateConnectionStatus();
            return;
        }

        try {
            // Send heartbeat
            socket.send(JSON.stringify({ type: 'heartbeat' }));
        } catch (error) {
            isConnected = false;
            updateConnectionStatus();
            if (socket) {
                socket.close();
                socket = null;
            }
        }
    }, HEARTBEAT_INTERVAL);
}

function initWebSocket() {
    // Clear any existing reconnect timer
    if (reconnectTimer) {
        clearTimeout(reconnectTimer);
        reconnectTimer = null;
    }

    // If an existing connection exists, close it first
    if (socket) {
        socket.close();
        socket = null;
    }

    try {
        socket = new WebSocket('ws://' + window.location.host + '/ws');
        
        socket.onopen = function() {
            isConnected = true;
            updateConnectionStatus();
            startHeartbeat(); // Start heartbeat after successful connection
        };
        
        socket.onclose = function() {
            isConnected = false;
            updateConnectionStatus();
            if (heartbeatTimer) clearInterval(heartbeatTimer);
            
            // Only attempt new connection if no timer is running
            if (!reconnectTimer) {
                reconnectTimer = setTimeout(() => {
                    initWebSocket();
                }, RECONNECT_INTERVAL);
            }
        };
        
        socket.onerror = function(error) {
            isConnected = false;
            updateConnectionStatus();
            if (heartbeatTimer) clearInterval(heartbeatTimer);

            // On error close connection and reconnect
            if (socket) {
                socket.close();
                socket = null;
            }
        };
        
        socket.onmessage = function(event) {
            lastHeartbeatResponse = Date.now(); // Update timestamp on every server response
            
            const data = JSON.parse(event.data);
            if (data.type === 'amsData') {
                displayAmsData(data.payload);
            } else if (data.type === 'nfcTag') {
                updateNfcStatusIndicator(data.payload);
            } else if (data.type === 'nfcData') {
                // Pass format info for OpenPrintTag detection
                if (data.format === 'openprinttag' && data.payload) {
                    data.payload.format = 'openprinttag';
                }
                updateNfcData(data.payload);
            } else if (data.type === 'writeNfcTag') {
                handleWriteNfcTagResponse(data.success);
            } else if (data.type === 'heartbeat') {
                // Optional: Specific handling of heartbeat responses
                // Update status dots
                const bambuDot = document.getElementById('bambuDot');
                const spoolmanDot = document.getElementById('spoolmanDot');
                const ramStatus = document.getElementById('ramStatus');

                if (bambuDot) {
                    bambuDot.className = 'status-dot ' + (data.bambu_connected ? 'online' : 'offline');
                    // Add click handler only when offline
                    if (!data.bambu_connected) {
                        bambuDot.style.cursor = 'pointer';
                        bambuDot.onclick = function() {
                            if (socket && socket.readyState === WebSocket.OPEN) {
                                socket.send(JSON.stringify({
                                    type: 'reconnect',
                                    payload: 'bambu'
                                }));
                            }
                        };
                    } else {
                        bambuDot.style.cursor = 'default';
                        bambuDot.onclick = null;
                    }
                }
                if (spoolmanDot) {
                    spoolmanDot.className = 'status-dot ' + (data.spoolman_connected ? 'online' : 'offline');
                    // Add click handler only when offline
                    if (!data.spoolman_connected) {
                        spoolmanDot.style.cursor = 'pointer';
                        spoolmanDot.onclick = function() {
                            if (socket && socket.readyState === WebSocket.OPEN) {
                                socket.send(JSON.stringify({
                                    type: 'reconnect',
                                    payload: 'spoolman'
                                }));
                            }
                        };
                    } else {
                        spoolmanDot.style.cursor = 'default';
                        spoolmanDot.onclick = null;
                    }
                }
                if (ramStatus) {
                    ramStatus.textContent = `${data.freeHeap}k`;
                }
            }
            else if (data.type === 'setSpoolmanSettings') {
                if (data.payload == 'success') {
                    showNotification(`Spoolman Settings set successfully`, true);
                } else {
                    showNotification(`Error setting Spoolman Settings`, false);
                }
            }
        };
    } catch (error) {
        isConnected = false;
        updateConnectionStatus();
        
        // Only attempt new connection if no timer is running
        if (!reconnectTimer) {
            reconnectTimer = setTimeout(() => {
                initWebSocket();
            }, RECONNECT_INTERVAL);
        }
    }
}

function updateConnectionStatus() {
    const statusElement = document.querySelector('.connection-status');
    if (!isConnected) {
        statusElement.classList.remove('hidden');
        // Add delay so CSS transition can take effect
        setTimeout(() => {
            statusElement.classList.add('visible');
        }, 10);
    } else {
        statusElement.classList.remove('visible');
        // Wait for fade-out animation to finish before setting hidden
        setTimeout(() => {
            statusElement.classList.add('hidden');
        }, 300);
    }
}

// Event Listeners
document.addEventListener("DOMContentLoaded", function() {
    initWebSocket();
    
    // Event listener for checkbox
    document.getElementById("onlyWithoutSmId").addEventListener("change", function() {
        const spoolsData = window.getSpoolData();
        window.populateVendorDropdown(spoolsData);
    });
});

// Event listener for Spoolman events
document.addEventListener('spoolDataLoaded', function(event) {
    window.populateVendorDropdown(event.detail);
});

document.addEventListener('spoolmanError', function(event) {
    showNotification(`Spoolman Error: ${event.detail.message}`, false);
});

document.addEventListener('filamentSelected', function (event) {
    updateNfcInfo();
    // Show spool buttons when a filament is selected
    const selectedText = document.getElementById("selected-filament").textContent;
    updateSpoolButtons(selectedText !== "Please choose...");
});

function updateNfcInfo() {
    const selectedText = document.getElementById("selected-filament").textContent;
    const nfcInfo = document.getElementById("nfcInfo");
    const writeButton = document.getElementById("writeNfcButton");

    if (selectedText === "Please choose...") {
        nfcInfo.textContent = "No Filament selected";
        nfcInfo.classList.remove("nfc-success", "nfc-error");
        writeButton.classList.add("hidden");
        return;
    }

    // Find the selected spool in the data
    const selectedSpool = spoolsData.find(spool => 
        `${spool.id} | ${spool.filament.name} (${spool.filament.material})` === selectedText
    );

    if (selectedSpool) {
        writeButton.classList.remove("hidden");
    } else {
        writeButton.classList.add("hidden");
    }
}

function displayAmsData(amsData) {
    const amsDataContainer = document.getElementById('amsData');
    amsDataContainer.innerHTML = ''; 

    amsData.forEach((ams) => {
        // Determine the display name for the AMS
        const amsDisplayName = ams.ams_id === 255 ? 'External Spool' : `AMS ${ams.ams_id}`;
        
        const trayHTML = ams.tray.map(tray => {
            // Check if any data is present
            const relevantFields = ['tray_type', 'tray_sub_brands', 'tray_info_idx', 'setting_id', 'cali_idx'];
            const hasAnyContent = relevantFields.some(field => 
                tray[field] !== null && 
                tray[field] !== undefined && 
                tray[field] !== '' &&
                tray[field] !== 'null'
            );

            // Determine the display name for the tray
            const trayDisplayName = (ams.ams_id === 255) ? 'External' : `Tray ${tray.id}`;

            // Only create button HTML for non-empty trays
            const buttonHtml = `
                <button class="spool-button" onclick="handleSpoolIn(${ams.ams_id}, ${tray.id})" 
                        style="position: absolute; top: -30px; left: -15px; 
                               background: none; border: none; padding: 0; 
                               cursor: pointer; display: none;">
                    <img src="spool_in.png" alt="Spool In" style="width: 48px; height: 48px;">
                </button>`;
            
                        // Only create button HTML for non-empty trays
            const outButtonHtml = `
                <button class="spool-button" onclick="handleSpoolOut()" 
                        style="position: absolute; top: -35px; right: -15px; 
                               background: none; border: none; padding: 0; 
                               cursor: pointer; display: block;">
                    <img src="spool_in.png" alt="Spool In" style="width: 48px; height: 48px; transform: rotate(180deg) scaleX(-1);">
                </button>`;

            const spoolmanButtonHtml = `
                <button class="spool-button" onclick="handleSpoolmanSettings('${tray.tray_info_idx}', '${tray.setting_id}', '${tray.cali_idx}', '${tray.nozzle_temp_min}', '${tray.nozzle_temp_max}')" 
                        style="position: absolute; bottom: 0px; right: 0px; 
                               background: none; border: none; padding: 0; 
                               cursor: pointer; display: none;">
                    <img src="set_spoolman.png" alt="Spool In" style="width: 38px; height: 38px;">
                </button>`;

            if (!hasAnyContent) {
                return `
                    <div class="tray">
                        <p class="tray-head">${trayDisplayName}</p>
                        <p>
                            ${(ams.ams_id === 255 && tray.tray_type === '') ? buttonHtml : ''}
                            Empty
                        </p>
                    </div>
                    <hr>`;
            }

            // Generate the type together with color box
            const typeWithColor = tray.tray_type ? 
                `<p>Typ: ${tray.tray_type} ${tray.tray_color ? `<span style="
                    background-color: #${tray.tray_color}; 
                    width: 20px; 
                    height: 20px; 
                    display: inline-block; 
                    vertical-align: middle;
                    border: 1px solid #333;
                    border-radius: 3px;
                    margin-left: 5px;"></span>` : ''}</p>` : '';

            // Array with remaining tray properties
            const trayProperties = [
                { key: 'tray_sub_brands', label: 'Sub Brands' },
                { key: 'tray_info_idx', label: 'Filament IDX' },
                { key: 'setting_id', label: 'Setting ID' },
                { key: 'cali_idx', label: 'Calibration IDX' }
            ];

            // Only display valid fields
            const trayDetails = trayProperties
                .filter(prop => 
                    tray[prop.key] !== null && 
                    tray[prop.key] !== undefined && 
                    tray[prop.key] !== '' &&
                    tray[prop.key] !== 'null'
                )
                .map(prop => {
                    // Special handling for setting_id
                    if (prop.key === 'cali_idx' && tray[prop.key] === '-1') {
                        return `<p>${prop.label}: not calibrated</p>`;
                    }
                    return `<p>${prop.label}: ${tray[prop.key]}</p>`;
                })
                .join('');

            // Only show temperatures when both are not 0
            const tempHTML = (tray.nozzle_temp_min > 0 && tray.nozzle_temp_max > 0) 
                ? `<p>Nozzle Temp: ${tray.nozzle_temp_min}°C - ${tray.nozzle_temp_max}°C</p>`
                : '';

            return `
                <div class="tray" ${tray.tray_color ? `style="border-left: 4px solid #${tray.tray_color};"` : 'style="border-left: 4px solid #007bff;"'}>
                    <div style="position: relative;">
                        ${buttonHtml}
                        <p class="tray-head">${trayDisplayName}</p>
                        ${typeWithColor}
                        ${trayDetails}
                        ${tempHTML}
                        ${(ams.ams_id === 255 && tray.tray_type !== '') ? outButtonHtml : ''}
                        ${(tray.setting_id != "" && tray.setting_id != "null") ? spoolmanButtonHtml : ''}
                    </div>
                    
                </div>`;
        }).join('');

        const amsInfo = `
            <div class="feature">
                <h3>${amsDisplayName}:</h3>
                <div id="trayContainer">
                    ${trayHTML}
                </div>
            </div>`;
        
        amsDataContainer.innerHTML += amsInfo;
    });
}

// Function to show/hide spool buttons
function updateSpoolButtons(show) {
    const spoolButtons = document.querySelectorAll('.spool-button');
    spoolButtons.forEach(button => {
        button.style.display = show ? 'block' : 'none';
    });
}

function handleSpoolmanSettings(tray_info_idx, setting_id, cali_idx, nozzle_temp_min, nozzle_temp_max) {
    // Get the selected filament
    const selectedText = document.getElementById("selected-filament").textContent;

    // Find the selected spool in the data
    const selectedSpool = spoolsData.find(spool => 
        `${spool.id} | ${spool.filament.name} (${spool.filament.material})` === selectedText
    );

    const payload = {
        type: 'setSpoolmanSettings',
        payload: {
            filament_id: selectedSpool.filament.id,
            tray_info_idx: tray_info_idx,
            setting_id: setting_id,
            cali_idx: cali_idx,
            temp_min: nozzle_temp_min,
            temp_max: nozzle_temp_max
        }
    };

    try {
        socket.send(JSON.stringify(payload));
        showNotification(`Setting send to Spoolman`, true);
    } catch (error) {
        console.error("Error while sending settings to Spoolman:", error);
        showNotification("Error while sending!", false);
    }
}

function handleSpoolOut() {
    // Create payload
    const payload = {
        type: 'setBambuSpool',
        payload: {
            amsId: 255,
            trayId: 254,
            color: "FFFFFF",
            nozzle_temp_min: 0,
            nozzle_temp_max: 0,
            type: "",
            brand: ""
        }
    };

    try {
        socket.send(JSON.stringify(payload));
        showNotification(`External Spool removed. Pls wait`, true);
    } catch (error) {
        console.error("Error sending WebSocket message:", error);
        showNotification("Error while sending!", false);
    }
}

// Function to handle spool-in click
function handleSpoolIn(amsId, trayId) {
    // Check WebSocket connection first
    if (!socket || socket.readyState !== WebSocket.OPEN) {
        showNotification("No active WebSocket connection!", false);
        console.error("WebSocket not connected");
        return;
    }

    // Get the selected filament
    const selectedText = document.getElementById("selected-filament").textContent;
    if (selectedText === "Please choose...") {
        showNotification("Choose Filament first", false);
        return;
    }

    // Find the selected spool in the data
    const selectedSpool = spoolsData.find(spool => 
        `${spool.id} | ${spool.filament.name} (${spool.filament.material})` === selectedText
    );

    if (!selectedSpool) {
        showNotification("Selected Spool not found", false);
        return;
    }

    // Extract temperature values
    let minTemp = "175";
    let maxTemp = "275";

    if (Array.isArray(selectedSpool.filament.nozzle_temperature) && 
        selectedSpool.filament.nozzle_temperature.length >= 2) {
        minTemp = selectedSpool.filament.nozzle_temperature[0];
        maxTemp = selectedSpool.filament.nozzle_temperature[1];
    }

    // Create payload
    const payload = {
        type: 'setBambuSpool',
        payload: {
            amsId: amsId,
            trayId: trayId,
            color: selectedSpool.filament.color_hex || "FFFFFF",
            nozzle_temp_min: parseInt(minTemp),
            nozzle_temp_max: parseInt(maxTemp),
            type: selectedSpool.filament.material,
            brand: selectedSpool.filament.vendor.name,
            tray_info_idx: selectedSpool.filament.extra.bambu_idx?.replace(/['"]+/g, '').trim() || '',
            cali_idx: "-1"  // Set default value
        }
    };

    // Check if key cali_idx is present and set it
    if (selectedSpool.filament.extra.bambu_cali_id) {
        payload.payload.cali_idx = selectedSpool.filament.extra.bambu_cali_id.replace(/['"]+/g, '').trim();
    }

    // Check if key bambu_setting_id is present
    if (selectedSpool.filament.extra.bambu_setting_id) {
        payload.payload.bambu_setting_id = selectedSpool.filament.extra.bambu_setting_id.replace(/['"]+/g, '').trim();
    }

    console.log("Spool-In Payload:", payload);

    try {
        socket.send(JSON.stringify(payload));
        showNotification(`Spool set in AMS ${amsId} Tray ${trayId}. Pls wait`, true);
    } catch (error) {
        console.error("Error sending WebSocket message:", error);
        showNotification("Error while sending", false);
    }
}

function updateNfcStatusIndicator(data) {
    const indicator = document.getElementById('nfcStatusIndicator');
    
    if (data.found === 0) {
        // No NFC tag found
        indicator.className = 'status-circle';
        spoolDetected = false;
    } else if (data.found === 1) {
        // NFC tag read successfully
        indicator.className = 'status-circle success';
        spoolDetected = true;
    } else {
        // Error reading tag
        indicator.className = 'status-circle error';
        spoolDetected = true;
    }
}

function updateNfcData(data) {
    // Find the container for NFC status
    const nfcStatusContainer = document.querySelector('.nfc-status-display');
    
    // Remove existing data display if present
    const existingData = nfcStatusContainer.querySelector('.nfc-data');
    if (existingData) {
        existingData.remove();
    }

    // Create new div for data display
    const nfcDataDiv = document.createElement('div');
    nfcDataDiv.className = 'nfc-data';

    // If an error exists or no data is present
    if (data.error || data.info || !data || Object.keys(data).length === 0) {
        // Show error message or empty message
        if (data.error || data.info) {
            if (data.error) {
                nfcDataDiv.innerHTML = `
                    <div class="error-message" style="margin-top: 10px; color: #dc3545;">
                        <p><strong>Error:</strong> ${data.error}</p>
                    </div>`;
            } else {
                nfcDataDiv.innerHTML = `
                    <div class="info-message" style="margin-top: 10px; color:rgb(18, 210, 0);">
                        <p><strong>Info:</strong> ${data.info}</p>
                    </div>`;
            }

        } else {
            nfcDataDiv.innerHTML = '<div class="info-message-inner" style="margin-top: 10px;"></div>';
        }
        nfcStatusContainer.appendChild(nfcDataDiv);
        return;
    }

    // Create HTML for data display
    let html = "";

    if(data.sm_id){
        html = `
        <div class="nfc-card-data" style="margin-top: 10px;">
            <p><strong>Brand:</strong> ${data.brand || 'N/A'}</p>
            <p><strong>Type:</strong> ${data.type || 'N/A'} ${data.color_hex ? `<span style="
                background-color: #${data.color_hex}; 
                width: 20px; 
                height: 20px; 
                display: inline-block; 
                vertical-align: middle;
                border: 1px solid #333;
                border-radius: 3px;
                margin-left: 5px;
            "></span>` : ''}</p>
        `;

        // Display Spoolman ID
        html += `<p><strong>Spoolman ID:</strong> ${data.sm_id} (<a href="${spoolmanUrl}/spool/show/${data.sm_id}">Open in Spoolman</a>)</p>`;
     }
     else if(data.location)
     {
        html = `
        <div class="nfc-card-data" style="margin-top: 10px;">
            <p><strong>Location:</strong> ${data.location || 'N/A'}</p>
        `;
     }
     else if(data.format === 'openprinttag')
     {
        // OpenPrintTag format display
        html = `
        <div class="nfc-card-data" style="margin-top: 10px;">
            <p><strong>Format:</strong> OpenPrintTag</p>
            <p><strong>Brand:</strong> ${data.brand_name || 'N/A'}</p>
            <p><strong>Material:</strong> ${data.material_type || 'N/A'} ${data.color_hex ? `<span style="
                background-color: #${data.color_hex}; 
                width: 20px; 
                height: 20px; 
                display: inline-block; 
                vertical-align: middle;
                border: 1px solid #333;
                border-radius: 3px;
                margin-left: 5px;
            "></span>` : ''}</p>
        `;
        if (data.material_name) html += `<p><strong>Name:</strong> ${data.material_name}</p>`;
        if (data.min_print_temp >= 0 && data.max_print_temp >= 0) {
            html += `<p><strong>Print Temp:</strong> ${data.min_print_temp}°C - ${data.max_print_temp}°C</p>`;
        }
        if (data.min_bed_temp >= 0 && data.max_bed_temp >= 0) {
            html += `<p><strong>Bed Temp:</strong> ${data.min_bed_temp}°C - ${data.max_bed_temp}°C</p>`;
        }
        if (data.nominal_weight_g > 0) html += `<p><strong>Weight:</strong> ${data.nominal_weight_g}g</p>`;
        if (data.density > 0) html += `<p><strong>Density:</strong> ${data.density} g/cm³</p>`;
        if (data.drying_temp >= 0) html += `<p><strong>Drying:</strong> ${data.drying_temp}°C / ${data.drying_time_min || '?'} min</p>`;
        if (data.instance_uuid) html += `<p><strong>UUID:</strong> <small>${data.instance_uuid}</small></p>`;
     }
     else
     {
        html = `
        <div class="nfc-card-data" style="margin-top: 10px;">
            <p><strong>Unknown tag</strong></p>
        `;
     }

    

    // Only update dropdowns when a sm_id is present
    if (data.sm_id) {
        const matchingSpool = spoolsData.find(spool => spool.id === parseInt(data.sm_id));
        if (matchingSpool) {
            // First update vendor dropdown
            document.getElementById("vendorSelect").value = matchingSpool.filament.vendor.id;
            
            // Then update filament dropdown and select spool
            updateFilamentDropdown();
            setTimeout(() => {
                // Wait briefly until dropdown is updated
                selectFilament(matchingSpool);
            }, 100);
        }
    }

    html += '</div>';
    nfcDataDiv.innerHTML = html;

    
    // Add new div to container
    nfcStatusContainer.appendChild(nfcDataDiv);
}

function writeNfcTag() {
    if(!spoolDetected || confirm("Are you sure you want to overwrite the Tag?") == true){
        const selectedText = document.getElementById("selected-filament").textContent;
        if (selectedText === "Please choose...") {
            alert('Please select a Spool first.');
            return;
        }

        const spoolsData = window.getSpoolData();
        const selectedSpool = spoolsData.find(spool => 
            `${spool.id} | ${spool.filament.name} (${spool.filament.material})` === selectedText
        );

        if (!selectedSpool) {
            alert('Selected spool could not be found.');
            return;
        }

        // Extract temperature values correctly
        let minTemp = "175";
        let maxTemp = "275";
        
        if (Array.isArray(selectedSpool.filament.nozzle_temperature) && 
            selectedSpool.filament.nozzle_temperature.length >= 2) {
            minTemp = String(selectedSpool.filament.nozzle_temperature[0]);
            maxTemp = String(selectedSpool.filament.nozzle_temperature[1]);
        }

        // Create NFC data packet with correct data types
        const nfcData = {
            color_hex: selectedSpool.filament.color_hex || "FFFFFF",
            type: selectedSpool.filament.material,
            min_temp: minTemp,
            max_temp: maxTemp,
            brand: selectedSpool.filament.vendor.name,
            sm_id: String(selectedSpool.id) // Convert to string
        };

        if (socket?.readyState === WebSocket.OPEN) {
            const writeButton = document.getElementById("writeNfcButton");
            writeButton.classList.add("writing");
            writeButton.textContent = "Writing";
            socket.send(JSON.stringify({
                type: 'writeNfcTag',
                tagType: 'spool',
                payload: nfcData
            }));
        } else {
            alert('Not connected to Server. Please check connection.');
        }
    }
}

function writeLocationNfcTag() {
    if(!spoolDetected || confirm("Are you sure you want to overwrite the Tag?") == true){
        const selectedText = document.getElementById("locationSelect").value;
        if (selectedText === "Please choose...") {
            alert('Please select a location first.');
            return;
        }
        // Create NFC data packet with correct data types
        const nfcData = {
            location: String(selectedText)
        };


        if (socket?.readyState === WebSocket.OPEN) {
            const writeButton = document.getElementById("writeLocationNfcButton");
            writeButton.classList.add("writing");
            writeButton.textContent = "Writing";
            socket.send(JSON.stringify({
                type: 'writeNfcTag',
                tagType: 'location',
                payload: nfcData
            }));
        } else {
            alert('Not connected to Server. Please check connection.');
        }
    }
}

function handleWriteNfcTagResponse(success) {
    const writeButton = document.getElementById("writeNfcButton");
    const writeLocationButton = document.getElementById("writeLocationNfcButton");
    if(writeButton.classList.contains("writing")){
        writeButton.classList.remove("writing");
        writeButton.classList.add(success ? "success" : "error");
        writeButton.textContent = success ? "Write success" : "Write failed";

        setTimeout(() => {
            writeButton.classList.remove("success", "error");
            writeButton.textContent = "Write Tag";
        }, 5000);
    }

    if(writeLocationButton.classList.contains("writing")){
        writeLocationButton.classList.remove("writing");
        writeLocationButton.classList.add(success ? "success" : "error");
        writeLocationButton.textContent = success ? "Write success" : "Write failed";

        setTimeout(() => {
            writeLocationButton.classList.remove("success", "error");
            writeLocationButton.textContent = "Write Location Tag";
        }, 5000);
    }

    
}

function showNotification(message, isSuccess) {
    const notification = document.createElement('div');
    notification.className = `notification ${isSuccess ? 'success' : 'error'}`;
    notification.textContent = message;
    document.body.appendChild(notification);

    // Fade out after 3 seconds
    setTimeout(() => {
        notification.classList.add('fade-out');
        setTimeout(() => {
            notification.remove();
        }, 300);
    }, 3000);
}