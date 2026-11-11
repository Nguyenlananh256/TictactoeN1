/* UI integration contract is documented in README.md. No simulated AI statistics. */
'use strict';
const $ = id => document.getElementById(id);
const { SIZE, createBoard, winningLine } = CaroRules;
const providers = new Map();
let board, moves, turn, result, config, startedAt, finishedAt, paused = false, busy = false;
let generation = 0, controller = null, pendingReset = null, aiError = '';
const cells = [];
const coordinate = (r, c) => `${String.fromCharCode(65 + c)}${r + 1}`;
const algorithmName = key => ({ minimax: 'MiniMax + Alpha-Beta', mcts: 'MCTS + UCT', random: 'Random' }[key]);
const number = value => Number.isFinite(value) && value >= 0 ? value.toLocaleString('en-US') : '—';

window.CaroUI = Object.freeze({
  registerAI(name, chooseMove) {
    if (!['random', 'minimax', 'mcts'].includes(name) || typeof chooseMove !== 'function') throw new Error('Invalid AI name or handler.');
    providers.set(name, chooseMove);
    updateSettings();
    if (config) { aiError = ''; render(); scheduleAI(); }
  },
  getState() { return { board: board.map(row => [...row]), turn, moves: structuredClone(moves), result: structuredClone(result) }; }
});

for (let i = 0; i < SIZE; i++) {
  const col = document.createElement('span'); col.textContent = String.fromCharCode(65 + i); document.querySelector('.column-labels').append(col);
  const row = document.createElement('span'); row.textContent = i + 1; document.querySelector('.row-labels').append(row);
}
for (let r = 0; r < SIZE; r++) for (let c = 0; c < SIZE; c++) {
  const cell = document.createElement('button'); cell.type = 'button'; cell.className = 'cell';
  cell.addEventListener('click', () => { if (!isAI(turn) && !busy && !result && !paused) place(r, c); });
  cell.addEventListener('keydown', event => {
    const offsets = { ArrowUp: [-1, 0], ArrowDown: [1, 0], ArrowLeft: [0, -1], ArrowRight: [0, 1] };
    if (!offsets[event.key]) return;
    event.preventDefault(); const [dr, dc] = offsets[event.key];
    let nr = r + dr, nc = c + dc;
    while (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE) {
      if (!cells[nr * SIZE + nc].disabled) { cells[nr * SIZE + nc].focus(); break; } nr += dr; nc += dc;
    }
  });
  cells.push(cell); $('board').append(cell);
}
function readConfig() {
  return { mode: $('mode').value, humanPiece: $('human-piece').value, agents: { X: $('x-agent').value, O: $('o-agent').value },
    depth: +$('depth').value, candidateLimit: +$('candidates').value, c: +$('exploration').value, iterations: +$('iterations').value };
}
function isAI(piece, settings = config) { return settings.mode === 'ai-ai' || (settings.mode === 'human-ai' && piece !== settings.humanPiece); }
function updateSettings() {
  const settings = readConfig();
  $('human-settings').hidden = settings.mode !== 'human-ai'; $('ai-settings').hidden = settings.mode === 'human';
  $('x-agent-wrap').hidden = !isAI('X', settings); $('o-agent-wrap').hidden = !isAI('O', settings);
  $('integration-note').hidden = !['X', 'O'].some(piece => isAI(piece, settings) && !providers.has(settings.agents[piece]));
}
function cancelThinking() { generation++; controller?.abort(); controller = null; busy = false; }
function startGame(settings) {
  cancelThinking(); config = settings; board = createBoard(); moves = []; turn = 'X'; result = null;
  startedAt = null; finishedAt = null; paused = false; aiError = '';
  render(); scheduleAI();
}
function requestReset(settings) {
  if (moves.length && !result) { pendingReset = settings; $('reset-dialog').showModal(); }
  else startGame(settings);
}
function place(row, col, metrics = null) {
  if (result || !Number.isInteger(row) || !Number.isInteger(col) || row < 0 || row >= SIZE || col < 0 || col >= SIZE || board[row][col]) throw new Error('AI returned an invalid move.');
  startedAt ??= Date.now(); board[row][col] = turn;
  moves.push({ piece: turn, row, col, coordinate: coordinate(row, col), metrics });
  const line = winningLine(board, row, col);
  if (line.length) result = { winner: turn, line };
  else if (moves.length === SIZE * SIZE) result = { winner: null, line: [] };
  if (result) finishedAt = Date.now();
  else turn = turn === 'X' ? 'O' : 'X';
  render(); scheduleAI();
}
function render() {
  const last = moves.at(-1);
  const win = new Set(result?.line.map(([r, c]) => r * SIZE + c));
  cells.forEach((cell, i) => {
    const r = Math.floor(i / SIZE), c = i % SIZE, piece = board[r][c];
    cell.textContent = piece === 'X' ? '×' : piece === 'O' ? '○' : '';
    cell.className = `cell ${piece?.toLowerCase() || ''}${last?.row === r && last?.col === c ? ' last' : ''}${win.has(i) ? ' winning' : ''}`;
    cell.disabled = !!piece || !!result || busy || paused || isAI(turn);
    cell.setAttribute('aria-label', `${coordinate(r, c)}: ${piece || 'empty cell'}`);
  });
  for (const piece of ['X', 'O']) {
    const player = $(`player-${piece.toLowerCase()}`);
    player.classList.toggle('active', !result && turn === piece);
    player.querySelector('strong').textContent = isAI(piece) ? `AI ${piece}` : `Player ${piece}`;
    $(`${piece.toLowerCase()}-caption`).textContent = isAI(piece) ? algorithmName(config.agents[piece]) : (piece === 'X' ? 'Plays first' : 'Plays second');
  }
  $('game-title').textContent = { human: 'Two players', 'human-ai': 'Human vs AI', 'ai-ai': 'AI vs AI' }[config.mode];
  let status = result ? (result.winner ? `${result.winner} wins! ${result.line.length} marks in a row.` : 'Draw. The board is full.') :
    paused ? 'Game paused.' : aiError || (isAI(turn) ? (providers.has(config.agents[turn]) ? `AI ${turn} is thinking…` : `AI ${turn}’s turn · ${algorithmName(config.agents[turn])} is not connected.`) : `${turn}’s turn. Select an empty cell.`);
  $('status').textContent = status;
  $('move-count').textContent = `${String(moves.length).padStart(2, '0')} / 81`; $('stat-moves').textContent = moves.length;
  $('undo').disabled = !moves.length || config.mode === 'ai-ai'; $('pause').hidden = config.mode !== 'ai-ai' || !!result;
  $('pause').textContent = paused ? 'Resume' : 'Pause';
  $('empty-history').hidden = moves.length > 0; $('export').disabled = !moves.length;
  $('history').replaceChildren();
  moves.slice().reverse().forEach((move, i) => {
    const li = document.createElement('li');
    const parts = [[String(moves.length - i).padStart(2, '0'), 'move-number'], [move.piece === 'X' ? '×' : '○', `piece ${move.piece.toLowerCase()}`], [move.coordinate, ''], [move.metrics ? `${number(move.metrics.timeMs)} ms` : 'Human', 'move-meta']];
    for (const [text, cls] of parts) { const span = document.createElement('span'); span.textContent = text; span.className = cls; li.append(span); }
    $('history').append(li);
  });
  const metrics = moves.findLast(move => move.metrics)?.metrics;
  $('metric-time').textContent = metrics ? `${number(metrics.timeMs)} ms` : '—';
  $('metric-depth').textContent = number(metrics?.depth); $('metric-nodes').textContent = number(metrics?.nodes);
  updateClock();
}
async function scheduleAI() {
  if (result || paused || busy || aiError || !isAI(turn)) return;
  const chooseMove = providers.get(config.agents[turn]); if (!chooseMove) return;
  busy = true; controller = new AbortController(); const signal = controller.signal, token = generation;
  startedAt ??= Date.now(); render();
  // Yield for the thinking state to paint before invoking the integration.
  await new Promise(resolve => setTimeout(resolve, 100));
  if (signal.aborted || token !== generation) return;
  try {
    const before = performance.now();
    const response = await chooseMove({ board: board.map(row => [...row]), player: turn,
      algorithm: config.agents[turn], options: { depth: config.depth, candidateLimit: config.candidateLimit, c: config.c, iterations: config.iterations }, signal });
    if (signal.aborted || token !== generation) return;
    const timeMs = Math.round((performance.now() - before) * 100) / 100;
    busy = false;
    place(response.row, response.col, { ...response.metrics, timeMs, algorithm: config.agents[turn] });
  } catch (error) {
    if (signal.aborted || token !== generation) return;
    busy = false; aiError = `AI connection error: ${error.message || 'No move received.'}`; render();
  }
}
function updateClock() {
  const seconds = startedAt ? Math.floor(((finishedAt || Date.now()) - startedAt) / 1000) : 0;
  $('elapsed').textContent = `${String(Math.floor(seconds / 60)).padStart(2, '0')}:${String(seconds % 60).padStart(2, '0')}`;
}
$('settings-form').addEventListener('change', updateSettings);
$('settings-form').addEventListener('submit', event => { event.preventDefault(); requestReset(readConfig()); });
$('restart').addEventListener('click', () => requestReset(structuredClone(config)));
$('reset-dialog').addEventListener('close', () => { if ($('reset-dialog').returnValue === 'confirm' && pendingReset) startGame(pendingReset); pendingReset = null; });
$('reset-dialog').addEventListener('cancel', () => { pendingReset = null; });
$('undo').addEventListener('click', () => {
  cancelThinking(); aiError = ''; paused = false; result = null; finishedAt = null;
  do { const move = moves.pop(); if (!move) break; board[move.row][move.col] = null; turn = move.piece; }
  while (config.mode === 'human-ai' && turn !== config.humanPiece && moves.length);
  if (!moves.length) startedAt = null;
  render(); scheduleAI();
});
$('pause').addEventListener('click', () => { paused = !paused; cancelThinking(); aiError = ''; render(); if (!paused) scheduleAI(); });
$('export').addEventListener('click', () => {
  const blob = new Blob([JSON.stringify({ boardSize: SIZE, config, moves, result }, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob); const a = document.createElement('a'); a.href = url; a.download = 'caro-game.json'; a.click(); setTimeout(() => URL.revokeObjectURL(url), 1000);
});
setInterval(updateClock, 1000);
updateSettings(); startGame(readConfig());
