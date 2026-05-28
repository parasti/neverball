  var Editor = {
    gridSize: 15,
    cellSize: 20,
    grid: [],
    currentTool: 'floor',
    playerPos: {x: 2, y: 2},
    goalPos: {x: 12, y: 12},
    coins: [],

    init: function() {
      this.canvas = document.getElementById('editor-canvas');
      if (!this.canvas) return; // wait for DOM
      this.ctx = this.canvas.getContext('2d');
      this.cellSize = this.canvas.width / this.gridSize;

      for(let y=0; y<this.gridSize; y++) {
        this.grid[y] = [];
        for(let x=0; x<this.gridSize; x++) {
          this.grid[y][x] = (x > 1 && x < 13 && y > 1 && y < 13) ? 1 : 0;
        }
      }

      let isDrawing = false;

      const handleInput = (e) => {
        e.preventDefault();
        const rect = this.canvas.getBoundingClientRect();
        const clientX = e.touches ? e.touches[0].clientX : e.clientX;
        const clientY = e.touches ? e.touches[0].clientY : e.clientY;
        const x = Math.floor((clientX - rect.left) / this.cellSize);
        const y = Math.floor((clientY - rect.top) / this.cellSize);

        if (x >= 0 && x < this.gridSize && y >= 0 && y < this.gridSize) {
          if (this.currentTool === 'floor') {
            this.grid[y][x] = 1;
          } else if (this.currentTool === 'erase') {
            this.grid[y][x] = 0;
          } else if (this.currentTool === 'player') {
            this.playerPos = {x, y};
          } else if (this.currentTool === 'goal') {
            this.goalPos = {x, y};
          } else if (this.currentTool === 'coin') {
            if (!this.coins.some(c => c.x === x && c.y === y)) {
              this.coins.push({x, y});
            }
          }
          this.draw();
        }
      };

      this.canvas.addEventListener('mousedown', (e) => { isDrawing = true; handleInput(e); });
      this.canvas.addEventListener('mousemove', (e) => { if (isDrawing) handleInput(e); });
      this.canvas.addEventListener('mouseup', () => isDrawing = false);
      this.canvas.addEventListener('mouseleave', () => isDrawing = false);

      this.canvas.addEventListener('touchstart', (e) => { isDrawing = true; handleInput(e); }, {passive: false});
      this.canvas.addEventListener('touchmove', (e) => { if (isDrawing) handleInput(e); }, {passive: false});
      this.canvas.addEventListener('touchend', () => isDrawing = false);

      this.draw();
    },

    setTool: function(tool) {
      this.currentTool = tool;
    },

    draw: function() {
      if(!this.ctx) return;
      this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

      for(let y=0; y<this.gridSize; y++) {
        for(let x=0; x<this.gridSize; x++) {
          this.ctx.strokeStyle = '#444';
          this.ctx.strokeRect(x * this.cellSize, y * this.cellSize, this.cellSize, this.cellSize);

          if (this.grid[y][x] === 1) {
            this.ctx.fillStyle = '#1a6cb4';
            this.ctx.fillRect(x * this.cellSize, y * this.cellSize, this.cellSize, this.cellSize);
          }
        }
      }

      this.coins.forEach(c => {
        this.ctx.fillStyle = '#ff0';
        this.ctx.beginPath();
        this.ctx.arc(c.x * this.cellSize + this.cellSize/2, c.y * this.cellSize + this.cellSize/2, this.cellSize/3, 0, Math.PI*2);
        this.ctx.fill();
      });

      this.ctx.fillStyle = '#0f0';
      this.ctx.fillRect(this.playerPos.x * this.cellSize + 2, this.playerPos.y * this.cellSize + 2, this.cellSize - 4, this.cellSize - 4);

      this.ctx.fillStyle = '#f00';
      this.ctx.fillRect(this.goalPos.x * this.cellSize + 2, this.goalPos.y * this.cellSize + 2, this.cellSize - 4, this.cellSize - 4);
    },

    generateMapContent: function() {
      let map = `
material {
    1 default { diffuse 1.0 1.0 1.0 specular 0.0 0.0 0.0 exponent 0 fl 0 alpha_func 519 alpha_ref 0.0 }
    2 goal { diffuse 1.0 1.0 1.0 specular 0.0 0.0 0.0 exponent 0 fl 0 alpha_func 519 alpha_ref 0.0 }
}
`;

      let unit = 1.0;
      let height = 0.5;

      let b_id = 1;

      // Paths and body blocks
      for(let y=0; y<this.gridSize; y++) {
        for(let x=0; x<this.gridSize; x++) {
          if (this.grid[y][x] === 1) {
            let wx = x * unit;
            let wz = y * unit;
            map += `
path ${b_id} {
  point { ${wx} 0 ${wz} }
}
body ${b_id} {
  brush {
    # top
    plane { 0.0 1.0 0.0  ${height} }
    # bottom
    plane { 0.0 -1.0 0.0 0.0 }
    # +x
    plane { 1.0 0.0 0.0  ${unit} }
    # -x
    plane { -1.0 0.0 0.0 0.0 }
    # +z
    plane { 0.0 0.0 1.0  ${unit} }
    # -z
    plane { 0.0 0.0 -1.0 0.0 }

    texc { 1 1 0 0 1 1 1 }
  }
}
`;
            b_id++;
          }
        }
      }

      // Ball spawn
      let spawnX = this.playerPos.x * unit + (unit/2);
      let spawnZ = this.playerPos.y * unit + (unit/2);
      map += `ball { ${spawnX} ${height + 0.5} ${spawnZ} 0 }\n`;

      // Goal
      let goalX = this.goalPos.x * unit + (unit/2);
      let goalZ = this.goalPos.y * unit + (unit/2);
      map += `
goal {
    ${goalX} ${height + 0.1} ${goalZ}
    1.0 0 0 0
    1
}
`;

      // Coins
      this.coins.forEach(c => {
         let cx = c.x * unit + (unit/2);
         let cz = c.y * unit + (unit/2);
         map += `item { ${cx} ${height + 0.3} ${cz} 1 }\n`;
      });

      return map;
    },

    playMap: function() {
      let mapText = this.generateMapContent();
      if (!Neverball._module) {
          alert("Engine not loaded yet. Please click 'Play Now' and wait for the game to start before testing maps.");
          return;
      }

      // We will define this export in C
      if (!document.body.classList.contains("in-game") || !Neverball.isRunning) { alert("Please click 'Play Now' and wait for the game to start before testing maps."); return; } console.log("Calling emscripten_play_test_map with mapText:", mapText); Neverball._module.ccall("emscripten_play_test_map", null, ["string"], [mapText]); console.log("emscripten_play_test_map call finished");
    }
  };

  document.addEventListener("DOMContentLoaded", () => {
    Editor.init();
  });
