const { spawnSync } = require('child_process');
const path = require('path');

const root = path.resolve(__dirname, '..');
const backend = path.join(root, 'native_firmware', 'build', 'shaer_preview_backend');
const result = spawnSync(backend, { input: 'theme:bombay_ticket\nclick\nclick\nquit\n', encoding: 'utf8', cwd: path.join(root, 'native_firmware') });
if (result.status !== 0) throw new Error(result.stderr || `backend exited ${result.status}`);
const frames = result.stdout.trim().split('\n').map(JSON.parse);
if (!frames.length || frames[0].width !== 240 || frames[0].height !== 320) throw new Error('missing 240x320 frame');
if (!frames.some(frame => frame.screen === 'Now Playing')) throw new Error('encoder action did not reach Now Playing');
const themed = frames.find(frame => frame.screen === 'Now Playing');
if (themed.theme !== 'bombay_ticket') throw new Error('theme selector did not select Bombay Ticket');
if (themed.commands.some(command => command.type === 'image')) throw new Error('runtime frame still depends on a bitmap');
if (!themed.commands.some(command => command.type === 'rect') || !themed.commands.some(command => command.type === 'text')) throw new Error('theme was not assembled from renderable primitives');
console.log(`desktop preview backend passed (${frames.length} firmware frames, procedural themed primitives present)`);
