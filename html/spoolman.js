// Global variables
let spoolmanUrl = '';
let spoolsData = [];
let locationData = [];

// Helper functions for data manipulation
function processSpoolData(data) {
    return data.map(spool => ({
        id: spool.id,
        remaining_weight: spool.remaining_weight,
        remaining_length: spool.remaining_length,
        filament: spool.filament,
        extra: spool.extra
    }));
}

// Dropdown functions
function populateVendorDropdown(data, selectedSmId = null) {
    const vendorSelect = document.getElementById("vendorSelect");
    if (!vendorSelect) {
        console.error('vendorSelect element not found');
        return;
    }
    const onlyWithoutSmId = document.getElementById("onlyWithoutSmId");
    if (!onlyWithoutSmId) {
        console.error('onlyWithoutSmId element not found');
        return;
    }

    // Separate objects for all vendors and filtered vendors
    const allVendors = {};
    const filteredVendors = {};

    vendorSelect.innerHTML = '<option value="">Please select...</option>';

    let vendorIdToSelect = null;
    let totalSpools = 0;
    let spoolsWithoutTag = 0;
    let totalWeight = 0;
    let totalLength = 0;
    // New object for material grouping
    const materials = {};

    data.forEach(spool => {
        if (!spool.filament || !spool.filament.vendor) {
            return;
        }

        totalSpools++;
        
        // Count and group materials
        if (spool.filament.material) {
            const material = spool.filament.material.toUpperCase(); // Normalization
            materials[material] = (materials[material] || 0) + 1;
        }

        // Add weight and length
        if (spool.remaining_weight) {
            totalWeight += spool.remaining_weight;
        }
        if (spool.remaining_length) {
            totalLength += spool.remaining_length;
        }

        const vendor = spool.filament.vendor;
        
        const hasValidNfcId = spool.extra && 
                             spool.extra.nfc_id && 
                             spool.extra.nfc_id !== '""' && 
                             spool.extra.nfc_id !== '"\\"\\"\\""';
        
        if (!hasValidNfcId) {
            spoolsWithoutTag++;
        }

        // Collect all vendors
        if (!allVendors[vendor.id]) {
            allVendors[vendor.id] = vendor.name;
        }

        // Filtered vendors for dropdown
        if (!filteredVendors[vendor.id]) {
            if (!onlyWithoutSmId.checked || !hasValidNfcId) {
                filteredVendors[vendor.id] = vendor.name;
            }
        }
    });

    // After loop: format total length
    const lengthInM = totalLength / 1000;  // convert to m first
    const formattedLength = lengthInM > 1000 
        ? (lengthInM / 1000).toFixed(2) + " km" 
        : lengthInM.toFixed(2) + " m";

    // Format total weight (from g to kg to t)
    const weightInKg = totalWeight / 1000;  // convert to kg first
    const formattedWeight = weightInKg > 1000 
        ? (weightInKg / 1000).toFixed(2) + " t" 
        : weightInKg.toFixed(2) + " kg";

    // Fill dropdown with filtered vendors - alphabetically sorted
    Object.entries(filteredVendors)
        .sort(([, nameA], [, nameB]) => nameA.localeCompare(nameB)) // Sort vendors alphabetically by name
        .forEach(([id, name]) => {
            const option = document.createElement("option");
            option.value = id;
            option.textContent = name;
            vendorSelect.appendChild(option);
        });

    document.getElementById("totalSpools").textContent = totalSpools;
    document.getElementById("spoolsWithoutTag").textContent = spoolsWithoutTag;
    // Show total number of all vendors
    document.getElementById("totalVendors").textContent = Object.keys(allVendors).length;
    
    // Add new statistics
    document.getElementById("totalWeight").textContent = formattedWeight;
    document.getElementById("totalLength").textContent = formattedLength;

    // Add material statistics to DOM
    const materialsList = document.getElementById("materialsList");
    materialsList.innerHTML = '';
    Object.entries(materials)
        .sort(([,a], [,b]) => b - a) // Sort by count descending
        .forEach(([material, count]) => {
            const li = document.createElement("li");
            li.textContent = `${material}: ${count} ${count === 1 ? 'Spool' : 'Spools'}`;
            materialsList.appendChild(li);
        });

    if (vendorIdToSelect) {
        vendorSelect.value = vendorIdToSelect;
        updateFilamentDropdown(selectedSmId);
    }
}

// Dropdown functions
function populateLocationDropdown(data) {
    const locationSelect = document.getElementById("locationSelect");
    if (!locationSelect) {
        console.error('locationSelect element not found');
        return;
    }

    locationSelect.innerHTML = '<option value="">Please select...</option>';
    // Fill dropdown with filtered vendors - alphabetically sorted
    Object.entries(data)
        .sort(([, nameA], [, nameB]) => nameA.localeCompare(nameB)) // Sort vendors alphabetically by name
        .forEach(([id, name]) => {
            const option = document.createElement("option");
            option.value = name;
            option.textContent = name;
            locationSelect.appendChild(option);
        });
}

function updateFilamentDropdown(selectedSmId = null) {
    const vendorId = document.getElementById("vendorSelect").value;
    const dropdownContentInner = document.getElementById("filament-dropdown-content");
    const filamentSection = document.getElementById("filamentSection");
    const onlyWithoutSmId = document.getElementById("onlyWithoutSmId").checked;
    const selectedText = document.getElementById("selected-filament");
    const selectedColor = document.getElementById("selected-color");

    dropdownContentInner.innerHTML = '';
    selectedText.textContent = "Please select...";
    selectedColor.style.backgroundColor = '#FFFFFF';

    if (vendorId) {
        const filteredFilaments = spoolsData.filter(spool => {
            if (!spool?.filament?.vendor?.id) {
                console.log('Problem occurred with: ', spool?.filament?.vendor);
                console.log('Problematic spools:', 
                    spoolsData.filter(spool => !spool?.filament?.vendor?.id));
                return false;
            }

            const hasValidNfcId = spool.extra && 
                                 spool.extra.nfc_id && 
                                 spool.extra.nfc_id !== '""' && 
                                 spool.extra.nfc_id !== '"\\"\\"\\""';
            
            return spool.filament.vendor.id == vendorId && 
                   (!onlyWithoutSmId || !hasValidNfcId);
        });

        filteredFilaments.forEach(spool => {
            const option = document.createElement("div");
            option.className = "dropdown-option";
            option.setAttribute("data-value", spool.filament.id);
            option.setAttribute("data-nfc-id", spool.extra.nfc_id || "");
            

            // Generate color representation based on filament type (single or multi color)
            let colorHTML = '';
            
            // Check if this is a multicolor filament
            if (spool.filament.multi_color_hexes) {
                // Parse multi color hexes from comma-separated string
                const colors = spool.filament.multi_color_hexes.replace(/#/g, '').split(',');
                
                // Determine the display style based on direction
                const direction = spool.filament.multi_color_direction || 'coaxial';
                
                // Generate color circles for each color
                colorHTML = '<div class="option-colors">';
                colors.forEach(color => {
                    colorHTML += `<div class="option-color multi-color ${direction}" style="background-color: #${color}"></div>`;
                });
                colorHTML += '</div>';
            } else {
                // Single color filament
                const colorHex = spool.filament.color_hex || 'FFFFFF';
                colorHTML = `<div class="option-color" style="background-color: #${colorHex}"></div>`;
            }
            
            option.innerHTML = `
                ${colorHTML}
                <span>${spool.id} | ${spool.filament.name} (${spool.filament.material})</span>
            `;
            
            option.onclick = () => selectFilament(spool);
            dropdownContentInner.appendChild(option);
        });

        filamentSection.classList.remove("hidden");
    } else {
        filamentSection.classList.add("hidden");
    }
}

function updateLocationSelect(){
    const writeLocationNfcButton = document.getElementById('writeLocationNfcButton');
    if(writeLocationNfcButton){
        writeLocationNfcButton.classList.remove("hidden");
    }
}

function selectFilament(spool) {
    const selectedColor = document.getElementById("selected-color");
    const selectedText = document.getElementById("selected-filament");
    const dropdownContent = document.getElementById("filament-dropdown-content");
    
    // Update the selected color display
    if (spool.filament.multi_color_hexes) {
        // Handle multicolor filament display in the selection header
        const colors = spool.filament.multi_color_hexes.replace(/#/g, '').split(',');
        const direction = spool.filament.multi_color_direction || 'coaxial';
        
        // Replace the single color div with multiple color divs
        selectedColor.innerHTML = '';
        colors.forEach(color => {
            const colorDiv = document.createElement('div');
            colorDiv.className = `color-segment multi-color ${direction}`;
            colorDiv.style.backgroundColor = `#${color}`;
            selectedColor.appendChild(colorDiv);
        });
        // Add multiple color class to the container
        selectedColor.classList.add('multi-color-container');
    } else {
        // Single color filament - reset to default display
        selectedColor.innerHTML = '';
        selectedColor.classList.remove('multi-color-container');
        selectedColor.style.backgroundColor = `#${spool.filament.color_hex || 'FFFFFF'}`;
    }
    
    selectedText.textContent = `${spool.id} | ${spool.filament.name} (${spool.filament.material})`;
    dropdownContent.classList.remove("show");
    
    document.dispatchEvent(new CustomEvent('filamentSelected', { 
        detail: spool 
    }));
}

// Initialization and event handlers
async function initSpoolman() {
    try {
        const response = await fetch('/api/url');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        if (!data.spoolman_url) {
            throw new Error('spoolman_url not found in response');
        }
        
        spoolmanUrl = data.spoolman_url;
        
        const fetchedData = await fetchSpoolData();
        spoolsData = processSpoolData(fetchedData);

        document.dispatchEvent(new CustomEvent('spoolDataLoaded', { 
            detail: spoolsData 
        }));
        
        locationData = await fetchLocationData();
        
        document.dispatchEvent(new CustomEvent('locationDataLoaded', { 
            detail: locationData 
        }));


    } catch (error) {
        console.error('Error initializing Spoolman:', error);
        document.dispatchEvent(new CustomEvent('spoolmanError', { 
            detail: { message: error.message } 
        }));
    }
}

async function fetchSpoolData() {
    try {
        if (!spoolmanUrl) {
            throw new Error('Spoolman URL is not initialized');
        }
        
        const response = await fetch(`${spoolmanUrl}/api/v1/spool`);
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        return data;
    } catch (error) {
        console.error('Error fetching spool data:', error);
        return [];
    }
}

async function fetchLocationData() {
    try {
        if (!spoolmanUrl) {
            throw new Error('Spoolman URL is not initialized');
        }
        
        const response = await fetch(`${spoolmanUrl}/api/v1/location`);
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        return data;
    } catch (error) {
        console.error('Error fetching location data:', error);
        return [];
    }
}

// Event Listener
document.addEventListener('DOMContentLoaded', () => {
    initSpoolman();
    
    const vendorSelect = document.getElementById('vendorSelect');
    if (vendorSelect) {
        vendorSelect.addEventListener('change', () => updateFilamentDropdown());
    }

    const locationSelect = document.getElementById('locationSelect');
    if (locationSelect) {
        locationSelect.addEventListener('change', () => updateLocationSelect());
    }
    
    const onlyWithoutSmId = document.getElementById('onlyWithoutSmId');
    if (onlyWithoutSmId) {
        onlyWithoutSmId.addEventListener('change', () => {
            populateVendorDropdown(spoolsData);
            updateFilamentDropdown();
        });
    }
    
    document.addEventListener('spoolDataLoaded', (event) => {
        populateVendorDropdown(event.detail);
    });

    document.addEventListener('locationDataLoaded', (event) => {
        populateLocationDropdown(event.detail);
    });
    
    window.onclick = function(event) {
        if (!event.target.closest('.custom-dropdown')) {
            const dropdowns = document.getElementsByClassName("dropdown-content");
            for (let dropdown of dropdowns) {
                dropdown.classList.remove("show");
            }
        }
    };

    const refreshButton = document.getElementById('refreshSpoolman');
    if (refreshButton) {
        refreshButton.addEventListener('click', async () => {
            try {
                refreshButton.disabled = true;
                refreshButton.textContent = 'Updating...';
                await initSpoolman();
                refreshButton.textContent = 'Refresh Spoolman';
            } finally {
                refreshButton.disabled = false;
            }
        });
    }
});

// Export functions
window.getSpoolData = () => spoolsData;
window.setSpoolData = (data) => { spoolsData = data; };
window.reloadSpoolData = initSpoolman;
window.populateVendorDropdown = populateVendorDropdown;
window.populateLocationDropdown = populateLocationDropdown;
window.updateFilamentDropdown = updateFilamentDropdown;
window.toggleFilamentDropdown = () => {
    const content = document.getElementById("filament-dropdown-content");
    content.classList.toggle("show");
};

// Event listener for click outside dropdown
window.onclick = function(event) {
    if (!event.target.closest('.custom-dropdown')) {
        const dropdowns = document.getElementsByClassName("dropdown-content");
        for (let dropdown of dropdowns) {
            dropdown.classList.remove("show");
        }
    }
};