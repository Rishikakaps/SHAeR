const http = require('http');
const fs = require('fs');
const path = require('path');
const readline = require('readline');
const { spawn, spawnSync } = require('child_process');

const root = path.resolve(__dirname, '..');
const publicRoot = path.join(__dirname, 'public');
const backendPath = path.join(root, 'native_firmware', 'build', 'shaer_preview_backend');

function ensureBackend() {
  if (fs.existsSync(backendPath)) return;
  const result = spawnSync('make', ['-C', path.join(root, 'native_firmware'), 'preview'], { stdio: 'inherit' });
  if (result.status !== 0) throw new Error('Unable to build the firmware preview backend');
}

ensureBackend();
const backend = spawn(backendPath, [], { cwd: path.join(root, 'native_firmware') });
const frames = readline.createInterface({ input: backend.stdout });
let latestFrame = null;
let pending = [];
frames.on('line', line => {
  try {
    latestFrame = JSON.parse(line);
    const resolve = pending.shift();
    if (resolve) resolve(latestFrame);
  } catch (error) {
    console.error('Preview backend returned invalid JSON:', error.message);
  }
});
backend.stderr.on('data', data => process.stderr.write(`[firmware] ${data}`));
backend.on('exit', code => {
  while (pending.length) pending.shift()(latestFrame);
  if (code) console.error(`Preview backend exited with code ${code}`);
});

function sendAction(action) {
  return new Promise(resolve => {
    pending.push(resolve);
    backend.stdin.write(`${action}\n`);
  });
}

function contentType(file) {
  return file.endsWith('.html') ? 'text/html; charset=utf-8'
    : file.endsWith('.js') ? 'text/javascript; charset=utf-8'
      : file.endsWith('.css') ? 'text/css; charset=utf-8'
        : file.endsWith('.png') ? 'image/png'
        : 'application/octet-stream';
}

function serveFirmwareAsset(request, response) {
  const assetRoot = path.join(root, 'native_firmware');
  const relative = decodeURIComponent(request.url.slice('/firmware-assets/'.length));
  const file = path.resolve(assetRoot, relative);
  if (!file.startsWith(assetRoot) || !fs.existsSync(file)) {
    response.writeHead(404); response.end('Asset not found'); return;
  }
  response.writeHead(200, { 'Content-Type': contentType(file), 'Cache-Control': 'no-store' });
  fs.createReadStream(file).pipe(response);
}

function serveStatic(request, response) {
  const requested = request.url === '/' ? '/index.html' : request.url;
  const file = path.resolve(publicRoot, `.${requested}`);
  if (!file.startsWith(publicRoot) || !fs.existsSync(file)) {
    response.writeHead(404); response.end('Not found'); return;
  }
  response.writeHead(200, { 'Content-Type': contentType(file), 'Cache-Control': 'no-store' });
  fs.createReadStream(file).pipe(response);
}

function body(request) {
  return new Promise((resolve, reject) => {
    let value = '';
    request.on('data', chunk => { value += chunk; });
    request.on('end', () => { try { resolve(JSON.parse(value || '{}')); } catch (error) { reject(error); } });
    request.on('error', reject);
  });
}

const server = http.createServer(async (request, response) => {
  try {
    if (request.url === '/api/state' && request.method === 'GET') {
      response.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
      response.end(JSON.stringify(latestFrame)); return;
    }
    if (request.url === '/api/action' && request.method === 'POST') {
      const payload = await body(request);
      const frame = await sendAction(String(payload.action || 'none'));
      response.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
      response.end(JSON.stringify(frame)); return;
    }
    if (request.url.startsWith('/firmware-assets/')) {
      serveFirmwareAsset(request, response); return;
    }
    serveStatic(request, response);
  } catch (error) {
    response.writeHead(500, { 'Content-Type': 'application/json' });
    response.end(JSON.stringify({ error: error.message }));
  }
});

const port = Number(process.env.PORT || 3000);
server.listen(port, '127.0.0.1', () => {
  console.log(`SHAeR Desktop Preview: http://localhost:${port}`);
});

function shutdown() {
  backend.stdin.write('quit\n');
  backend.kill();
  server.close(() => process.exit(0));
}
process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);
