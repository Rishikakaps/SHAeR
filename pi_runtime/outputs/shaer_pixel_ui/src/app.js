import { themes, screens } from "./themes.js";

const canvas = document.getElementById("screen");
const ctx = canvas.getContext("2d");
ctx.imageSmoothingEnabled = false;

const state = {
  theme: 0,
  screen: "home",
  selection: 1,
  playing: true,
  battery: 87,
  volume: 50,
  popup: false
};

const fontStack = {
  mono: '"Courier New", monospace',
  ticket: '"Courier New", monospace',
  raga: '"Courier New", monospace',
  xp: 'Tahoma, Verdana, sans-serif',
  punk: '"Courier New", monospace',
  soft: 'Georgia, serif'
};

function dprText(size, theme) {
  return `${size}px ${fontStack[theme.typography] || fontStack.mono}`;
}

function theme() {
  return themes[state.theme];
}

function clear(t) {
  ctx.fillStyle = t.palette.bg;
  ctx.fillRect(0, 0, 240, 320);
}

function rect(x, y, w, h, color) {
  ctx.fillStyle = color;
  ctx.fillRect(Math.round(x), Math.round(y), Math.round(w), Math.round(h));
}

function stroke(x, y, w, h, color, width = 1) {
  ctx.strokeStyle = color;
  ctx.lineWidth = width;
  ctx.strokeRect(Math.round(x) + 0.5, Math.round(y) + 0.5, Math.round(w) - 1, Math.round(h) - 1);
}

function text(value, x, y, size, color, t, align = "left", weight = "700") {
  ctx.fillStyle = color;
  ctx.font = `${weight} ${dprText(size, t)}`;
  ctx.textAlign = align;
  ctx.textBaseline = "top";
  ctx.fillText(String(value), Math.round(x), Math.round(y));
}

function clippedText(value, x, y, maxWidth, size, color, t, weight = "700") {
  ctx.save();
  ctx.beginPath();
  ctx.rect(x, y, maxWidth, size + 5);
  ctx.clip();
  text(value, x, y, size, color, t, "left", weight);
  ctx.restore();
}

function drawChrome(t, label) {
  if (t.chrome === "xp") {
    rect(0, 0, 240, 22, t.palette.accent);
    text(label, 7, 5, 11, "#ffffff", t);
    rect(219, 5, 13, 12, "#d94b37");
    text("x", 223, 4, 10, "#ffffff", t);
    rect(0, 22, 240, 16, "#ece9d8");
    text("File   View   Play", 7, 25, 9, "#111111", t, "left", "400");
    stroke(0, 0, 240, 320, "#245edb", 2);
    return 40;
  }
  if (t.chrome === "paper") {
    for (let x = 3; x < 240; x += 12) {
      rect(x, 0, 6, 4, t.palette.bg);
      rect(x, 316, 6, 4, t.palette.bg);
    }
    stroke(8, 8, 224, 304, t.palette.accent, 1);
    text(label, 14, 13, 10, t.palette.muted, t);
    return 31;
  }
  if (t.chrome === "ornament") {
    stroke(7, 7, 226, 306, t.palette.dim, 1);
    stroke(12, 12, 216, 296, t.palette.line, 1);
    text(label, 18, 18, 10, t.palette.accent2, t);
    return 39;
  }
  if (t.chrome === "zine") {
    rect(0, 0, 240, 24, t.palette.accent);
    text(label, 8, 6, 11, "#ffffff", t);
    for (let x = 0; x < 240; x += 10) rect(x, 28, 5, 3, t.palette.accent2);
    return 39;
  }
  if (t.chrome === "watercolor") {
    rect(8, 8, 224, 304, t.palette.panel);
    stroke(8, 8, 224, 304, "rgba(0,0,0,0.2)", 1);
    text(label, 18, 19, 10, t.palette.muted, t);
    return 42;
  }
  rect(0, 0, 240, 18, t.palette.panel);
  rect(1, 1, 238, 16, "#000080");
  text(label, 6, 4, 10, "#ffffff", t);
  rect(0, 18, 240, 15, t.palette.panel);
  text("File  View  Play  Theme", 6, 21, 9, "#000000", t, "left", "400");
  return 37;
}

function footer(t) {
  const y = 299;
  if (t.chrome === "xp") rect(0, y - 2, 240, 23, "#ece9d8");
  text("SHAeR", 8, y, 9, t.palette.muted, t, "left", "700");
  text(`${state.battery}%`, 224, y, 9, t.palette.muted, t, "right", "700");
}

function drawRows(rows, selected, x, y, w, rowH, t) {
  rows.forEach((row, index) => {
    const active = index === selected;
    let bg = active ? t.palette.select : t.palette.panel;
    let fg = active ? t.palette.selectInk : t.palette.ink;
    if (t.chrome === "paper") {
      rect(x, y + index * rowH, 6, rowH - 2, active ? t.palette.accent : t.palette.dim);
      rect(x + 8, y + index * rowH, w - 8, rowH - 2, bg);
    } else if (t.chrome === "zine") {
      rect(x + (index % 2 ? 3 : 0), y + index * rowH, w - 8, rowH - 3, bg);
      rect(x + w - 14, y + index * rowH, 10, rowH - 3, active ? t.palette.accent2 : t.palette.accent);
    } else {
      rect(x, y + index * rowH, w, rowH - 3, bg);
      stroke(x, y + index * rowH, w, rowH - 3, active ? t.palette.line : t.palette.dim, 1);
    }
    clippedText(row, x + 10, y + index * rowH + 6, w - 44, 11, fg, t);
    text(String(index + 1).padStart(2, "0"), x + w - 9, y + index * rowH + 6, 10, active ? fg : t.palette.muted, t, "right");
  });
}

function drawBoot(t) {
  const top = drawChrome(t, "LOADING");
  text("opening D Drive", 120, top + 35, 12, t.palette.ink, t, "center");
  text("mhmm mhmm", 120, top + 53, 11, t.palette.muted, t, "center");
  const points = [[58, 138], [92, 103], [141, 124], [169, 91], [191, 151], [128, 191], [76, 178]];
  ctx.strokeStyle = t.palette.accent2;
  ctx.lineWidth = 1;
  ctx.beginPath();
  points.forEach(([x, y], i) => i ? ctx.lineTo(x, y) : ctx.moveTo(x, y));
  ctx.stroke();
  points.forEach(([x, y], i) => {
    rect(x - 2, y - 2, 4 + (i % 2), 4 + (i % 2), i === 3 ? t.palette.accent : t.palette.ink);
  });
  text("SHAeR", 120, 240, 18, t.palette.ink, t, "center");
  footer(t);
}

function drawHome(t) {
  const top = drawChrome(t, "HOME");
  drawRows(t.menu, state.selection, 18, top + 10, 204, 34, t);
  footer(t);
}

function drawLibrary(t) {
  const top = drawChrome(t, "LIBRARY");
  text("My Music", 18, top + 4, 13, t.palette.accent2, t);
  drawRows(t.library, state.selection, 18, top + 26, 204, 30, t);
  footer(t);
}

function drawSettings(t) {
  const top = drawChrome(t, "SETTINGS");
  drawRows(t.settings, state.selection, 18, top + 6, 204, 26, t);
  footer(t);
}

function drawNowPlaying(t) {
  const top = drawChrome(t, "PLAYING");
  if (t.chrome === "paper") {
    stroke(46, top + 12, 148, 112, t.palette.accent, 1);
    rect(56, top + 22, 128, 73, "#f9edd0");
    text("SONG", 120, top + 49, 18, t.palette.accent, t, "center");
  } else if (t.chrome === "xp") {
    rect(42, top + 11, 156, 112, "#ffffff");
    stroke(42, top + 11, 156, 112, "#7f9db9", 1);
    rect(63, top + 31, 114, 58, "#d7e8ff");
    text("VLC", 120, top + 51, 23, "#f08a00", t, "center");
  } else if (t.chrome === "watercolor") {
    rect(48, top + 12, 144, 100, "#dce8d4");
    for (let i = 0; i < 10; i++) rect(56 + i * 13, top + 88 - (i % 3) * 9, 9, 21 + (i % 4) * 4, t.palette.accent2);
  } else {
    const cx = 120, cy = top + 65;
    ctx.strokeStyle = t.palette.accent2;
    for (let r = 18; r <= 48; r += 7) {
      ctx.beginPath();
      ctx.arc(cx, cy, r, 0, Math.PI * 2);
      ctx.stroke();
    }
    rect(cx - 7, cy - 7, 14, 14, t.palette.accent);
  }
  clippedText("Cloud Song", 22, top + 140, 196, 18, t.palette.ink, t);
  clippedText("SHAeR Local Archive", 22, top + 162, 196, 11, t.palette.muted, t, "400");
  stroke(22, top + 188, 196, 9, t.palette.dim, 1);
  rect(24, top + 190, 112, 5, t.palette.accent);
  text("02:04", 22, top + 204, 10, t.palette.muted, t);
  text("04:17", 218, top + 204, 10, t.palette.muted, t, "right");
  footer(t);
}

function drawPopup(t) {
  drawChrome(t, "NOTICE");
  rect(24, 102, 192, 110, t.palette.panel);
  stroke(24, 102, 192, 110, t.palette.line, 2);
  text("Spotify slipped", 120, 119, 15, t.palette.ink, t, "center");
  text("Playback stopped.", 120, 147, 11, t.palette.muted, t, "center", "400");
  text("OK opens Local Library.", 120, 163, 11, t.palette.muted, t, "center", "400");
  rect(78, 185, 84, 18, t.palette.select);
  text("OK", 120, 189, 10, t.palette.selectInk, t, "center");
  footer(t);
}

function drawCharging(t) {
  const top = drawChrome(t, "CHARGING");
  text("Plugged In", 120, top + 31, 16, t.palette.ink, t, "center");
  stroke(62, top + 75, 105, 34, t.palette.line, 2);
  rect(168, top + 85, 8, 14, t.palette.line);
  rect(66, top + 79, 74, 26, t.palette.accent2);
  text("87%", 120, top + 121, 17, t.palette.accent, t, "center");
  footer(t);
}

function drawSleep(t) {
  drawChrome(t, "AOD");
  text("Cloud Song", 120, 111, 14, t.palette.ink, t, "center");
  text("02:04 / 04:17", 120, 137, 18, t.palette.accent, t, "center");
  text("AOD", 120, 195, 34, t.palette.dim, t, "center");
  footer(t);
}

function drawScanlines() {
  ctx.save();
  ctx.globalAlpha = 0.16;
  ctx.fillStyle = "#ffffff";
  for (let y = 0; y < 320; y += 4) ctx.fillRect(0, y, 240, 1);
  ctx.restore();
}

function render() {
  const t = theme();
  clear(t);
  if (state.screen === "boot") drawBoot(t);
  if (state.screen === "home") drawHome(t);
  if (state.screen === "library") drawLibrary(t);
  if (state.screen === "now") drawNowPlaying(t);
  if (state.screen === "settings") drawSettings(t);
  if (state.screen === "spotify_drop") drawPopup(t);
  if (state.screen === "charging") drawCharging(t);
  if (state.screen === "sleep") drawSleep(t);
  drawScanlines();
  document.getElementById("themeName").textContent = t.label;
  document.getElementById("screenName").textContent = screens.find(s => s.id === state.screen).label;
  document.getElementById("selectionName").textContent = String(state.selection);
}

function maxSelection() {
  const t = theme();
  if (state.screen === "home") return t.menu.length - 1;
  if (state.screen === "library") return t.library.length - 1;
  if (state.screen === "settings") return t.settings.length - 1;
  return 0;
}

function sendInput(input) {
  if (input === "up") state.selection = Math.max(0, state.selection - 1);
  if (input === "down") state.selection = Math.min(maxSelection(), state.selection + 1);
  if (input === "back") {
    if (state.screen === "home") state.screen = "sleep";
    else state.screen = "home";
    state.selection = 0;
  }
  if (input === "ok") {
    if (state.screen === "home") {
      state.screen = ["spotify_drop", "library", "library", "settings"][state.selection] || "library";
      state.selection = 0;
    } else if (state.screen === "library") {
      state.screen = "now";
    } else if (state.screen === "spotify_drop") {
      state.screen = "library";
    } else if (state.screen === "settings" && state.selection === 5) {
      state.screen = "sleep";
    }
  }
  render();
}

function makeControls() {
  const themeGrid = document.getElementById("themeGrid");
  themes.forEach((t, index) => {
    const b = document.createElement("button");
    b.className = "chip";
    b.innerHTML = `<span class="swatch" style="background:${t.palette.accent}"></span><span>${t.label}</span>`;
    b.addEventListener("click", () => {
      state.theme = index;
      state.selection = Math.min(state.selection, maxSelection());
      syncControls();
      render();
    });
    themeGrid.appendChild(b);
  });

  const screenGrid = document.getElementById("screenGrid");
  screens.forEach((screen) => {
    const b = document.createElement("button");
    b.className = "chip";
    b.textContent = screen.label;
    b.addEventListener("click", () => {
      state.screen = screen.id;
      state.selection = Math.min(state.selection, maxSelection());
      syncControls();
      render();
    });
    screenGrid.appendChild(b);
  });
}

function syncControls() {
  document.querySelectorAll("#themeGrid .chip").forEach((b, i) => b.classList.toggle("active", i === state.theme));
  document.querySelectorAll("#screenGrid .chip").forEach((b, i) => b.classList.toggle("active", screens[i].id === state.screen));
}

document.querySelectorAll("[data-input]").forEach((button) => {
  button.addEventListener("click", () => sendInput(button.dataset.input));
});

window.addEventListener("keydown", (event) => {
  if (event.key === "ArrowUp") sendInput("up");
  if (event.key === "ArrowDown") sendInput("down");
  if (event.key === "Enter") sendInput("ok");
  if (event.key === "Escape" || event.key === "Backspace") sendInput("back");
});

makeControls();
syncControls();
render();
