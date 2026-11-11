/* Shared board rules only. AI algorithms belong in a separate integration. */
(function (root) {
  'use strict';
  const SIZE = 9;
  function createBoard() { return Array.from({ length: SIZE }, () => Array(SIZE).fill(null)); }
  function winningLine(board, row, col) {
    const piece = board[row]?.[col];
    if (!piece) return [];
    for (const [dr, dc] of [[0, 1], [1, 0], [1, 1], [1, -1]]) {
      const line = [[row, col]];
      for (const sign of [-1, 1]) {
        let r = row + dr * sign, c = col + dc * sign;
        while (r >= 0 && r < SIZE && c >= 0 && c < SIZE && board[r][c] === piece) {
          line.push([r, c]); r += dr * sign; c += dc * sign;
        }
      }
      if (line.length >= 5) return line;
    }
    return [];
  }
  const rules = { SIZE, createBoard, winningLine };
  root.CaroRules = rules;
  if (typeof module !== 'undefined') module.exports = rules;
})(globalThis);
