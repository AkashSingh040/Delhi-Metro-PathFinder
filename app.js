/* ============================================================
   Delhi Metro PathFinder — Frontend Logic
   ============================================================ */

const API_BASE = 'http://localhost:1158';

// Metro line → CSS color (must match style.css variables)
const LINE_COLORS = {
  Red: '#e53935',
  Yellow: '#fbbf24',
  Blue: '#3b82f6',
  Green: '#22c55e',
  Violet: '#8b5cf6',
  Pink: '#ec4899',
  Magenta: '#d946ef',
  Grey: '#94a3b8',
  Aqua: '#06b6d4',
  Orange: '#f97316',
  Rapid: '#78716c',
};

// ── DOM refs ──────────────────────────────────────────────────
const fromInput = document.getElementById('from-input');
const toInput = document.getElementById('to-input');
const fromDatalist = document.getElementById('from-datalist');
const toDatalist = document.getElementById('to-datalist');
const form = document.getElementById('route-form');
const findBtn = document.getElementById('find-btn');
const findBtnText = findBtn.querySelector('.find-btn-text');
const findBtnLoader = findBtn.querySelector('.find-btn-loader');
const swapBtn = document.getElementById('swap-btn');
const clearFromBtn = document.getElementById('clear-from');
const clearToBtn = document.getElementById('clear-to');
const resultSection = document.getElementById('result-section');
const errorSection = document.getElementById('error-section');
const errorMessage = document.getElementById('error-message');
const loadingSection = document.getElementById('loading-section');
const routeContent = document.getElementById('route-content');
const statTime = document.getElementById('stat-time');
const statStations = document.getElementById('stat-stations');
const statChanges = document.getElementById('stat-changes');

// ── Station data ───────────────────────────────────────────────
let allStations = []; // [{ name, lines }]

async function loadStations() {
  try {
    const res = await fetch(`${API_BASE}/api/stations`);
    if (!res.ok) throw new Error('Failed to fetch stations');
    allStations = await res.json();
    populateDatalist(fromDatalist, allStations);
    populateDatalist(toDatalist, allStations);
    console.log(`✅ Loaded ${allStations.length} stations`);
  } catch (err) {
    console.warn('Could not load stations from server:', err.message);
    showError('Cannot connect to the server. Make sure server.exe is running on port 8080.');
  }
}

function populateDatalist(datalist, stations) {
  datalist.innerHTML = '';
  stations.forEach(s => {
    const opt = document.createElement('option');
    opt.value = s.name;
    opt.label = s.lines.join(', ');
    datalist.appendChild(opt);
  });
}

// ── Clear buttons ──────────────────────────────────────────────
clearFromBtn.addEventListener('click', () => { fromInput.value = ''; fromInput.focus(); updateClearBtns(); });
clearToBtn.addEventListener('click', () => { toInput.value = ''; toInput.focus(); updateClearBtns(); });
fromInput.addEventListener('input', updateClearBtns);
toInput.addEventListener('input', updateClearBtns);

function updateClearBtns() {
  clearFromBtn.classList.toggle('visible', fromInput.value.length > 0);
  clearToBtn.classList.toggle('visible', toInput.value.length > 0);
}

// ── Swap button ────────────────────────────────────────────────
swapBtn.addEventListener('click', () => {
  const tmp = fromInput.value;
  fromInput.value = toInput.value;
  toInput.value = tmp;
  swapBtn.classList.add('spinning');
  setTimeout(() => swapBtn.classList.remove('spinning'), 350);
  updateClearBtns();
});

// ── Form submit ────────────────────────────────────────────────
form.addEventListener('submit', async (e) => {
  e.preventDefault();
  const from = fromInput.value.trim();
  const to = toInput.value.trim();

  if (!from || !to) return;
  if (from === to) {
    showError('Source and destination cannot be the same station.');
    return;
  }

  await findRoute(from, to);
});

// ── Find Route ─────────────────────────────────────────────────
async function findRoute(from, to) {
  setLoading(true);
  hideAll();
  showSection(loadingSection);

  try {
    const res = await fetch(`${API_BASE}/api/route`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ from, to }),
    });

    if (!res.ok) throw new Error(`Server error: ${res.status}`);
    const data = await res.json();

    hideAll();

    if (data.error) {
      showError(data.error + ` (${data.from} → ${data.to})`);
    } else {
      renderRoute(data);
      showSection(resultSection);
    }
  } catch (err) {
    hideAll();
    showError('Could not connect to the server. Make sure server.exe is running.');
    console.error(err);
  } finally {
    setLoading(false);
  }
}

// ── Render Route ───────────────────────────────────────────────
function renderRoute(data) {
  // Stats bar
  animateNumber(statTime, 0, data.total_time, ' ');
  animateNumber(statStations, 0, data.total_stations, ' ');
  animateNumber(statChanges, 0, data.changes, ' ');

  // Route content
  routeContent.innerHTML = '';

  const segments = data.segments;
  if (!segments || segments.length === 0) {
    routeContent.innerHTML = '<p style="color:var(--text-secondary);text-align:center;padding:20px">No route data.</p>';
    return;
  }

  segments.forEach((seg, idx) => {
    const color = LINE_COLORS[seg.line] || '#94a3b8';
    const stops = seg.stations;

    // If not first segment, show interchange arrow
    if (idx > 0) {
      const prevSeg = segments[idx - 1];
      const prevColor = LINE_COLORS[prevSeg.line] || '#94a3b8';
      const arrow = createInterchangeArrow(prevSeg.line, seg.line, prevColor, color);
      routeContent.appendChild(arrow);
    }

    const segEl = document.createElement('div');
    segEl.className = 'segment';
    segEl.style.setProperty('--lc', color);
    segEl.style.animationDelay = `${idx * 0.08}s`;
    segEl.style.opacity = '0';

    // Segment header
    segEl.innerHTML = `
      <div class="segment-header" style="background: color-mix(in srgb, ${color} 10%, transparent);">
        <span class="line-swatch" style="background:${color}; --lc:${color};"></span>
        <span style="color:${color}">${seg.line} Line</span>
        <span class="segment-stops">${stops.length} stop${stops.length !== 1 ? 's' : ''}</span>
      </div>
    `;

    // Station list
    const listEl = document.createElement('div');
    listEl.className = 'station-list';
    listEl.style.setProperty('--lc', color);

    stops.forEach((stationName, si) => {
      const isFirst = idx === 0 && si === 0;
      const isLast = idx === segments.length - 1 && si === stops.length - 1;

      // Check if interchange (appears in multiple segments' station lists)
      const isInterchange = isInterchangeStation(stationName, data.segments, idx, si);

      const item = document.createElement('div');
      item.className = `station-item${isFirst ? ' start' : ''}${isLast ? ' end-station' : ''}`;
      item.style.setProperty('--lc', color);

      item.innerHTML = `
        <div class="station-marker"></div>
        <div class="station-name">${stationName}</div>
        ${isInterchange && !isFirst && !isLast ? '<span class="interchange-badge">INTERCHANGE</span>' : ''}
        ${isFirst ? '<span style="font-size:11px;color:var(--accent);font-weight:600;margin-left:auto">BOARD</span>' : ''}
        ${isLast ? '<span style="font-size:11px;color:#a855f7;font-weight:600;margin-left:auto">ARRIVE</span>' : ''}
      `;
      listEl.appendChild(item);
    });

    segEl.appendChild(listEl);
    routeContent.appendChild(segEl);

    // Trigger animation
    requestAnimationFrame(() => {
      segEl.style.transition = 'opacity 0.4s ease, transform 0.4s ease';
      segEl.style.transform = 'translateY(10px)';
      requestAnimationFrame(() => {
        segEl.style.opacity = '1';
        segEl.style.transform = 'translateY(0)';
      });
    });
  });
}

function isInterchangeStation(name, segments, curSegIdx, stationIdx) {
  // A station is an interchange if it appears in more than one segment
  let count = 0;
  segments.forEach(seg => {
    if (seg.stations.includes(name)) count++;
  });
  return count > 1;
}

function createInterchangeArrow(fromLine, toLine, fromColor, toColor) {
  const el = document.createElement('div');
  el.className = 'interchange-arrow';
  el.innerHTML = `
    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
      <path d="M12 5v14M5 12l7 7 7-7"/>
    </svg>
    <span>Transfer: <strong style="color:${fromColor}">${fromLine} Line</strong></span>
    <span>→</span>
    <strong style="color:${toColor}">${toLine} Line</strong>
    <span class="interchange-line-info">(+5 min transfer time)</span>
  `;
  return el;
}

// ── Animate numbers ────────────────────────────────────────────
function animateNumber(el, from, to, suffix) {
  const duration = 800;
  const start = performance.now();
  function step(now) {
    const t = Math.min((now - start) / duration, 1);
    const ease = 1 - Math.pow(1 - t, 3); // ease-out-cubic
    el.textContent = Math.round(from + (to - from) * ease) + suffix;
    if (t < 1) requestAnimationFrame(step);
    else el.textContent = to + suffix;
  }
  requestAnimationFrame(step);
}

// ── UI helpers ─────────────────────────────────────────────────
function setLoading(active) {
  findBtn.disabled = active;
  findBtnText.hidden = active;
  findBtnLoader.hidden = !active;
}

function hideAll() {
  resultSection.hidden = true;
  errorSection.hidden = true;
  loadingSection.hidden = true;
}

function showSection(el) {
  el.hidden = false;
}

function showError(msg) {
  errorMessage.textContent = msg;
  showSection(errorSection);
}

// ── Keyboard shortcut: Enter to find ──────────────────────────
document.addEventListener('keydown', (e) => {
  if (e.key === 'Enter' && document.activeElement !== fromInput && document.activeElement !== toInput) {
    if (fromInput.value && toInput.value) {
      form.requestSubmit();
    }
  }
});

// ── Init ───────────────────────────────────────────────────────
loadStations();
updateClearBtns();
