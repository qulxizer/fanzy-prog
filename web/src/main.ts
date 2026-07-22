import './style.css';

type Config = {
  magic: number;
  temp_divider_pu: boolean;
  fan_pwm_inverted: boolean;
  ac_pullup: boolean;
  temp_r_fixed_ohm: number;
  temp_adc_max: number;
  temp_adc_min: number;
  temp_adc_short_threshold: number;
  temp_adc_open_threshold: number;
  temp_min_valid_c: number;
  temp_max_valid_c: number;
  fan_temp_on_c: number;
  fan_temp_full_c: number;
  fan_min_duty: number;
  fan_max_duty: number;
  ac_multiplier: number;
  ac_min_speed: number;
};

const integerFields: Array<[keyof Config, string, string]> = [
  ['temp_r_fixed_ohm', 'Fixed resistance', 'ohm'],
  ['temp_adc_max', 'ADC maximum', 'raw'],
  ['temp_adc_min', 'ADC minimum', 'raw'],
  ['temp_adc_short_threshold', 'Short threshold', 'raw'],
  ['temp_adc_open_threshold', 'Open threshold', 'raw'],
  ['temp_min_valid_c', 'Minimum valid temperature', '°C'],
  ['temp_max_valid_c', 'Maximum valid temperature', '°C'],
  ['fan_min_duty', 'Minimum duty', '%'],
  ['fan_max_duty', 'Maximum duty', '%'],
  ['ac_min_speed', 'Minimum speed', 'rpm'],
];

const numberFields: Array<[keyof Config, string, string]> = [
  ['fan_temp_on_c', 'Fan starts at', '°C'],
  ['fan_temp_full_c', 'Fan reaches full speed', '°C'],
  ['ac_multiplier', 'AC multiplier', 'x'],
];

const defaults: Config = {
  magic: 0,
  temp_divider_pu: false,
  fan_pwm_inverted: false,
  ac_pullup: false,
  temp_r_fixed_ohm: 0,
  temp_adc_max: 0,
  temp_adc_min: 0,
  temp_adc_short_threshold: 0,
  temp_adc_open_threshold: 0,
  temp_min_valid_c: 0,
  temp_max_valid_c: 0,
  fan_temp_on_c: 0,
  fan_temp_full_c: 0,
  fan_min_duty: 0,
  fan_max_duty: 0,
  ac_multiplier: 0,
  ac_min_speed: 0,
};

const app = document.querySelector<HTMLDivElement>('#app');
if (!app) throw new Error('App root is missing');

const field = (key: keyof Config, label: string, unit: string, step: string) => `
  <label class="field">
    <span>${label}</span>
    <div class="input-wrap">
      <input name="${key}" type="number" step="${step}" inputmode="decimal" required>
      <small>${unit}</small>
    </div>
  </label>`;

app.innerHTML = `
  <div class="shell">
    <header class="topbar">
      <div class="brand"><span class="brand-mark">F</span><div><strong>FANZY</strong><span>CONTROL PANEL</span></div></div>
      <div class="connection"><i></i><span>DEVICE LINK</span><b id="endpoint">/config</b></div>
    </header>
    <section class="hero">
      <div><p class="eyebrow">ESP32 / CONFIGURATION</p><h1>Thermal control,<br><em>tuned precisely.</em></h1><p class="intro">Read the active controller settings, adjust them safely, and write the complete configuration back to the device.</p></div>
      <div class="hero-stamp"><span>LIVE</span><strong>CONFIG</strong><small>LOCAL DEVICE</small></div>
    </section>
    <div id="notice" class="notice" role="status" aria-live="polite"></div>
    <form id="config-form">
      <section class="panel identity-panel">
        <div class="panel-heading"><div><span class="section-number">01</span><h2>Device identity</h2></div><p>Protocol identifier</p></div>
        ${field('magic', 'Magic value', 'hex / int', '1')}
      </section>
      <div class="grid two-col">
        <section class="panel">
          <div class="panel-heading"><div><span class="section-number">02</span><h2>Signal inputs</h2></div><p>Electrical sensing</p></div>
          <div class="switch-list">
            <label class="switch-row"><span><b>Temperature divider pull-up</b><small>Enable the temperature divider resistor</small></span><input name="temp_divider_pu" type="checkbox"><i></i></label>
            <label class="switch-row"><span><b>AC input pull-up</b><small>Enable pull-up on the AC sense line</small></span><input name="ac_pullup" type="checkbox"><i></i></label>
          </div>
          <div class="fields">${integerFields.slice(0, 5).map(([key, label, unit]) => field(key, label, unit, '1')).join('')}</div>
        </section>
        <section class="panel">
          <div class="panel-heading"><div><span class="section-number">03</span><h2>Temperature limits</h2></div><p>Safety boundaries</p></div>
          <div class="fields">${integerFields.slice(5, 7).map(([key, label, unit]) => field(key, label, unit, '1')).join('')}</div>
          <div class="range-note"><span class="dot"></span><span>Keep the valid range wider than the fan control range.</span></div>
          <div class="fields">${numberFields.slice(0, 2).map(([key, label, unit]) => field(key, label, unit, '0.1')).join('')}</div>
        </section>
        <section class="panel">
          <div class="panel-heading"><div><span class="section-number">04</span><h2>Fan response</h2></div><p>Output behaviour</p></div>
          <label class="switch-row"><span><b>Invert PWM output</b><small>Reverse the duty cycle sent to the fan</small></span><input name="fan_pwm_inverted" type="checkbox"><i></i></label>
          <div class="fields">${integerFields.slice(7, 9).map(([key, label, unit]) => field(key, label, unit, '1')).join('')}</div>
        </section>
        <section class="panel">
          <div class="panel-heading"><div><span class="section-number">05</span><h2>AC sensing</h2></div><p>Speed calculation</p></div>
          <div class="fields">${field('ac_multiplier', 'AC multiplier', 'x', '0.01')}${field('ac_min_speed', 'Minimum speed', 'rpm', '1')}</div>
          <div class="json-card"><span>PAYLOAD PREVIEW</span><code id="json-preview">{ }</code></div>
        </section>
      </div>
      <footer class="actions"><span id="last-action">No changes sent</span><div><button type="button" class="button secondary" id="reload">↻ <span>Read device</span></button><button type="submit" class="button primary" id="save">Write configuration <b>→</b></button></div></footer>
    </form>
  </div>`;

const form = document.querySelector<HTMLFormElement>('#config-form');
if (!form) throw new Error('Configuration form is missing');
const notice = document.querySelector<HTMLDivElement>('#notice');
const preview = document.querySelector<HTMLElement>('#json-preview');
const lastAction = document.querySelector<HTMLElement>('#last-action');
const reloadButton = document.querySelector<HTMLButtonElement>('#reload');
const saveButton = document.querySelector<HTMLButtonElement>('#save');

function readForm(): Config {
  const data = new FormData(form!);
  const config = { ...defaults };
  (Object.keys(config) as Array<keyof Config>).forEach((key) => {
    if (typeof defaults[key] === 'boolean') (config as unknown as Record<string, number | boolean>)[key] = data.has(key);
    else (config as unknown as Record<string, number | boolean>)[key] = Number(data.get(key));
  });
  return config;
}

function fillForm(config: Config): void {
  (Object.keys(defaults) as Array<keyof Config>).forEach((key) => {
    const input = form!.elements.namedItem(key);
    if (!(input instanceof HTMLInputElement)) return;
    if (input.type === 'checkbox') input.checked = Boolean(config[key]);
    else input.value = String(config[key]);
  });
  updatePreview();
}

function updatePreview(): void {
  if (preview) preview.textContent = JSON.stringify(readForm(), null, 2);
}

function setNotice(message: string, kind: 'success' | 'error' | 'loading'): void {
  if (!notice) return;
  notice.textContent = message;
  notice.className = `notice ${kind}`;
}

async function loadConfig(): Promise<void> {
  reloadButton?.classList.add('busy');
  setNotice('Reading configuration from device…', 'loading');
  try {
    const response = await fetch('/config', { headers: { Accept: 'application/json' } });
    if (!response.ok) throw new Error(`Device returned ${response.status}`);
    fillForm(await response.json() as Config);
    setNotice('Configuration loaded from device.', 'success');
    if (lastAction) lastAction.textContent = `Read at ${new Date().toLocaleTimeString()}`;
  } catch (error) {
    setNotice(error instanceof Error ? error.message : 'Could not read device.', 'error');
  } finally { reloadButton?.classList.remove('busy'); }
}

form.addEventListener('input', updatePreview);
form.addEventListener('submit', async (event) => {
  event.preventDefault();
  const config = readForm();
  saveButton?.classList.add('busy');
  setNotice('Writing configuration to device…', 'loading');
  try {
    const response = await fetch('/config', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(config) });
    if (!response.ok) throw new Error(`Device returned ${response.status}`);
    setNotice('Configuration written successfully.', 'success');
    if (lastAction) lastAction.textContent = `Written at ${new Date().toLocaleTimeString()}`;
  } catch (error) {
    setNotice(error instanceof Error ? error.message : 'Could not write device.', 'error');
  } finally { saveButton?.classList.remove('busy'); }
});

reloadButton?.addEventListener('click', () => void loadConfig());
fillForm(defaults);
void loadConfig();
