const canvas = document.getElementById('screen');
const ctx = canvas.getContext('2d');
const status = document.getElementById('status');
const screenLabel = document.getElementById('screen-label');
const themeLabel = document.getElementById('theme-label');
const themeId = document.getElementById('theme-id');
const themeSelect = document.getElementById('theme-select');
const overlays = { grid: false, bounds: false, safe: false, ids: false, fps: false };
let lastFrame = null;

const color = value => `rgb(${value[0]}, ${value[1]}, ${value[2]})`;
const screenName = value => value.replace(/([A-Z])/g, ' $1').trim();

function draw(frame) {
  lastFrame = frame;
  ctx.clearRect(0, 0, 240, 320);
  ctx.imageSmoothingEnabled = false;
  for (const command of frame.commands) {
    const [x, y, w, h] = command.rect;
    if (command.type === 'rect') {
      ctx.fillStyle = color(command.fg);
      ctx.fillRect(x, y, w, h);
    } else if (command.type === 'text' || command.type === 'icon' || command.type === 'transition') {
      const size = Math.max(7, 7 * command.scale);
      ctx.font = `${command.type === 'icon' ? 'bold ' : ''}${size}px ui-monospace, SFMono-Regular, Menlo, monospace`;
      ctx.textBaseline = 'top';
      ctx.fillStyle = color(command.fg);
      if (command.type === 'icon') {
        ctx.strokeStyle = color(command.fg);
        ctx.strokeRect(x, y, w, h);
        ctx.fillText(command.text, x + 2, y + 5);
      } else if (command.type === 'transition') {
        ctx.globalAlpha = Math.min(1, command.value / Math.max(1, command.max_value));
        ctx.fillText(command.text, x, y);
        ctx.globalAlpha = 1;
      } else {
        ctx.fillText(command.text, x, y);
      }
    } else if (command.type === 'progress') {
      ctx.fillStyle = color(command.bg);
      ctx.fillRect(x, y, w, h);
      ctx.fillStyle = color(command.fg);
      ctx.fillRect(x, y, Math.round(w * command.value / Math.max(1, command.max)), h);
    }
  }
  drawOverlays(frame);
  screenLabel.textContent = screenName(frame.screen);
  themeLabel.textContent = frame.themeName;
  themeId.textContent = frame.theme;
  if (themeSelect.value !== frame.theme) themeSelect.value = frame.theme;
  document.getElementById('debug-theme').textContent = frame.themeName;
  document.getElementById('debug-screen').textContent = screenName(frame.screen);
  document.getElementById('debug-playback').textContent = frame.playback;
  document.getElementById('debug-song').textContent = frame.song.trim() === '-' ? 'No active song' : frame.song;
  document.getElementById('debug-battery').textContent = `${frame.battery}%`;
  document.getElementById('debug-bluetooth').textContent = frame.bluetooth ? 'Connected' : 'Disconnected';
  document.getElementById('debug-spotify').textContent = frame.spotify ? 'Connected' : 'Disconnected';
  document.getElementById('debug-wifi').textContent = frame.wifi ? 'Connected' : 'Disconnected';
}

function drawOverlays(frame) {
  ctx.save();
  if (overlays.grid) {
    ctx.strokeStyle = 'rgba(80, 210, 180, .24)';
    ctx.lineWidth = 1;
    for (let x = 0; x <= 240; x += 4) { ctx.beginPath(); ctx.moveTo(x + .5, 0); ctx.lineTo(x + .5, 320); ctx.stroke(); }
    for (let y = 0; y <= 320; y += 4) { ctx.beginPath(); ctx.moveTo(0, y + .5); ctx.lineTo(240, y + .5); ctx.stroke(); }
  }
  if (overlays.safe) {
    ctx.strokeStyle = '#ffca6b'; ctx.setLineDash([3, 2]); ctx.strokeRect(8.5, 8.5, 223, 303); ctx.setLineDash([]);
  }
  if (overlays.bounds || overlays.ids) {
    frame.commands.forEach((command, index) => {
      const [x, y, w, h] = command.rect;
      if (overlays.bounds) { ctx.strokeStyle = 'rgba(255, 92, 140, .72)'; ctx.strokeRect(x + .5, y + .5, w, h); }
      if (overlays.ids) { ctx.fillStyle = '#ff5c8c'; ctx.font = '6px ui-monospace, monospace'; ctx.fillText(`${index}:${command.type}`, x + 1, Math.max(0, y - 6)); }
    });
  }
  if (overlays.fps) {
    ctx.fillStyle = 'rgba(0,0,0,.76)'; ctx.fillRect(166, 12, 66, 15);
    ctx.fillStyle = '#8fffd8'; ctx.font = '8px ui-monospace, monospace'; ctx.fillText(`FPS ${frame.fps}`, 171, 16);
  }
  ctx.restore();
}

async function action(value) {
  status.textContent = `Sending ${value} to native firmware...`;
  try {
    const response = await fetch('/api/action', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ action: value }) });
    const frame = await response.json();
    if (frame.error) throw new Error(frame.error);
    draw(frame);
    status.textContent = `Native UiFrame: ${frame.commands.length} drawing commands`;
  } catch (error) {
    status.textContent = `Backend unavailable: ${error.message}`;
  }
}

document.querySelectorAll('[data-action]').forEach(button => button.addEventListener('click', () => action(button.dataset.action)));
document.querySelectorAll('[data-debug-action]').forEach(button => button.addEventListener('click', () => action(button.dataset.debugAction)));
document.querySelectorAll('[data-overlay]').forEach(input => input.addEventListener('change', event => {
  overlays[event.target.dataset.overlay] = event.target.checked;
  if (lastFrame) draw(lastFrame);
}));
themeSelect.addEventListener('change', event => action(`theme:${event.target.value}`));
action('none');
