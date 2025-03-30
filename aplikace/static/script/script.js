/************************************
 * Pomocné funkce
 ************************************/

/**
 * Obecná pomocná funkce pro fetch požadavky, která zpracuje HTTP chyby a vrátí JSON.
 * @param {string} url - URL požadavku.
 * @param {object} options - Volitelné nastavení fetch.
 * @returns {Promise<any>} - Promise s JSON odpovědí.
 */
const performFetch = (url, options = {}) =>
  fetch(url, options)
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      return response.json();
    });

/**
 * Aktualizuje text elementu s daným ID.
 * @param {string} id - ID elementu.
 * @param {string|number} value - Hodnota, která se má zobrazit.
 * @param {string} [unit=''] - Volitelná jednotka, která se připojí za hodnotu.
 */
const updateElement = (id, value, unit = '') => {
  const element = document.getElementById(id);
  if (element) {
    element.innerText = `${value}${unit}`;
  }
};

/**
 * Aktualizuje stav elementu (text, třídy) a volitelně zakáže či povolí input.
 * @param {string} elementId - ID elementu, který se aktualizuje.
 * @param {string} text - Text, který se má zobrazit.
 * @param {string|Array} addClass - Třída/třídy, které se mají přidat.
 * @param {string|Array} removeClass - Třída/třídy, které se mají odebrat.
 * @param {string|null} inputId - (Volitelně) ID input elementu.
 * @param {boolean} [disableInput=false] - Pokud true, input bude zakázán.
 */
const updateStatus = (elementId, text, addClass, removeClass, inputId = null, disableInput = false) => {
  const element = document.getElementById(elementId);
  if (element) {
    element.innerText = text;
    element.classList.remove(...(Array.isArray(removeClass) ? removeClass : [removeClass]));
    element.classList.add(...(Array.isArray(addClass) ? addClass : [addClass]));
  }
  if (inputId) {
    const input = document.getElementById(inputId);
    if (input) input.disabled = disableInput;
  }
};

/****************************************
 * Update dat, stavu a ovládacích prvků
 ****************************************/

/**
 * Načte data ze serveru a aktualizuje příslušné elementy.
 */
const updateData = () => {
  performFetch('/data')
    .then(data => {
      updateElement('temperature', data.temperature, ' °C');
      updateElement('humidity', data.humidity, ' %');
      updateElement('temperature-bmp', data.temperaturebmp, ' °C');
      updateElement('pressure', data.pressure, ' hPa');
      updateElement('water_temperature', data.water_temperature, ' °C');
      updateElement('ph', data.ph);
      updateElement('tds', data.tds, ' ppm');
    })
    .catch(error => console.error('Chyba při aktualizaci dat:', error));
};

/**
 * Načte stav ovládacích zařízení a aktualizuje checkboxy, stavy a další související elementy.
 */
const updateControlState = () => {
  performFetch('/control/state')
    .then(data => {
      console.log("Aktuální stav z Flasku:", data);
      ['led', 'outfan', 'pump', 'infan'].forEach(device => {
        updateCheckbox(device, data[device]);
      });

      if (!(data.outfan_setting && data.outfan_setting !== "none")) {
        updateCheckbox('outfan', data.outfan);
      }
      if (!(data.infan_interval && data.infan_interval !== "none")) {
        updateCheckbox('infan', data.infan);
      }
      updateScheduleStatus(data);
      updatePHStatus(data.ph);
      updateInfanIntervalStatus(data.infan_interval);
      updateTDSStatus(data.tds);
      updateOutfanSettingsState();
    })
    .catch(error => console.error('Chyba při načítání stavu ovládání:', error));
};

/**
 * Aktualizuje checkbox, příslušný label a indikátor pro zařízení.
 * @param {string} device - Název zařízení (ID checkboxu).
 * @param {string} state - Stav zařízení ("ZAPNUTO" nebo "VYPNUTO").
 */
const updateCheckbox = (device, state) => {
  const checkbox = document.getElementById(device);
  const label = document.getElementById(`${device}_status`);
  const indicator = document.getElementById(`${device}_indicator`);
  const isOn = (state === "ZAPNUTO");

  if (checkbox) checkbox.checked = isOn;
  if (label) label.innerText = state;
  if (indicator) {
    indicator.classList.toggle('on', isOn);
    indicator.classList.toggle('off', !isOn);
  }
};

/************************************
 * Zobrazování stavu nastavení
 ************************************/

/**
 * Aktualizuje stav plánu LED osvětlení.
 * @param {object} data - Data obsahující nastavení plánu.
 */
const updateScheduleStatus = data => {
  const isSet = data.schedule && data.schedule !== "none" && data.schedule !== "delete";
  const scheduleText = isSet ? `Plán je nastaven ${data.schedule}` : "Plán není nastaven";

  updateStatus('scheduleStatus', scheduleText, isSet ? 'schedule-set' : 'schedule-not-set', isSet ? 'schedule-not-set' : 'schedule-set');

  const ledCheckbox = document.getElementById('led');
  if (ledCheckbox) {

    ledCheckbox.disabled = isSet;
    if (!isSet) {
      ledCheckbox.checked = false;

      performFetch('/control', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ device: "led", state: "VYPNUTO" })
      })
      .then(resp => console.log("LED nastavení bylo změněno:", resp))
      .catch(error => console.error("Chyba při odesílání zprávy o změně nastavení LED:", error));
    }
  }
};

/**
 * Aktualizuje stav pH nastavení.
 * @param {string} phData - Data pH, která mohou obsahovat dvě hodnoty oddělené ampersandem.
 */
const updatePHStatus = phData => {
  const phStatusElement = document.getElementById('phStatus');
  const phMaxSlider = document.getElementById('ph_max_value');
  const phMinSlider = document.getElementById('ph_min_value');
  const phMaxLabel = document.getElementById('ph_max_value_label');
  const phMinLabel = document.getElementById('ph_min_value_label');
  let text = "";

  if (phData && phData !== "none") {
    const parts = phData.split('&');
    if (parts.length === 2) {
      const phMin = parts[0].trim();
      const phMax = parts[1].trim();
      text = `pH nastaveno: min = ${phMin}, max = ${phMax}`;
      phMinSlider.value = phMin;
      phMaxSlider.value = phMax;
      phMinLabel.textContent = phMin;
      phMaxLabel.textContent = phMax;
      phMinSlider.disabled = true;
      phMaxSlider.disabled = true;
    } else {
      text = `pH nastaveno: ${phData}`;
    }
    updateStatus('phStatus', text, 'infan-set', 'infan-not-set');
  } else {
    updateStatus('phStatus', "pH není nastaveno", 'infan-not-set', 'infan-set');
    phMinSlider.disabled = false;
    phMaxSlider.disabled = false;
  }
};

/**
 * Aktualizuje zobrazení intervalu pro vnitřní větrák.
 * @param {string} intervalData - Nastavený interval nebo "none".
 */
const updateInfanIntervalStatus = intervalData => {
  const text = (intervalData && intervalData !== "none")
    ? `Interval vnitřního větráku: ${intervalData} min/hod`
    : "Interval vnitřního větráku není nastaven";
  updateStatus('infanIntervalStatus', text,
    (intervalData && intervalData !== "none") ? 'infan-set' : 'infan-not-set',
    (intervalData && intervalData !== "none") ? 'infan-not-set' : 'infan-set',
    'infan', (intervalData && intervalData !== "none"));
};

/**
 * Aktualizuje zobrazení minimální hodnoty TDS.
 * @param {string} tdsSetting - Nastavená minimální hodnota TDS nebo "none".
 */
const updateTDSStatus = tdsSetting => {
  const text = (tdsSetting && tdsSetting !== "none")
    ? `Minimální TDS nastaveno: ${tdsSetting}`
    : "Minimální TDS není nastaveno";
  updateStatus('tdsStatus', text,
    (tdsSetting && tdsSetting !== "none") ? 'infan-set' : 'infan-not-set',
    (tdsSetting && tdsSetting !== "none") ? 'infan-not-set' : 'infan-set',
    'tds_min_value', (tdsSetting && tdsSetting !== "none"));
};

/************************************
 * Akční funkce
 ************************************/

/**
 * Nastaví interval pro vnitřní větrák pomocí POST požadavku.
 */
const setupInfanInterval = () => {
  const intervalMinutes = document.getElementById('infan_interval_minutes').value;
  const infanCheckBox = document.getElementById('infan');

  performFetch('/infan/interval', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ interval_minutes: intervalMinutes })
  })
    .then(data => {
      if (data.status === 'success') {
        updateControlState();
        const statusText = `Nastaveno na ${intervalMinutes} minut každou hodinu`;
        updateElement('infanIntervalStatus', statusText);
        updateStatus('infanIntervalStatus', statusText, 'infan-set', 'infan-not-set', 'infan', true);
      } else {
        updateStatus('infanIntervalStatus', 'Není nastaveno', 'infan-not-set', 'infan-set', 'infan', false);
      }
    })
    .catch(error => console.error('Chyba:', error));
};

/**
 * Smaže nastavený interval vnitřního větráku a případně vypne zařízení.
 */
const deleteInfanInterval = () => {
  performFetch('/infan/interval/delete', { method: 'POST' })
    .then(data => {
      if (data.status === 'success') {
        updateInfanIntervalStatus("none");
        document.getElementById('infan_interval_minutes').value = 'Počet minut z hodiny';

        const infanCheckbox = document.getElementById('infan');
        if (infanCheckbox && infanCheckbox.checked) {
          infanCheckbox.checked = false;
          performFetch('/control', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ device: "infan", state: "VYPNUTO" })
          })
            .then(resp => console.log("Infan byl vypnut:", resp))
            .catch(error => console.error('Chyba při vypínání infan:', error));
        }
      }
    })
    .catch(error => console.error('Chyba:', error));
};

/**
 * Přepne stav daného zařízení.
 * @param {string} device - Název zařízení (ID checkboxu).
 */
const toggleDevice = device => {
  const checkbox = document.getElementById(device);
  const state = checkbox.checked ? "ZAPNUTO" : "VYPNUTO";
  document.getElementById(`${device}_status`).innerText = state;

  performFetch('/control', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ device, state })
  })
    .then(data => console.log("Odpověď z Flasku:", data))
    .catch(error => console.error('Error:', error));
};

/**
 * Aktualizuje stav nastavení vnějšího větráku.
 */
const updateOutfanSettingsState = () => {
  performFetch('/outfan/settings/state')
    .then(data => {
      const isSet = data.outfan_setting && data.outfan_setting !== "none";
      let text = "";
      if (isSet) {
        const tempMatch = data.outfan_setting.match(/T\s*=\s*([\d.]+)/);
        const humMatch = data.outfan_setting.match(/H\s*=\s*([\d.]+)/);
        const temperature = tempMatch ? tempMatch[1] : "";
        const humidity = humMatch ? humMatch[1] : "";

        if (temperature && humidity) {
          text = `Mezní hodnoty:\nTeplota = ${temperature}°C a Vlhkost = ${humidity}%`;
        } else if (temperature) {
          text = `Mezní hodnoty:\nTeplota = ${temperature}°C`;
        } else if (humidity) {
          text = `Mezní hodnoty:\nVlhkost = ${humidity}%`;
        } else {
          text = "Mezní hodnoty nastaveny";
        }
        updateStatus('outfanSettingsStatus', text, 'outfan-set', 'outfan-not-set');
        const outfanCheckbox = document.getElementById('outfan');
        if (outfanCheckbox) {
          outfanCheckbox.checked = true;
          outfanCheckbox.disabled = true;
        }
      } else {
        updateStatus('outfanSettingsStatus', "Nejsou nastaveny mezní hodnoty", 'outfan-not-set', 'outfan-set');
        const outfanCheckbox = document.getElementById('outfan');
        if (outfanCheckbox) {
          outfanCheckbox.disabled = false;
          document.getElementById('outfan_temperature').value = '';
          document.getElementById('outfan_humidity').value = '';
        }
      }
    })
    .catch(error => console.error('Chyba:', error));
};

/************************************
 * Inicializace Event Listenerů
 ************************************/

const initEventListeners = () => {
  // Plán LED osvětlení - nastavení a smazání
  document.getElementById('scheduleForm').addEventListener('submit', event => {
    event.preventDefault();
    const start_time = document.getElementById('start_time').value;
    const end_time = document.getElementById('end_time').value;

    performFetch('/schedule', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ start_time, end_time })
    })
      .then(data => updateScheduleStatus(data))
      .catch(error => {
        console.error('Chyba:', error);
        updateElement('scheduleStatus', "Chyba při komunikaci se serverem");
      });
  });

  document.getElementById('deleteScheduleBtn').addEventListener('click', () => {
    performFetch('/schedule/delete', { method: 'POST' })
      .then(data => {
        if (data.status === "success") {
          updateElement('scheduleStatus', "Plán není nastaven");
          document.getElementById('start_time').value = '';
          document.getElementById('end_time').value = '';
          const ledCheckbox = document.getElementById('led');
          if (ledCheckbox && ledCheckbox.checked) {
            ledCheckbox.checked = false;
            performFetch('/control', {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify({ device: "led", state: "VYPNUTO" })
            })
              .then(resp => console.log("LED osvětlení bylo vypnuto:", resp))
              .catch(error => console.error('Chyba při vypínání LED:', error));
          }
        } else {
          updateElement('scheduleStatus', "Chyba při mazání plánu");
        }
      })
      .catch(error => {
        console.error('Chyba:', error);
        updateElement('scheduleStatus', "Chyba při komunikaci se serverem");
      });
  });

  // Outfan nastavení
  document.getElementById('outfanSettingsForm').addEventListener('submit', event => {
    event.preventDefault();
    const temperature = document.getElementById('outfan_temperature').value;
    const humidity = document.getElementById('outfan_humidity').value;

    performFetch('/outfan/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ temperature, humidity })
    })
      .then(data => {
        if (data.status === "success") {
          updateOutfanSettingsState();
        } else {
          alert("Chyba nastavení mezních hodnot");
        }
      })
      .catch(error => console.error('Chyba:', error));
  });

  document.getElementById('deleteLimitsBtn').addEventListener('click', () => {
    performFetch('/outfan/settings/delete', { method: 'POST' })
      .then(data => {
        const outfanCheckbox = document.getElementById('outfan');
        if (outfanCheckbox && outfanCheckbox.checked) {
          outfanCheckbox.checked = false;
          performFetch('/control', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ device: "outfan", state: "VYPNUTO" })
          })
            .then(resp => console.log("Outfan byl vypnut:", resp))
            .catch(error => console.error('Chyba při vypínání outfan:', error));
        } else {
          alert("Chyba při mazání mezních hodnot");
        }
      })
      .catch(error => console.error('Chyba:', error));
  });

  // PH nastavení
  const phMaxSlider = document.getElementById('ph_max_value');
  const phMinSlider = document.getElementById('ph_min_value');
  const phMaxLabel = document.getElementById('ph_max_value_label');
  const phMinLabel = document.getElementById('ph_min_value_label');
  const phSliderContainer = document.getElementById('phSliderContainer');

  phMaxLabel.textContent = phMaxSlider.value;
  phMinLabel.textContent = phMinSlider.value;

  phMaxSlider.addEventListener('input', () => {
    phMaxLabel.textContent = phMaxSlider.value;
  });
  phMinSlider.addEventListener('input', () => {
    phMinLabel.textContent = phMinSlider.value;
  });

  document.getElementById('savePHSettings').addEventListener('click', () => {
    const phMax = phMaxSlider.value;
    const phMin = phMinSlider.value;
    if (phMax === "" || phMin === "") {
      alert("Prosím zadejte obě hodnoty pH (min a max).");
      return;
    }
    performFetch('/ph/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ ph_max: phMax, ph_min: phMin })
    })
      .then(data => {
        if (data.status === "success") {
          const statusText = `pH ovládání nastaveno: ${phMin} - ${phMax}`;
          updateElement('phStatus', statusText);
          const phStatusElement = document.getElementById('phStatus');
          phStatusElement.classList.remove('infan-not-set');
          phStatusElement.classList.add('infan-set');
          phSliderContainer.classList.add('ph-set');
          phMaxSlider.disabled = true;
          phMinSlider.disabled = true;
        } else {
          updateElement('phStatus', `Chyba při nastavování pH: ${data.message}`);
        }
      })
      .catch(error => console.error('Chyba:', error));
  });

  document.getElementById('deletePHSettings').addEventListener('click', () => {
    performFetch('/ph/settings/delete', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' }
    })
      .then(data => {
        if (data.status === "success") {
          updateElement('phStatus', "pH ovládání není nastaveno");
          const phStatusElement = document.getElementById('phStatus');
          phStatusElement.classList.remove('infan-set');
          phStatusElement.classList.add('infan-not-set');
          phMaxSlider.value = '';
          phMinSlider.value = '';
          phMaxLabel.textContent = "-";
          phMinLabel.textContent = "-";
          phSliderContainer.classList.remove('ph-set');
          phMaxSlider.disabled = false;
          phMinSlider.disabled = false;
        } else {
          updateElement('phStatus', "Chyba při mazání pH nastavení");
        }
      })
      .catch(error => console.error('Chyba:', error));
  });

  // TDS nastavení
  document.getElementById('saveTDSSettings').addEventListener('click', () => {
    const tdsMin = document.getElementById('tds_min_value').value;
    if (tdsMin === "") {
      alert("Zadejte minimální hodnotu TDS.");
      return;
    }
    performFetch('/tds/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ tds_min_value: tdsMin })
    })
      .then(data => {
        if (data.status === "success") {
          updateTDSStatus(data.tds_settting);
        } else {
          updateElement('tdsStatus', `Chyba při nastavování TDS: ${data.message}`);
        }
      })
      .catch(error => console.error('Chyba:', error));
  });

  document.getElementById('deleteTDSSettings').addEventListener('click', () => {
    performFetch('/tds/settings/delete', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' }
    })
      .then(data => updateTDSStatus(data.tds_settting))
      .catch(error => console.error('Chyba:', error));
  });
};

/************************************
 * Inicializace
 ************************************/

// Aktualizace dat a stavu po načtení stránky
window.onload = () => {
  updateData();
  updateControlState();
  updateOutfanSettingsState();
};

// Periodické aktualizace dat a stavu
setInterval(updateData, 60000);
setInterval(updateControlState, 5000);

document.addEventListener('DOMContentLoaded', initEventListeners);
