<script lang="ts">
	import Display from '$lib/canvas/Display.svelte';
	import LedPanel from '$lib/canvas/LedPanel.svelte';
	import { Canvas, Color, Font, HAlign, VAlign, BlendMode } from '$lib/canvas/Canvas.js';
	import { defaultLeds, type LedState } from '$lib/canvas/LedState.js';

	// ── fixture data ─────────────────────────────────────────────────────────

	const TRACKS = [
		{ num: 1, type: 'NOTE', pat: 1, mute: false, gates: [1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0], notes: [60,0,0,0,62,0,0,0,64,0,0,0,65,0,0,0], conds: [3,0,0,0,1,0,0,0,2,0,0,0,0,0,0,0], repeats: [1,0,0,0,2,0,0,0,3,0,0,0,1,0,0,0] },
		{ num: 2, type: 'NOTE', pat: 2, mute: false, gates: [1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0], notes: [55,0,57,0,0,0,55,0,57,0,0,0,55,0,57,0] },
		{ num: 3, type: 'CRVE', pat: 1, mute: false, gates: [1,1,0,1,0,0,1,0,1,1,0,1,0,0,0,0], notes: [48,0,0,0,0,0,0,0,48,0,0,0,0,0,0,0] },
		{ num: 4, type: 'STCH', pat: 3, mute: true,  gates: [1,0,0,1,0,1,0,0,1,0,0,1,0,1,0,0], notes: [] },
		{ num: 5, type: 'ARP',  pat: 1, mute: false, gates: [1,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0], notes: [] },
		{ num: 6, type: 'NOTE', pat: 2, mute: false, gates: [1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0], notes: [36,0,0,0,0,0,0,0,36,0,0,0,0,0,0,0] },
		{ num: 7, type: 'NOTE', pat: 1, mute: false, gates: [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0], notes: [72,0,74,0,72,0,74,0,72,0,74,0,72,0,74,0] },
		{ num: 8, type: 'QNTZ', pat: 2, mute: true,  gates: [1,1,1,0,0,0,1,1,1,0,0,0,1,1,0,0], notes: [] },
	];

	// ── shared helpers ────────────────────────────────────────────────────────

	function hdr(c: Canvas, text: string): void {
		c.setFont(Font.Tiny); c.setColorValue(Color.MediumBright);
		c.drawText(2, 6, text);
		c.setColorValue(Color.Low); c.hline(0, 7, 256);
	}

	function stepBar(c: Canvas, x: number, y: number, w: number, h: number, on: boolean, beat: boolean, cursor: boolean): void {
		c.setColorValue(on ? (beat ? Color.Bright : Color.MediumBright) : (beat ? Color.MediumLow : Color.Low));
		c.fillRect(x, y, w, h);
		if (cursor) { c.setColorValue(Color.Bright); c.rect(x - 1, y - 1, w + 2, h + 2); }
	}

	// ── DASHBOARD variations ─────────────────────────────────────────────────

	// A: beat-marked grid + type initial + per-track pattern number
	function drawDash_A(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumBright); c.drawText(2, 6, 'PLAY');
		c.setColorValue(Color.Low); c.hline(0, 7, 256);
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const y = 8 + t * 7;
			c.setColorValue(trk.mute ? Color.Low : Color.Medium);
			c.drawText(2, y + 6, String(trk.num));
			c.setColorValue(Color.Low); c.drawText(9, y + 6, trk.type[0]);
			c.setColorValue(Color.MediumLow); c.drawText(16, y + 6, String(trk.pat).padStart(2));
			for (let s = 0; s < 16; s++) {
				stepBar(c, 28 + s * 14, y + 1, 13, 5, Boolean(trk.gates[s]), s % 4 === 0, s === 4);
			}
			if (trk.mute) { c.setColorValue(Color.Low); c.drawText(226, y + 6, 'M'); }
		}
	}

	// B: raw pattern only — pure visual density, no labels
	function drawDash_B(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumBright); c.drawText(2, 6, 'PLAY');
		c.setColorValue(Color.Low); c.hline(0, 7, 256);
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const y = 8 + t * 7;
			// track num tiny
			c.setColorValue(trk.mute ? Color.Low : Color.MediumLow);
			c.drawText(1, y + 6, String(trk.num));
			// full-width bars, thicker
			for (let s = 0; s < 16; s++) {
				c.setColorValue(trk.gates[s] ? (trk.mute ? Color.Medium : Color.Bright) : Color.Low);
				c.fillRect(9 + s * 15, y, 14, 6);
			}
			// active step cursor
			c.setColorValue(Color.Bright); c.vline(9 + 4 * 15 + 7, y, 6);
		}
	}

	// C: shows note pitch as bar height inside each gate cell
	function drawDash_C(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumBright); c.drawText(2, 6, 'PLAY');
		c.setColorValue(Color.Low); c.hline(0, 7, 256);
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const y = 8 + t * 7;
			c.setColorValue(trk.mute ? Color.Low : Color.Medium); c.drawText(2, y + 6, String(trk.num));
			c.setColorValue(Color.Low); c.drawText(9, y + 6, trk.type[0]);
			for (let s = 0; s < 16; s++) {
				const on = Boolean(trk.gates[s]);
				const note = trk.notes[s] || 60;
				// map midi note 36-84 to bar height 1-5
				const h = on ? Math.max(1, Math.round(((note - 36) / 48) * 4) + 1) : 0;
				c.setColorValue(on ? Color.MediumBright : Color.Low);
				c.fillRect(18 + s * 14, y + 1, 12, 5);
				if (on) {
					c.setColorValue(Color.Bright);
					c.fillRect(18 + s * 14, y + 1 + (5 - h), 12, h);
				}
			}
		}
	}

	// D: text-heavy — shows pattern number and type prominently alongside mute
	function drawDash_D(c: Canvas): void {
		c.setFont(Font.Tiny);
		// header: global play + clock
		c.setColorValue(Color.MediumBright); c.drawText(2, 6, 'PLAY  INT');
		c.setColorValue(Color.Low); c.hline(0, 7, 256);
		// two-section layout: left=track info, right=mini gate grid
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const y = 8 + t * 7;
			c.setColorValue(trk.mute ? Color.Low : Color.Bright); c.drawText(2, y + 6, String(trk.num));
			c.setColorValue(Color.MediumLow); c.drawText(10, y + 6, trk.type.slice(0, 4));
			c.setColorValue(Color.Medium); c.drawText(38, y + 6, String(trk.pat).padStart(2, '0'));
			if (trk.mute) { c.setColorValue(Color.Low); c.drawText(52, y + 6, 'MUT'); }
			// compact 16-step row (small dots)
			for (let s = 0; s < 16; s++) {
				c.setColorValue(trk.gates[s] ? Color.Bright : Color.Low);
				c.fillRect(68 + s * 11, y + 2, 9, 3);
			}
		}
	}

	// ── PERFORM variations ───────────────────────────────────────────────────

	// A: 8 columns, 4×4 pattern mini-grid
	function drawPerf_A(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'PERFORM  PLAY  INT');
		const W = 30, G = 2;
		for (let t = 0; t < 8; t++) {
			const x = t * (W + G) + 1;
			c.setColorValue(Color.Medium); c.drawTextAligned(x, 9, W, 7, HAlign.Center, VAlign.Center, String(t + 1));
			c.setColorValue(Color.Bright); c.drawTextAligned(x, 17, W, 9, HAlign.Center, VAlign.Center, String(TRACKS[t].pat).padStart(2, '0'));
			for (let p = 0; p < 16; p++) {
				c.setColorValue(p === TRACKS[t].pat - 1 ? Color.Bright : Color.Low);
				c.fillRect(x + (p % 4) * 7, 28 + Math.floor(p / 4) * 6, 6, 5);
			}
			c.setColorValue(TRACKS[t].mute ? Color.MediumBright : Color.Low);
			c.drawTextAligned(x, 54, W, 8, HAlign.Center, VAlign.Center, TRACKS[t].mute ? 'MUT' : 'UNM');
		}
	}

	// B: 8 rows — each track is a horizontal strip with 16 pattern slots
	function drawPerf_B(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'PERFORM  PLAY  INT');
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const y = 9 + t * 7;
			c.setColorValue(trk.mute ? Color.Low : Color.Medium); c.drawText(2, y + 6, String(trk.num));
			c.setColorValue(Color.Low); c.drawText(9, y + 6, trk.type[0]);
			// 16 pattern slots as small cells
			for (let p = 0; p < 16; p++) {
				const active = p === trk.pat - 1;
				c.setColorValue(active ? Color.Bright : Color.Low);
				c.fillRect(18 + p * 14, y + 1, 13, 5);
				if (active) { c.setColorValue(Color.Bright); c.drawText(18 + p * 14 + 3, y + 6, String(p + 1)); }
			}
			if (trk.mute) { c.setColorValue(Color.MediumLow); c.drawText(247, y + 6, 'M'); }
		}
	}

	// C: minimalist — track + big current pattern + incoming pattern
	function drawPerf_C(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'PERFORM  PLAY  INT');
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const y = 9 + t * 7;
			c.setColorValue(trk.mute ? Color.Low : Color.MediumLow); c.drawText(2, y + 6, String(trk.num));
			// current pattern (prominent)
			c.setFont(Font.Normal);
			c.setColorValue(trk.mute ? Color.Low : Color.Bright);
			c.drawTextAligned(10, y - 2, 24, 12, HAlign.Center, VAlign.Center, String(trk.pat).padStart(2,'0'));
			c.setFont(Font.Tiny);
			// arrow + queued pattern
			c.setColorValue(Color.Low); c.drawText(36, y + 6, '→');
			c.setColorValue(Color.Medium); c.drawText(44, y + 6, String((trk.pat % 16) + 1).padStart(2,'0'));
			// mute
			if (trk.mute) { c.setColorValue(Color.MediumBright); c.drawText(60, y + 6, 'MUTE'); }
			// fill bar placeholder
			c.setColorValue(Color.Low); c.fillRect(80, y + 2, 170, 2);
		}
	}

	// D: 2×4 grid layout — more space per track, fill bars visible
	function drawPerf_D(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'PERFORM');
		const W = 60, H = 27;
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t];
			const col = t % 2, row = Math.floor(t / 2);
			const x = col * (W + 4) + 2, y = 9 + row * (H + 2);
			c.setColorValue(Color.Low); c.rect(x, y, W, H);
			c.setColorValue(trk.mute ? Color.Low : Color.Medium);
			c.drawText(x + 2, y + 7, String(trk.num));
			c.setFont(Font.Normal);
			c.setColorValue(trk.mute ? Color.Low : Color.Bright);
			c.drawTextAligned(x, y + 8, W, 14, HAlign.Center, VAlign.Center, String(trk.pat).padStart(2,'0'));
			c.setFont(Font.Tiny);
			if (trk.mute) { c.setColorValue(Color.Medium); c.drawTextAligned(x, y + H - 8, W, 8, HAlign.Center, VAlign.Center, 'MUTE'); }
		}
	}

	// ── TRACK EDIT variations ────────────────────────────────────────────────

	// A: tall uniform cells — gate on = filled bright
	function drawEdit_A(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TRK 1  NOTE  PAT 01');
		const steps = TRACKS[0].gates;
		for (let s = 0; s < 16; s++) {
			const x = 1 + s * 16; const on = Boolean(steps[s]);
			c.setColorValue(on ? Color.MediumBright : Color.Low); c.fillRect(x, 9, 14, 44);
			if (s === 3) { c.setColorValue(Color.Bright); c.rect(x - 1, 8, 16, 46); }
			c.setColorValue(Color.MediumLow); c.drawText(x + (s < 9 ? 4 : 1), 56, String(s + 1));
		}
		c.setColorValue(Color.Medium); c.drawText(2, 63, 'GATE');
	}

	// B: bottom-anchored note-height bars — gate=filled, note=bar height
	function drawEdit_B(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TRK 1  NOTE  PAT 01');
		const steps = TRACKS[0]; const maxH = 40;
		const noteH = [32,0,0,0,28,0,0,0,36,0,0,0,24,0,0,0];
		for (let s = 0; s < 16; s++) {
			const x = 1 + s * 16; const on = Boolean(steps.gates[s]); const h = on ? (noteH[s] || 20) : 4;
			c.setColorValue(Color.Low); c.fillRect(x, 9, 14, maxH);
			c.setColorValue(on ? Color.MediumBright : Color.MediumLow);
			c.fillRect(x, 9 + maxH - h, 14, h);
			if (on) { c.setColorValue(Color.Bright); c.hline(x, 9 + maxH - h, 14); }
			if (s === 3) { c.setColorValue(Color.Bright); c.vline(x + 7, 9, maxH); }
			c.setColorValue(Color.Low); c.drawText(x + (s < 9 ? 4 : 1), 53, String(s + 1));
		}
		// pitch axis hint
		c.setColorValue(Color.Low); c.vline(0, 9, maxH);
		c.drawText(1, 14, 'C5'); c.drawText(1, 30, 'C4'); c.drawText(1, 46, 'C3');
		c.setColorValue(Color.Medium); c.drawText(2, 62, 'NOTE');
	}

	// C: 2 rows of 8 steps — more width per step, shows note name inside
	function drawEdit_C(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TRK 1  NOTE  PAT 01');
		const notes = ['C4','','','','E4','','','','G4','','','','A4','','',''];
		const steps = TRACKS[0].gates;
		for (let row = 0; row < 2; row++) {
			for (let col = 0; col < 8; col++) {
				const s = row * 8 + col; const x = 2 + col * 31; const y = 10 + row * 26;
				const on = Boolean(steps[s]);
				c.setColorValue(on ? Color.MediumBright : Color.Low); c.fillRect(x, y, 29, 20);
				if (s === 3) { c.setColorValue(Color.Bright); c.rect(x - 1, y - 1, 31, 22); }
				c.setColorValue(on ? Color.Bright : Color.MediumLow);
				c.drawTextAligned(x, y, 29, 10, HAlign.Center, VAlign.Center, String(s + 1));
				if (on && notes[s]) {
					c.setColorValue(Color.Low);
					c.drawTextAligned(x, y + 10, 29, 10, HAlign.Center, VAlign.Center, notes[s]);
				}
			}
		}
		c.setColorValue(Color.Medium); c.drawText(2, 62, 'GATE + NOTE');
	}

	// D: compact top strip for gate, large central area for the active layer value
	function drawEdit_D(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TRK 1  NOTE  PAT 01');
		const steps = TRACKS[0].gates;
		// compact gate row at top
		for (let s = 0; s < 16; s++) {
			c.setColorValue(steps[s] ? Color.MediumBright : Color.Low);
			c.fillRect(1 + s * 16, 9, 14, 8);
			if (s === 3) { c.setColorValue(Color.Bright); c.rect(s * 16, 8, 16, 10); }
		}
		c.hline(0, 18, 256);
		// large value display for selected step
		c.setFont(Font.Normal);
		c.setColorValue(Color.Bright);
		c.drawTextAligned(0, 20, 256, 24, HAlign.Center, VAlign.Center, 'C4');
		c.setFont(Font.Tiny);
		c.setColorValue(Color.Low);
		c.drawTextAligned(0, 44, 256, 8, HAlign.Center, VAlign.Center, 'STEP 4 / NOTE');
		// mini keyboard hint
		const whites = [0,1,2,3,4,5,6];
		for (let i = 0; i < 7; i++) {
			c.setColorValue(i === 2 ? Color.Bright : Color.Medium);
			c.fillRect(90 + i * 11, 53, 10, 8);
			c.setColorValue(Color.Low); c.vline(90 + i * 11, 53, 8);
		}
		c.setColorValue(Color.Low);
		c.drawText(2, 63, 'NOTE');
	}

	// ── TRACK CONFIG variations ──────────────────────────────────────────────

	const CFG_ROWS: [string, string][] = [
		['MODE','NOTE'],['PLAY','FORWARD'],['FILL','NONE'],
		['DIV','1/16'],['RESET','4 BAR'],['OCTAVE','+0'],
		['TRANSP','+0'],['CV OUT','CV 1'],['GT OUT','GATE 1'],
	];

	// A: flat list, selected item highlighted
	function drawCfg_A(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TRK 1 CONFIG');
		for (let i = 0; i < CFG_ROWS.length; i++) {
			const y = 9 + i * 6; const sel = i === 3;
			if (sel) { c.setColorValue(Color.Low); c.fillRect(0, y, 256, 6); }
			c.setColorValue(sel ? Color.Bright : Color.Medium); c.drawText(4, y + 5, CFG_ROWS[i][0]);
			c.setColorValue(sel ? Color.Bright : Color.MediumLow); c.drawText(70, y + 5, CFG_ROWS[i][1]);
		}
	}

	// B: grouped sections with dividers
	function drawCfg_B(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TRK 1 CONFIG');
		const sections: { title: string; rows: [string, string][] }[] = [
			{ title: 'TYPE', rows: [['MODE','NOTE'],['PLAY','FORWARD'],['FILL','NONE']] },
			{ title: 'TIMING', rows: [['DIV','1/16'],['RESET','4 BAR']] },
			{ title: 'TUNING', rows: [['OCTAVE','+0'],['TRANSP','+0']] },
			{ title: 'OUTPUT', rows: [['CV','CV 1'],['GATE','GATE 1']] },
		];
		let y = 9;
		for (const sec of sections) {
			c.setColorValue(Color.Low); c.drawText(2, y + 5, sec.title); c.hline(30, y + 2, 226);
			y += 7;
			for (const [label, value] of sec.rows) {
				const sel = label === 'DIV';
				c.setColorValue(sel ? Color.Bright : Color.Medium); c.drawText(6, y + 5, label);
				c.setColorValue(sel ? Color.Bright : Color.MediumLow); c.drawText(60, y + 5, value);
				y += 6;
			}
		}
	}

	// C: two-column with clear value emphasis
	function drawCfg_C(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TRK 1 CONFIG');
		c.setColorValue(Color.Low); c.vline(100, 8, 56);
		const col1 = CFG_ROWS.slice(0, 5), col2 = CFG_ROWS.slice(4);
		for (let i = 0; i < 5; i++) {
			const y = 9 + i * 11; const sel = i === 3;
			c.setColorValue(sel ? Color.Medium : Color.Low); c.drawText(4, y + 5, col1[i][0]);
			c.setColorValue(sel ? Color.Bright : Color.MediumBright);
			c.setFont(Font.Normal); c.drawText(4, y + 12, col1[i][1]); c.setFont(Font.Tiny);
		}
		for (let i = 0; i < col2.length && i < 5; i++) {
			const y = 9 + i * 11;
			c.setColorValue(Color.Low); c.drawText(104, y + 5, col2[i][0]);
			c.setColorValue(Color.MediumBright);
			c.setFont(Font.Normal); c.drawText(104, y + 12, col2[i][1]); c.setFont(Font.Tiny);
		}
	}

	// D: tab-style sections, showing content of active tab
	function drawCfg_D(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TRK 1 CONFIG');
		const tabs = ['TYPE', 'TIME', 'TUNE', 'OUT'];
		for (let i = 0; i < tabs.length; i++) {
			const x = i * 64; const active = i === 1;
			c.setColorValue(active ? Color.Low : Color.Low); c.fillRect(x, 8, 63, 8);
			if (active) { c.setColorValue(Color.Bright); c.rect(x, 8, 63, 8); }
			c.setColorValue(active ? Color.Bright : Color.Medium);
			c.drawTextAligned(x, 8, 63, 8, HAlign.Center, VAlign.Center, tabs[i]);
		}
		// content of TIMING tab
		const timingRows: [string, string][] = [['DIV','1/16'],['RESET','4 BAR'],['SLIDE','0ms']];
		for (let i = 0; i < timingRows.length; i++) {
			const y = 20 + i * 14; const sel = i === 0;
			if (sel) { c.setColorValue(Color.Low); c.fillRect(0, y, 256, 13); }
			c.setColorValue(sel ? Color.Bright : Color.Medium);
			c.drawText(4, y + 6, timingRows[i][0]);
			c.setFont(Font.Normal);
			c.setColorValue(sel ? Color.Bright : Color.MediumBright);
			c.drawText(80, y + 10, timingRows[i][1]);
			c.setFont(Font.Tiny);
		}
	}

	// ── SONG variations ──────────────────────────────────────────────────────

	const SONG_DATA = [
		{ pats: [1,1,1,1,1,1,1,1], reps: 4 },
		{ pats: [2,1,1,1,2,1,1,1], reps: 2 },
		{ pats: [2,2,2,2,2,2,2,2], reps: 4 },
		{ pats: [1,2,3,2,1,2,3,2], reps: 8 },
		{ pats: [3,3,1,1,3,3,1,1], reps: 2 },
	];

	// A: slot × track grid
	function drawSong_A(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'SONG');
		c.setColorValue(Color.Low);
		c.drawText(2, 14, '#');
		for (let t = 0; t < 8; t++) c.drawText(18 + t * 28, 14, String(t + 1));
		c.drawText(243, 14, 'R'); c.hline(0, 15, 256);
		for (let r = 0; r < SONG_DATA.length; r++) {
			const y = 17 + r * 9; const { pats, reps } = SONG_DATA[r]; const active = r === 1;
			if (active) { c.setColorValue(Color.Low); c.fillRect(0, y, 256, 8); }
			c.setColorValue(active ? Color.Bright : Color.Medium); c.drawText(2, y + 7, String(r + 1));
			for (let t = 0; t < 8; t++) {
				c.setColorValue(active ? Color.Bright : Color.MediumLow);
				c.drawText(18 + t * 28, y + 7, String(pats[t]).padStart(2));
			}
			c.setColorValue(active ? Color.MediumBright : Color.Low);
			c.drawText(243, y + 7, reps + 'x');
		}
	}

	// B: timeline view — horizontal blocks per track
	function drawSong_B(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'SONG');
		// track labels
		for (let t = 0; t < 8; t++) {
			c.setColorValue(Color.Low); c.drawText(2, 11 + t * 7, String(t + 1));
		}
		c.vline(10, 8, 56);
		// timeline blocks (each slot = 28px wide)
		const COLORS = [Color.Low, Color.Medium, Color.MediumBright, Color.Bright] as const;
		for (let r = 0; r < SONG_DATA.length; r++) {
			const { pats, reps } = SONG_DATA[r];
			const bw = reps * 6 - 1;
			for (let t = 0; t < 8; t++) {
				const x = 12 + r * 30; const y = 9 + t * 7;
				const col = COLORS[Math.min(pats[t], 3)];
				c.setColorValue(r === 1 ? Color.MediumBright : col);
				c.fillRect(x, y, bw > 28 ? 28 : bw, 5);
				c.setColorValue(Color.Low); c.drawText(x + 2, y + 5, String(pats[t]));
			}
		}
	}

	// C: condensed chain — just the sequence of slots, focus on order + repeats
	function drawSong_C(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'SONG');
		for (let r = 0; r < SONG_DATA.length; r++) {
			const { pats, reps } = SONG_DATA[r]; const y = 10 + r * 10; const active = r === 1;
			if (active) { c.setColorValue(Color.Low); c.fillRect(0, y, 256, 9); }
			// slot number
			c.setColorValue(active ? Color.Bright : Color.MediumLow);
			c.drawText(2, y + 7, String(r + 1));
			// compact pattern summary (just show unique values)
			const summary = pats.map(p => String(p)).join(' ');
			c.setColorValue(active ? Color.MediumBright : Color.Low);
			c.drawText(12, y + 7, summary);
			// repeat count
			c.setColorValue(active ? Color.Bright : Color.Medium);
			c.drawText(225, y + 7, '×' + reps);
		}
		c.setColorValue(Color.Low); c.drawText(2, 63, '→ ADD  DEL  MOVE');
	}

	// D: one slot focused, full detail
	function drawSong_D(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'SONG  SLOT 2 / 5');
		// slot nav
		for (let r = 0; r < 5; r++) {
			const x = 2 + r * 16; const active = r === 1;
			c.setColorValue(active ? Color.Bright : Color.Low);
			c.fillRect(x, 9, 14, 5);
		}
		c.hline(0, 15, 256);
		// per-track pattern assignment for current slot
		c.setColorValue(Color.Low); c.drawText(2, 21, 'TRACK'); c.drawText(40, 21, 'PATTERN');
		c.hline(0, 22, 100);
		for (let t = 0; t < 8; t++) {
			const y = 24 + t * 5; const active = t === 0;
			c.setColorValue(active ? Color.Bright : Color.Medium); c.drawText(2, y + 4, String(t + 1));
			c.setColorValue(active ? Color.Bright : Color.MediumLow);
			c.drawText(40, y + 4, String(SONG_DATA[1].pats[t]).padStart(2, '0'));
		}
		c.setColorValue(Color.Medium); c.drawText(110, 26, '×' + SONG_DATA[1].reps + ' repeats');
	}

	// ── SETTINGS variations ──────────────────────────────────────────────────

	const SECS = ['PROJECT', 'CLOCK', 'ROUTING', 'MIDI', 'SCALES', 'SYSTEM'];

	// A: two-panel (section list + detail)
	function drawSet_A(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'SETTINGS');
		for (let i = 0; i < SECS.length; i++) {
			c.setColorValue(i === 0 ? Color.Bright : Color.Medium);
			c.drawText(2, 16 + i * 9, SECS[i]);
		}
		c.setColorValue(Color.Low); c.vline(55, 8, 56);
		const items = ['Name: MY PROJECT', 'Scale: CHROMATIC', 'Root: C', 'Save', 'Load'];
		for (let i = 0; i < items.length; i++) {
			c.setColorValue(i === 0 ? Color.MediumBright : Color.Medium);
			c.drawText(59, 16 + i * 9, items[i]);
		}
	}

	// B: flat list with section headers as dividers
	function drawSet_B(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'SETTINGS');
		const all: { sec?: string; label?: string; value?: string }[] = [
			{ sec: 'PROJECT' },
			{ label: 'Name', value: 'MY PROJECT' }, { label: 'Scale', value: 'CHROMATIC' },
			{ sec: 'CLOCK' },
			{ label: 'Source', value: 'INT' }, { label: 'PPQN', value: '24' }, { label: 'Swing', value: '0%' },
			{ sec: 'MIDI' },
			{ label: 'Out 1', value: 'CH 1' }, { label: 'Out 2', value: 'CH 2' },
		];
		let y = 9;
		for (const row of all) {
			if (row.sec) {
				c.setColorValue(Color.Low); c.hline(0, y, 256);
				c.setColorValue(Color.MediumLow); c.drawText(2, y + 5, row.sec); y += 7;
			} else {
				const sel = row.label === 'Source';
				if (sel) { c.setColorValue(Color.Low); c.fillRect(0, y, 256, 6); }
				c.setColorValue(sel ? Color.Bright : Color.Medium); c.drawText(8, y + 5, row.label!);
				c.setColorValue(sel ? Color.Bright : Color.MediumLow); c.drawText(90, y + 5, row.value!);
				y += 6;
			}
		}
	}

	// C: full-screen single section — encoder scrolls between sections
	function drawSet_C(c: Canvas): void {
		c.setFont(Font.Tiny);
		// section indicator dots at top
		for (let i = 0; i < SECS.length; i++) {
			c.setColorValue(i === 0 ? Color.Bright : Color.Low);
			c.fillRect(2 + i * 8, 2, 6, 3);
		}
		// big section name
		c.setFont(Font.Normal);
		c.setColorValue(Color.Bright);
		c.drawTextAligned(0, 6, 256, 12, HAlign.Center, VAlign.Center, 'PROJECT');
		c.setFont(Font.Tiny);
		c.setColorValue(Color.Low); c.hline(0, 19, 256);
		const items: [string, string][] = [
			['Name', 'MY PROJECT'], ['Scale', 'CHROMATIC'],
			['Root', 'C'], ['Save', ''], ['Load', ''],
		];
		for (let i = 0; i < items.length; i++) {
			const y = 22 + i * 9; const sel = i === 0;
			if (sel) { c.setColorValue(Color.Low); c.fillRect(0, y - 1, 256, 8); }
			c.setColorValue(sel ? Color.Bright : Color.Medium); c.drawText(4, y + 6, items[i][0]);
			c.setColorValue(sel ? Color.Bright : Color.MediumLow); c.drawText(80, y + 6, items[i][1]);
		}
	}

	// D: breadcrumb — shows Settings > Clock > Source
	function drawSet_D(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.Low); c.drawText(2, 6, 'SET > CLOCK > SOURCE');
		c.hline(0, 7, 256);
		// large current value
		c.setFont(Font.Normal);
		c.setColorValue(Color.Bright);
		c.drawTextAligned(0, 10, 256, 28, HAlign.Center, VAlign.Center, 'INT');
		c.setFont(Font.Tiny);
		c.setColorValue(Color.Low); c.hline(0, 40, 256);
		// other options
		const opts = ['INT', 'MIDI', 'USB', 'CV'];
		for (let i = 0; i < opts.length; i++) {
			c.setColorValue(i === 0 ? Color.MediumBright : Color.Low);
			c.drawTextAligned(i * 64, 42, 63, 8, HAlign.Center, VAlign.Center, opts[i]);
		}
		// hint
		c.setColorValue(Color.Low); c.hline(0, 52, 256);
		c.drawTextAligned(0, 54, 256, 8, HAlign.Center, VAlign.Center, 'ENCODER: select  PAGE+←: back');
	}

	// ── TEMPO variations ─────────────────────────────────────────────────────

	// A: large centered number
	function drawTempo_A(c: Canvas): void {
		c.setFont(Font.Normal); c.setColorValue(Color.Bright);
		c.drawTextAligned(0, 8, 256, 32, HAlign.Center, VAlign.Center, '120.0');
		c.setFont(Font.Tiny); c.setColorValue(Color.Low);
		c.drawTextAligned(0, 40, 256, 8, HAlign.Center, VAlign.Center, 'BPM');
		c.hline(0, 50, 256);
		c.setColorValue(Color.MediumLow);
		c.drawTextAligned(0, 52, 128, 10, HAlign.Center, VAlign.Center, 'F0: TAP');
		c.drawTextAligned(128, 52, 128, 10, HAlign.Center, VAlign.Center, 'F1/F2: NUDGE');
	}

	// B: BPM + swing, two parameters
	function drawTempo_B(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TEMPO');
		c.setFont(Font.Normal); c.setColorValue(Color.Bright);
		c.drawTextAligned(0, 9, 140, 30, HAlign.Center, VAlign.Center, '120.0');
		c.setFont(Font.Tiny); c.setColorValue(Color.Low);
		c.drawTextAligned(0, 39, 140, 8, HAlign.Center, VAlign.Center, 'BPM');
		c.setColorValue(Color.Low); c.vline(140, 8, 48);
		c.setFont(Font.Normal); c.setColorValue(Color.MediumBright);
		c.drawTextAligned(140, 9, 116, 30, HAlign.Center, VAlign.Center, '15%');
		c.setFont(Font.Tiny); c.setColorValue(Color.Low);
		c.drawTextAligned(140, 39, 116, 8, HAlign.Center, VAlign.Center, 'SWING');
		c.hline(0, 56, 256);
		c.setColorValue(Color.MediumLow);
		c.drawTextAligned(0, 57, 256, 7, HAlign.Center, VAlign.Center, 'ENCODER: BPM   SHIFT+ENC: SWING   F0: TAP');
	}

	// C: minimal — just the number, fills the screen
	function drawTempo_C(c: Canvas): void {
		c.setFont(Font.Normal); c.setColorValue(Color.Bright);
		c.drawTextAligned(0, 0, 256, 56, HAlign.Center, VAlign.Center, '120.0');
		c.setFont(Font.Tiny); c.setColorValue(Color.Low);
		c.drawTextAligned(0, 57, 256, 7, HAlign.Center, VAlign.Center, 'BPM');
	}

	// D: tap tempo — shows recent tap intervals as a beat grid
	function drawTempo_D(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TEMPO — TAP');
		c.setFont(Font.Normal); c.setColorValue(Color.Bright);
		c.drawTextAligned(0, 9, 100, 30, HAlign.Center, VAlign.Center, '120');
		c.setFont(Font.Tiny); c.setColorValue(Color.Low);
		c.drawTextAligned(0, 39, 100, 8, HAlign.Center, VAlign.Center, 'BPM');
		// tap pulse dots
		const taps = [1, 0.9, 0.95, 1, 0]; // relative timing deviation
		for (let i = 0; i < 4; i++) {
			const x = 110 + i * 36;
			c.setColorValue(i < 3 ? Color.MediumBright : Color.Low);
			c.fillRect(x, 18, 28, 22);
			c.setColorValue(Color.Low);
			c.drawText(x + 8, 46, i < 3 ? 'TAP' : '...');
		}
	}

	// ── QUICK EDIT variations ────────────────────────────────────────────────

	// A: floating center overlay
	function drawQE_A(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TRK 1  NOTE  PAT 01');
		c.setColorValue(Color.Low);
		for (let s = 0; s < 16; s++) c.fillRect(1 + s * 16, 9, 14, 44);
		c.fillRect(20, 16, 216, 32); c.setColorValue(Color.MediumLow); c.rect(20, 16, 216, 32);
		const params: [string, string][] = [['FIRST','01'],['LAST','16'],['MODE','FWD'],['DIV','1/16'],['RESET','4BAR']];
		for (let i = 0; i < params.length; i++) {
			const x = 28 + i * 41;
			c.setColorValue(i === 0 ? Color.Bright : Color.Medium); c.drawTextAligned(x, 18, 38, 8, HAlign.Center, VAlign.Center, params[i][0]);
			c.setColorValue(i === 0 ? Color.Bright : Color.MediumBright); c.drawTextAligned(x, 28, 38, 10, HAlign.Center, VAlign.Center, params[i][1]);
		}
		c.setColorValue(Color.Low); c.drawText(28, 62, 'STEP 8-12');
	}

	// B: bottom bar — doesn't obscure the step grid
	function drawQE_B(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TRK 1  NOTE  PAT 01');
		const steps = TRACKS[0].gates;
		for (let s = 0; s < 16; s++) {
			c.setColorValue(steps[s] ? Color.MediumBright : Color.Low); c.fillRect(1 + s * 16, 9, 14, 36);
		}
		c.setColorValue(Color.Low); c.hline(0, 46, 256); c.fillRect(0, 47, 256, 17);
		c.setColorValue(Color.MediumLow); c.hline(0, 47, 256);
		const params: [string, string][] = [['FST','01'],['LST','16'],['RUN','FWD'],['DIV','1/16'],['RST','4B']];
		for (let i = 0; i < params.length; i++) {
			const x = 4 + i * 50;
			c.setColorValue(i === 0 ? Color.Bright : Color.Low); c.drawText(x, 53, params[i][0]);
			c.setColorValue(i === 0 ? Color.Bright : Color.MediumBright); c.drawText(x, 61, params[i][1]);
		}
	}

	// C: full-screen takeover — no background, focused editing
	function drawQE_C(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'QUICK EDIT — TRK 1  PAT 01');
		const params: [string, string, string][] = [
			['FIRST STEP', '01', 'step 8'],
			['LAST STEP',  '16', 'step 9'],
			['RUN MODE',   'FWD','step 10'],
			['DIVISOR',   '1/16','step 11'],
			['RESET',     '4 BAR','step 12'],
		];
		for (let i = 0; i < params.length; i++) {
			const y = 10 + i * 10; const sel = i === 0;
			if (sel) { c.setColorValue(Color.Low); c.fillRect(0, y, 256, 10); }
			c.setColorValue(Color.Low); c.drawText(185, y + 7, params[i][2]);
			c.setColorValue(sel ? Color.Medium : Color.Low); c.drawText(4, y + 7, params[i][0]);
			c.setFont(Font.Normal);
			c.setColorValue(sel ? Color.Bright : Color.MediumLow);
			c.drawText(100, y + 9, params[i][1]);
			c.setFont(Font.Tiny);
		}
	}

	// D: minimal popup — encoder label + value, near current step
	function drawQE_D(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'TRK 1  NOTE  PAT 01');
		const steps = TRACKS[0].gates;
		for (let s = 0; s < 16; s++) {
			c.setColorValue(steps[s] ? Color.MediumBright : Color.Low); c.fillRect(1 + s * 16, 9, 14, 44);
			if (s === 3) { c.setColorValue(Color.Bright); c.rect(s * 16, 8, 16, 46); }
		}
		// tiny popup at top-right
		c.setColorValue(Color.Low); c.fillRect(160, 9, 95, 22);
		c.setColorValue(Color.MediumLow); c.rect(160, 9, 95, 22);
		c.setColorValue(Color.Low); c.drawText(164, 16, 'FIRST');
		c.setColorValue(Color.Bright); c.drawText(200, 16, '01');
		c.setColorValue(Color.Low); c.drawText(164, 24, 'LAST');
		c.setColorValue(Color.MediumBright); c.drawText(200, 24, '16');
		c.setColorValue(Color.Low); c.drawText(2, 62, 'PAGE+STEP 8-12');
	}

	// ── Elektron-inspired variations (batch 1) ──────────────────────────────

	// Dashboard: inverted bright row for focused track; 4-char type labels; beat-aware gate dots
	function drawDash_Elektron1(c: Canvas): void {
		c.setFont(Font.Tiny);
		hdr(c, 'PLAY  INT');
		const FOCUS = 1;
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const y = 8 + t * 7;
			const sel = t === FOCUS;
			if (sel) {
				c.setColorValue(Color.Bright); c.fillRect(0, y, 256, 7);
				c.setColorValue(Color.Low);
				c.drawText(2, y + 6, String(trk.num));
				c.drawText(10, y + 6, trk.type.slice(0, 4));
				c.drawText(36, y + 6, 'P' + String(trk.pat).padStart(2, '0'));
				for (let s = 0; s < 16; s++) {
					if (trk.gates[s]) c.fillRect(56 + s * 12, y + 1, 11, 5);
				}
			} else {
				c.setColorValue(trk.mute ? Color.Low : Color.MediumLow);
				c.drawText(2, y + 6, String(trk.num));
				c.setColorValue(Color.Low); c.drawText(10, y + 6, trk.type.slice(0, 4));
				c.setColorValue(trk.mute ? Color.Low : Color.Medium);
				c.drawText(36, y + 6, 'P' + String(trk.pat).padStart(2, '0'));
				for (let s = 0; s < 16; s++) {
					c.setColorValue(trk.gates[s] ? (s % 4 === 0 ? Color.Medium : Color.MediumLow) : Color.Low);
					c.fillRect(56 + s * 12, y + 1, 11, 5);
				}
				if (trk.mute) { c.setColorValue(Color.Low); c.drawText(248, y + 6, 'M'); }
			}
		}
	}

	// Track Edit: inverted cursor cell; outlined-only off-steps; brightness encodes beat
	function drawEdit_Elektron1(c: Canvas): void {
		c.setFont(Font.Tiny);
		hdr(c, 'TRK 1  NOTE  P01');
		const steps = TRACKS[0].gates;
		const CURSOR = 3;
		for (let s = 0; s < 16; s++) {
			const x = 1 + s * 16; const on = Boolean(steps[s]);
			const isBeat = s % 4 === 0; const isCursor = s === CURSOR;
			if (isCursor) {
				c.setColorValue(Color.Bright); c.fillRect(x, 9, 14, 44);
				c.setColorValue(Color.Low);
				c.drawTextAligned(x, 44, 14, 8, HAlign.Center, VAlign.Center, String(s + 1));
				if (on) c.fillRect(x + 2, 11, 10, 6);
			} else if (on) {
				c.setColorValue(isBeat ? Color.Bright : Color.MediumBright);
				c.fillRect(x, 9, 14, 44);
				c.setColorValue(Color.Low);
				c.drawTextAligned(x, 44, 14, 8, HAlign.Center, VAlign.Center, String(s + 1));
			} else {
				c.setColorValue(isBeat ? Color.MediumLow : Color.Low);
				c.rect(x, 9, 14, 44);
				c.drawTextAligned(x, 44, 14, 8, HAlign.Center, VAlign.Center, String(s + 1));
			}
		}
		c.setColorValue(Color.Medium); c.drawText(2, 63, 'GATE');
	}

	// Perform: inverted row for focused track; mini 16-slot pattern strip per track
	function drawPerf_Elektron1(c: Canvas): void {
		c.setFont(Font.Tiny);
		hdr(c, 'PERFORM  PLAY  INT');
		const FOCUS = 1;
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const y = 9 + t * 7;
			const sel = t === FOCUS;
			if (sel) {
				c.setColorValue(Color.Bright); c.fillRect(0, y, 256, 7);
				c.setColorValue(Color.Low);
				c.drawText(2, y + 6, String(trk.num));
				c.drawText(10, y + 6, trk.type.slice(0, 4));
				c.drawText(36, y + 6, String(trk.pat).padStart(2, '0'));
				for (let p = 0; p < 16; p++) {
					// active slot left bright; others darkened with Low fill
					if (p !== trk.pat - 1) c.fillRect(56 + p * 12, y + 1, 11, 5);
				}
			} else {
				c.setColorValue(trk.mute ? Color.Low : Color.MediumLow);
				c.drawText(2, y + 6, String(trk.num));
				c.setColorValue(Color.Low); c.drawText(10, y + 6, trk.type.slice(0, 4));
				c.setColorValue(trk.mute ? Color.Low : Color.Medium);
				c.drawText(36, y + 6, String(trk.pat).padStart(2, '0'));
				for (let p = 0; p < 16; p++) {
					c.setColorValue(p === trk.pat - 1 ? Color.Bright : Color.Low);
					c.fillRect(56 + p * 12, y + 1, 11, 5);
				}
				if (trk.mute) { c.setColorValue(Color.Low); c.drawText(248, y + 6, 'M'); }
			}
		}
	}

	// ── Teenage Engineering-inspired variations (batch 1) ───────────────────

	// Dashboard: focused track gets large bars (dominant); others compressed to dot-strip
	function drawDash_TE2(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.Low); c.drawText(2, 6, 'PLAY');
		c.setColorValue(Color.Low); c.hline(0, 7, 256);
		const FOCUS = 1;
		const trk = TRACKS[FOCUS];
		// subordinate track label
		c.setColorValue(Color.MediumLow); c.drawText(2, 14, String(trk.num)); c.drawText(9, 14, trk.type[0]);
		// dominant: step bars for focused track
		const BASEy = 36, BH = 22;
		for (let s = 0; s < 16; s++) {
			const x = 2 + s * 16; const on = Boolean(trk.gates[s]);
			c.setColorValue(on ? (s % 4 === 0 ? Color.Bright : Color.MediumBright) : Color.Low);
			c.fillRect(x, BASEy - (on ? BH : 1), 14, on ? BH : 1);
		}
		c.setColorValue(Color.Low); c.hline(0, 38, 256);
		// other 7 tracks: compressed dot-matrix strip
		for (let t = 0; t < 8; t++) {
			if (t === FOCUS) continue;
			const ot = TRACKS[t]; const row = t < FOCUS ? t : t - 1;
			const y = 40 + row * 3;
			for (let s = 0; s < 16; s++) {
				c.setColorValue(ot.gates[s] ? (ot.mute ? Color.Low : Color.MediumLow) : Color.Low);
				c.point(12 + s * 15 + 6, y + 1);
			}
		}
	}

	// Track Edit: compact gate strip at top; large note name dominant; pitch bar below
	function drawEdit_TE2(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.Low); c.drawText(2, 6, 'T1  P01');
		c.setColorValue(Color.Low); c.hline(0, 7, 256);
		const steps = TRACKS[0].gates;
		const CURSOR = 3;
		// compact gate row
		for (let s = 0; s < 16; s++) {
			c.setColorValue(steps[s] ? Color.MediumLow : Color.Low);
			c.fillRect(1 + s * 16, 9, 14, 6);
			if (s === CURSOR) { c.setColorValue(Color.Bright); c.rect(s * 16, 8, 16, 8); }
		}
		c.setColorValue(Color.Low); c.hline(0, 17, 256);
		// dominant: large current note
		c.setFont(Font.Normal); c.setColorValue(Color.Bright);
		c.drawTextAligned(0, 19, 256, 24, HAlign.Center, VAlign.Center, 'C4');
		c.setFont(Font.Tiny);
		// pitch bar (visual encoding)
		c.setColorValue(Color.Low); c.rect(10, 46, 236, 5);
		c.setColorValue(Color.MediumBright); c.fillRect(10, 46, 118, 5);
		// subordinate context
		c.setColorValue(Color.Low); c.hline(0, 54, 256);
		c.drawTextAligned(0, 56, 256, 7, HAlign.Center, VAlign.Center, 'STEP 4  NOTE  GATE ON');
	}

	// Perform: 8 columns, large pattern# dominant, tiny track# subordinate
	function drawPerf_TE2(c: Canvas): void {
		c.setFont(Font.Tiny); hdr(c, 'PERFORM');
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const x = t * 32 + 1;
			// track number: tiny, subordinate
			c.setColorValue(trk.mute ? Color.Low : Color.MediumLow);
			c.drawTextAligned(x, 9, 30, 6, HAlign.Center, VAlign.Center, String(trk.num));
			// pattern number: dominant
			c.setFont(Font.Normal);
			c.setColorValue(trk.mute ? Color.Low : Color.Bright);
			c.drawTextAligned(x, 16, 30, 30, HAlign.Center, VAlign.Center, String(trk.pat).padStart(2, '0'));
			c.setFont(Font.Tiny);
			if (trk.mute) {
				c.setColorValue(Color.Low);
				c.drawTextAligned(x, 50, 30, 8, HAlign.Center, VAlign.Center, 'MUT');
			}
		}
	}

	// ── Monome Norns-inspired variations (batch 1) ──────────────────────────

	// Dashboard: active steps are lit; off steps are true black; focus=bright, others=dim
	function drawDash_Norns3(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.Medium); c.drawText(2, 6, 'PLAY');
		// no separator — calm, no-noise header
		const FOCUS = 1;
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const y = 8 + t * 7;
			const isFocus = t === FOCUS;
			c.setColorValue(isFocus ? Color.Medium : Color.Low);
			c.drawText(2, y + 6, String(trk.num));
			// only draw on-steps; off-steps are true black (breathing room)
			for (let s = 0; s < 16; s++) {
				if (trk.gates[s]) {
					c.setColorValue(isFocus ? Color.Bright : Color.Low);
					c.fillRect(12 + s * 15, y + 1, 13, 5);
				}
			}
		}
	}

	// Track Edit: large sparse cells — on=bright, off=nothing, cursor=dim mid-level
	function drawEdit_Norns3(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.Low); c.drawText(2, 6, 'T1  NOTE  P01');
		const steps = TRACKS[0].gates;
		const CURSOR = 3;
		const CW = 28, CH = 22, GAP = 2, SX = 8, SY = 9;
		for (let s = 0; s < 16; s++) {
			const col = s % 8; const row = Math.floor(s / 8);
			const x = SX + col * (CW + GAP); const y = SY + row * (CH + GAP);
			const on = Boolean(steps[s]); const isCursor = s === CURSOR;
			if (on) {
				c.setColorValue(isCursor ? Color.Bright : Color.MediumBright);
				c.fillRect(x, y, CW, CH);
			} else if (isCursor) {
				// cursor on off-step: dim marker, not filled
				c.setColorValue(Color.MediumLow); c.rect(x, y, CW, CH);
			}
			// off + no cursor: nothing drawn (calm black)
		}
		c.setColorValue(Color.Low);
		c.drawTextAligned(0, 57, 256, 7, HAlign.Center, VAlign.Center, 'GATE');
	}

	// Perform: 8 pattern numbers at 3 brightness levels — focus=bright, active=medium, mute=dim
	function drawPerf_Norns3(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.Low); c.drawText(2, 6, 'PERFORM');
		const FOCUS = 1;
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const isFocus = t === FOCUS;
			const x = t * 32 + 1;
			c.setFont(Font.Normal);
			c.setColorValue(isFocus ? Color.Bright : (trk.mute ? Color.Low : Color.Medium));
			c.drawTextAligned(x, 9, 30, 46, HAlign.Center, VAlign.Center, String(trk.pat).padStart(2, '0'));
			c.setFont(Font.Tiny);
		}
	}

	// ── TRUNK variations ─────────────────────────────────────────────────────
	// These are the current "best candidate" per page, written to the design
	// language spec (Bright/Low/off palette only, inverted-fill = cursor/focus,
	// footer always present, purposeful layout). Challengers from the loop
	// must beat these to become the new trunk.

	// Dashboard trunk: Elektron's inverted-row focus + Norns' off=absent discipline.
	// 3 levels strict. Footer present but unlabeled (TBD).
	function drawDash_trunk(c: Canvas): void {
		c.setFont(Font.Tiny);
		// status strip (optional header — keeping it here for context)
		c.setColorValue(Color.Low); c.drawText(2, 6, 'PLAY  INT');
		c.setColorValue(Color.Low); c.hline(0, 7, 256);
		const FOCUS = 1;
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const y = 8 + t * 7;
			if (t === FOCUS) {
				// focused track: inverted row (Bright fill, Low content)
				c.setColorValue(Color.Bright); c.fillRect(0, y, 256, 7);
				c.setColorValue(Color.Low);
				c.drawText(2, y + 6, String(trk.num));
				c.drawText(9, y + 6, trk.type.slice(0, 4));
				c.drawText(36, y + 6, 'P' + String(trk.pat).padStart(2, '0'));
				// on-steps as gaps (inverse: off = Bright cell, on = Low gap)
				// Actually: step cells — on-steps are dark (no pixel, since bg is Bright)
				// off-steps: nothing (Bright bg shows through)
				// Render gate on-steps as dim voids in the bright row
				for (let s = 0; s < 16; s++) {
					if (!trk.gates[s]) {
						c.setColorValue(Color.Low);
						c.fillRect(56 + s * 12, y + 1, 11, 5);
					}
				}
			} else {
				// other tracks: just on-steps drawn, off-steps nothing (Norns discipline)
				c.setColorValue(trk.mute ? Color.Low : Color.Low);
				c.drawText(2, y + 6, String(trk.num));
				if (!trk.mute) {
					for (let s = 0; s < 16; s++) {
						if (trk.gates[s]) {
							c.setColorValue(Color.Low);
							c.fillRect(56 + s * 12, y + 1, 11, 5);
						}
					}
				}
			}
		}
		// footer — 3 dashes to show it's there but content TBD
		c.setColorValue(Color.Low); c.hline(0, 55, 256);
		c.drawText(2, 62, '—    —    —    —    —');
	}

	// Perform trunk: 8 rows, clear pattern state, 3-level palette, footer with actions.
	// Each track row shows: num · current pat (Bright) · 16 pattern slots · mute state.
	// Pattern slots: active=Bright, queued=Low fill (distinct from off=nothing).
	function drawPerf_trunk(c: Canvas): void {
		c.setFont(Font.Tiny);
		// status strip
		c.setColorValue(Color.Low); c.drawText(2, 6, 'LATCH  INT');
		c.setColorValue(Color.Low); c.hline(0, 7, 256);
		const FOCUS = 1;
		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t]; const y = 8 + t * 7;
			const isFocus = t === FOCUS;
			if (isFocus) {
				c.setColorValue(Color.Bright); c.fillRect(0, y, 256, 7);
				c.setColorValue(Color.Low);
				c.drawText(2, y + 6, String(trk.num));
				c.drawText(9, y + 6, String(trk.pat).padStart(2, '0'));
				// 16 slots: active = dark gap (in bright row), others = Low fill
				for (let p = 0; p < 16; p++) {
					if (p !== trk.pat - 1) {
						c.fillRect(20 + p * 14, y + 1, 13, 5);
					}
				}
				if (trk.mute) { c.drawText(232, y + 6, 'MUT'); }
			} else {
				c.setColorValue(Color.Low); c.drawText(2, y + 6, String(trk.num));
				// current pattern number
				c.setColorValue(trk.mute ? Color.Low : Color.Bright);
				c.drawText(9, y + 6, String(trk.pat).padStart(2, '0'));
				// 16 slots: only active drawn (Bright dot); queued would be Low dot
				c.setColorValue(Color.Low);
				c.fillRect(20 + (trk.pat - 1) * 14, y + 2, 13, 3);
				if (trk.mute) { c.setColorValue(Color.Low); c.drawText(232, y + 6, 'M'); }
			}
		}
		// footer
		c.setColorValue(Color.Low); c.hline(0, 55, 256);
		const fLabels = ['LTCH', 'SYNC', 'SNAP', 'FILL', '    '];
		for (let i = 0; i < 5; i++) {
			c.setColorValue(Color.Low);
			c.drawTextAligned(i * 51, 56, 51, 8, HAlign.Center, VAlign.Center, fLabels[i]);
		}
	}

	// ── LED state functions ──────────────────────────────────────────────────
	// One per variation that has meaningful LED semantics. Falls back to
	// defaultLeds() for everything else — see Variation.leds below.

	const PLAY_CURSOR = 4; // fixture: play cursor sits on step index 4

	function ledsOff(): LedState { return defaultLeds(); }

	// Dashboard — Elektron1
	function ledsDash_Elektron1(): LedState {
		const s = defaultLeds();
		s.performer = 'green';    // Performer key opened this page
		s.play = 'green';         // playing
		for (let t = 0; t < 8; t++) {
			s.track[t] = TRACKS[t].mute ? 'red' : 'green';
		}
		// focused track (1) step gates
		TRACKS[1].gates.forEach((on, i) => {
			s.step[i] = i === PLAY_CURSOR ? 'amber' : (on ? 'green' : 'off');
		});
		return s;
	}

	// Dashboard — TE2 (same semantics, different screen)
	function ledsDash_TE2(): LedState { return ledsDash_Elektron1(); }

	// Dashboard — Norns3
	function ledsDash_Norns3(): LedState {
		const s = defaultLeds();
		s.performer = 'green';
		s.play = 'green';
		for (let t = 0; t < 8; t++) {
			s.track[t] = t === 1 ? 'green' : (TRACKS[t].mute ? 'red' : 'off');
		}
		TRACKS[1].gates.forEach((on, i) => {
			if (on) s.step[i] = i === PLAY_CURSOR ? 'amber' : 'green';
		});
		return s;
	}

	// Perform — Elektron1
	function ledsPerf_Elektron1(): LedState {
		const s = defaultLeds();
		s.pattern = 'green';      // Pattern key opened this page
		s.play = 'green';
		for (let t = 0; t < 8; t++) {
			s.track[t] = TRACKS[t].mute ? 'red' : (t === 1 ? 'green' : 'green');
		}
		// Step LEDs = pattern slots for focused track (TRACKS[1].pat=2 → step index 1)
		s.step[TRACKS[1].pat - 1] = 'green';  // active pattern
		s.fkey[0] = 'off';  // LATCH
		s.fkey[1] = 'off';  // SYNC
		s.fkey[2] = 'off';  // SNAP
		s.fkey[3] = 'off';  // FILL
		return s;
	}

	// Perform — TE2 / Norns3 (same concept)
	function ledsPerf_TE2(): LedState { return ledsPerf_Elektron1(); }
	function ledsPerf_Norns3(): LedState { return ledsPerf_Elektron1(); }

	// Track Edit — Elektron1
	function ledsEdit_Elektron1(): LedState {
		const s = defaultLeds();
		s.play = 'green';
		s.track[0] = 'green';    // track 1 is selected
		// Step LEDs: on-steps = green, cursor (3) = amber, play cursor (4) = amber
		TRACKS[0].gates.forEach((on, i) => {
			if (i === 3) s.step[i] = 'amber';       // edit cursor
			else if (i === PLAY_CURSOR) s.step[i] = 'amber';  // play cursor
			else s.step[i] = on ? 'green' : 'off';
		});
		s.fkey[0] = 'green';     // GATE tab is active
		return s;
	}

	// Track Edit — TE2 / Norns3
	function ledsEdit_TE2(): LedState { return ledsEdit_Elektron1(); }
	function ledsEdit_Norns3(): LedState { return ledsEdit_Elektron1(); }

	// Trunk LED states
	function ledsDash_trunk(): LedState { return ledsDash_Elektron1(); }
	function ledsPerf_trunk(): LedState { return ledsPerf_Elektron1(); }

	// ── Squarp Hapax-inspired variations (batch 2) ──────────────────────────
	// Core insight: replace gate-dot pattern with a per-track PLAYBACK POSITION BAR.
	// Hapax's left screen shows a progress bar per track (how far through the current
	// pattern). That gives "phase" info (where are we NOW) vs gate-dots (what will play).
	// Type is a single char glyph to free horizontal space for the bar.
	// Muted tracks: no bar drawn — absence communicates mute state.

	// Dashboard: per-track phase bars; type char glyph; mute = absent bar
	function drawDash_Hapax4(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumLow); c.drawText(2, 6, 'PLAY  INT');
		c.setColorValue(Color.MediumLow); c.hline(0, 7, 256);

		const FOCUS = 1;
		const BAR_X = 20, BAR_W = 228;
		const fillW = Math.round((PLAY_CURSOR / 16) * BAR_W); // 4/16 → 57px

		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t];
			const y = 8 + t * 7;
			const isFocus = t === FOCUS;

			if (isFocus) {
				c.setColorValue(Color.Bright); c.fillRect(0, y, 256, 7);
				c.setColorValue(Color.MediumLow);
				c.drawText(2, y + 6, String(trk.num));
				c.drawText(10, y + 6, trk.type[0]);
				// Progress as dark cutout into the bright row
				c.setColorValue(Color.None);
				c.fillRect(BAR_X, y + 1, fillW, 5);
			} else if (trk.mute) {
				// Muted = absent bar — only dim track num
				c.setColorValue(Color.MediumLow);
				c.drawText(2, y + 6, String(trk.num));
			} else {
				c.setColorValue(Color.MediumLow);
				c.drawText(2, y + 6, String(trk.num));
				c.drawText(10, y + 6, trk.type[0]);
				// Bar outline + Bright fill for position
				c.rect(BAR_X, y + 1, BAR_W, 5);
				if (fillW > 1) {
					c.setColorValue(Color.Bright);
					c.fillRect(BAR_X + 1, y + 2, fillW - 2, 3);
				}
			}
		}

		c.setColorValue(Color.MediumLow); c.hline(0, 55, 256);
		c.drawText(2, 62, '—    —    —    —    —');
	}

	// Perform: pattern# + per-track phase bar; focused row inverted; footer with actions
	function drawPerf_Hapax4(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumLow); c.drawText(2, 6, 'LATCH  INT');
		c.setColorValue(Color.MediumLow); c.hline(0, 7, 256);

		const FOCUS = 1;
		const BAR_X = 24, BAR_W = 220;
		const fillW = Math.round((PLAY_CURSOR / 16) * BAR_W);

		for (let t = 0; t < 8; t++) {
			const trk = TRACKS[t];
			const y = 8 + t * 7;
			const isFocus = t === FOCUS;

			if (isFocus) {
				c.setColorValue(Color.Bright); c.fillRect(0, y, 256, 7);
				c.setColorValue(Color.MediumLow);
				c.drawText(2, y + 6, String(trk.num));
				c.drawText(9, y + 6, String(trk.pat).padStart(2, '0'));
				// Phase bar as dark cutout
				c.setColorValue(Color.None);
				c.fillRect(BAR_X, y + 1, fillW, 5);
				if (trk.mute) { c.setColorValue(Color.MediumLow); c.drawText(246, y + 6, 'M'); }
			} else if (trk.mute) {
				c.setColorValue(Color.MediumLow);
				c.drawText(2, y + 6, String(trk.num));
				c.drawText(9, y + 6, String(trk.pat).padStart(2, '0'));
				// No bar — muted = absent
			} else {
				c.setColorValue(Color.MediumLow);
				c.drawText(2, y + 6, String(trk.num));
				c.setColorValue(Color.Bright);
				c.drawText(9, y + 6, String(trk.pat).padStart(2, '0'));
				c.setColorValue(Color.MediumLow);
				c.rect(BAR_X, y + 1, BAR_W, 5);
				if (fillW > 1) {
					c.setColorValue(Color.Bright);
					c.fillRect(BAR_X + 1, y + 2, fillW - 2, 3);
				}
			}
		}

		c.setColorValue(Color.MediumLow); c.hline(0, 55, 256);
		const fLabels = ['LTCH', 'SYNC', 'SNAP', 'FILL', '    '];
		for (let i = 0; i < 5; i++) {
			c.setColorValue(Color.MediumLow);
			c.drawTextAligned(i * 51, 56, 51, 8, HAlign.Center, VAlign.Center, fLabels[i]);
		}
	}

	function ledsDash_Hapax4(): LedState {
		const s = defaultLeds();
		s.performer = 'green';
		s.play = 'green';
		// Only focused lit; muted = red; others off (phase-focused, not gate-focused)
		for (let t = 0; t < 8; t++) {
			s.track[t] = t === 1 ? 'green' : (TRACKS[t].mute ? 'red' : 'off');
		}
		s.step[PLAY_CURSOR] = 'amber'; // play position only — no gate dots
		return s;
	}

	function ledsPerf_Hapax4(): LedState {
		const s = defaultLeds();
		s.pattern = 'green';
		s.play = 'green';
		for (let t = 0; t < 8; t++) {
			s.track[t] = TRACKS[t].mute ? 'red' : (t === 1 ? 'green' : 'off');
		}
		s.step[PLAY_CURSOR] = 'amber';
		return s;
	}

	// ── Tab helpers ──────────────────────────────────────────────────────────
	// Reusable footer tab bar. `active` is 0-based (F1=0 … F5=4).

	function drawFooterTabs(c: Canvas, tabs: string[], active: number): void {
		c.setColorValue(Color.MediumLow); c.hline(0, 55, 256);
		for (let i = 0; i < tabs.length; i++) {
			const slotW = Math.floor(256 / tabs.length);
			const w = i < tabs.length - 1 ? slotW : 256 - slotW * (tabs.length - 1);
			const x = i * slotW;
			if (i === active) {
				c.setColorValue(Color.Bright); c.fillRect(x, 56, w, 8);
				c.setColorValue(Color.None);
			} else {
				c.setColorValue(Color.MediumLow);
			}
			c.drawTextAligned(x, 56, w, 8, HAlign.Center, VAlign.Center, tabs[i]);
		}
	}

	function drawTabPlaceholder(c: Canvas, name: string): void {
		c.setFont(Font.Normal);
		c.setColorValue(Color.Low);
		c.drawTextAligned(0, 9, 256, 44, HAlign.Center, VAlign.Center, name);
		c.setFont(Font.Tiny);
	}

	// ── Note Track Edit — 3 numbered variations ─────────────────────────────
	// All three share the same footer tab set: GATE | NOTE | COND | REPT | —.
	// GATE covers Gate/GateProbability/GateOffset/Retrigger/Length.
	// CURSOR=4 (step 5, gate on, D4). PLAY_CURSOR=4 coincides for this fixture.

	const NOTE_CURSOR = 4;
	const NOTE_TABS = ['GATE', 'NOTE', 'COND', 'REPT', '—'];

	// Option 1 — full-height gate cells + embedded pitch bar at bottom of each on-step
	function drawNote_1(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumLow);
		c.drawText(2, 6, 'TRK 1  NOTE  P01'); c.hline(0, 7, 256);

		if (tab === 0) {
			for (let s = 0; s < 16; s++) {
				const x = 1 + s * 16;
				const on = Boolean(TRACKS[0].gates[s]);
				const isBeat = s % 4 === 0;
				const isCursor = s === NOTE_CURSOR;
				const noteVal = TRACKS[0].notes[s] || 0;
				const barH = (on && noteVal) ? Math.max(2, Math.round(((noteVal - 36) / 48) * 10)) : 0;

				if (isCursor) {
					c.setColorValue(Color.Bright); c.fillRect(x, 9, 14, 44);
					if (barH > 0) { c.setColorValue(Color.None); c.fillRect(x + 2, 44 - barH, 10, barH); }
					c.setColorValue(Color.MediumLow);
					c.drawTextAligned(x, 44, 14, 8, HAlign.Center, VAlign.Center, String(s + 1));
				} else if (on) {
					c.setColorValue(Color.Bright); c.fillRect(x, 9, 14, 44);
					if (isBeat) { c.setColorValue(Color.None); c.hline(x, 9, 14); }
					if (barH > 0) { c.setColorValue(Color.None); c.fillRect(x + 2, 44 - barH, 10, barH); }
					c.setColorValue(Color.MediumLow);
					c.drawTextAligned(x, 44, 14, 8, HAlign.Center, VAlign.Center, String(s + 1));
				} else {
					c.setColorValue(Color.MediumLow); c.rect(x, 9, 14, 44);
					if (isBeat) { c.hline(x, 9, 14); }
					c.drawTextAligned(x, 44, 14, 8, HAlign.Center, VAlign.Center, String(s + 1));
				}
			}
		} else {
			drawTabPlaceholder(c, NOTE_TABS[tab]);
		}

		drawFooterTabs(c, NOTE_TABS, tab);
	}

	// Option 2 — compact gate strip + large dominant note value for cursor step
	function drawNote_2(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumLow);
		c.drawText(2, 6, 'TRK 1  NOTE  P01'); c.hline(0, 7, 256);

		// Compact gate row (8px tall) — always visible regardless of tab
		for (let s = 0; s < 16; s++) {
			const x = 1 + s * 16;
			const on = Boolean(TRACKS[0].gates[s]);
			const isBeat = s % 4 === 0;
			const isCursor = s === NOTE_CURSOR;
			if (isCursor) {
				c.setColorValue(Color.Bright); c.fillRect(x, 9, 14, 6);
				c.setColorValue(Color.None); c.rect(x + 1, 10, 12, 4);
			} else if (on) {
				c.setColorValue(Color.Bright); c.fillRect(x, 9, 14, 6);
				if (isBeat) { c.setColorValue(Color.None); c.hline(x, 9, 14); }
			} else {
				c.setColorValue(Color.MediumLow); c.rect(x, 9, 14, 6);
				if (isBeat) { c.hline(x, 9, 14); }
			}
		}
		c.setColorValue(Color.MediumLow); c.hline(0, 16, 256);

		if (tab === 0) {
			// GATE tab — same content, gate strip already drawn above
			c.setColorValue(Color.MediumLow);
			c.drawTextAligned(0, 18, 256, 35, HAlign.Center, VAlign.Center, 'GATE');
		} else if (tab === 1) {
			// NOTE tab — dominant note name
			const noteVal = TRACKS[0].notes[NOTE_CURSOR] || 0;
			const gateOn = Boolean(TRACKS[0].gates[NOTE_CURSOR]);
			const noteNames = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
			const noteName = (gateOn && noteVal)
				? (noteNames[noteVal % 12] + String(Math.floor(noteVal / 12) - 1))
				: (gateOn ? 'C4' : '—');

			c.setFont(Font.Normal);
			c.setColorValue(gateOn ? Color.Bright : Color.MediumLow);
			c.drawTextAligned(0, 18, 256, 22, HAlign.Center, VAlign.Center, noteName);
			c.setFont(Font.Tiny);

			if (gateOn && noteVal) {
				const fraction = Math.min(1, (noteVal - 36) / 48);
				const barW = Math.round(fraction * 220);
				c.setColorValue(Color.MediumLow); c.rect(16, 42, 224, 4);
				if (barW > 1) { c.setColorValue(Color.Bright); c.fillRect(17, 43, barW - 1, 2); }
			}

			c.setColorValue(Color.MediumLow);
			c.drawTextAligned(0, 47, 256, 7, HAlign.Center, VAlign.Center,
				'STEP ' + (NOTE_CURSOR + 1) + (gateOn ? '  GATE ON  ' + noteName : '  GATE OFF'));
		} else {
			drawTabPlaceholder(c, NOTE_TABS[tab]);
		}

		drawFooterTabs(c, NOTE_TABS, tab);
	}

	// Option 3 — bottom-anchored pitch bars: height = note pitch; cursor inverted
	function drawNote_3(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumLow);
		c.drawText(2, 6, 'TRK 1  NOTE  P01'); c.hline(0, 7, 256);

		if (tab === 0) {
			const CELL_Y = 9, CELL_H = 44, CELL_BOT = CELL_Y + CELL_H;

			for (let s = 0; s < 16; s++) {
				const x = 1 + s * 16;
				const on = Boolean(TRACKS[0].gates[s]);
				const isBeat = s % 4 === 0;
				const isCursor = s === NOTE_CURSOR;
				const noteVal = TRACKS[0].notes[s] || 0;
				const barH = (on && noteVal) ? Math.max(4, Math.round(((noteVal - 36) / 48) * 40)) : 4;

				if (isCursor && on) {
					c.setColorValue(Color.Bright); c.fillRect(x, CELL_Y, 14, CELL_H);
					c.setColorValue(Color.None); c.fillRect(x, CELL_BOT - barH, 14, barH);
					if (isBeat) { c.hline(x, CELL_Y, 14); }
				} else if (isCursor) {
					c.setColorValue(Color.Bright); c.fillRect(x, CELL_Y, 14, CELL_H);
				} else if (on) {
					c.setColorValue(Color.Bright); c.fillRect(x, CELL_BOT - barH, 14, barH);
					if (isBeat) {
						c.setColorValue(Color.MediumLow);
						c.hline(x, CELL_Y, 14); c.hline(x, CELL_Y + 1, 14);
					}
				} else {
					c.setColorValue(Color.MediumLow); c.rect(x, CELL_Y, 14, CELL_H);
					if (isBeat) { c.hline(x, CELL_Y, 14); }
				}
			}
		} else {
			drawTabPlaceholder(c, NOTE_TABS[tab]);
		}

		drawFooterTabs(c, NOTE_TABS, tab);
	}

	// Option 4 — line sequencer: presence=gate, height=pitch, width=gate length
	// ── Note track Option 4 helpers ────────────────────────────────────────
	// Shared constants and helpers used by all Option 4 screens.

	function note4Lerp(v: number, inLo: number, inHi: number, outLo: number, outHi: number): number {
		return Math.round(outLo + ((v - inLo) / (inHi - inLo)) * (outHi - outLo));
	}

	const NOTE4_MIN = 36, NOTE4_MAX = 84;
	const NOTE4_TOP = 9, NOTE4_BOT = 53;
	const NOTE_NAMES_12 = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
	const COND_FULL = ['ALWAYS','FILL','!FILL','50%','25%','75%'];
	const COND_SHORT4 = ['','FI','!F','50','25','75'];

	function note4DrawGrid(c: Canvas): void {
		for (const oct of [36, 48, 60, 72, 84]) {
			const gy = note4Lerp(oct, NOTE4_MIN, NOTE4_MAX, NOTE4_BOT, NOTE4_TOP);
			c.setColorValue(oct === 60 ? Color.MediumLow : Color.Low);
			c.hline(0, gy, 256);
		}
	}

	function note4DrawLines(c: Canvas, focusCursor = false): void {
		for (let s = 0; s < 16; s++) {
			const cx = 8 + s * 16;
			const on = Boolean(TRACKS[0].gates[s]);
			const isCursor = s === NOTE_CURSOR;
			if (on) {
				const noteVal = TRACKS[0].notes[s] || 60;
				const lineY = note4Lerp(noteVal, NOTE4_MIN, NOTE4_MAX, NOTE4_BOT, NOTE4_TOP);
				c.setColorValue(focusCursor && !isCursor ? Color.MediumLow : Color.Bright);
				c.hline(cx - 5, lineY, 11);
			}
			if (isCursor) {
				c.setColorValue(on ? Color.Bright : Color.MediumLow);
				c.vline(cx, NOTE4_TOP, 3);
			}
		}
	}

	function note4Header(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumLow);
		c.drawText(2, 6, 'TRK 1  NOTE  P01'); c.hline(0, 7, 256);
	}

	// Browse state: line view with tab-specific additive overlays.
	function drawNote_4(c: Canvas, tab = 0): void {
		note4Header(c);

		// NOTE tab: faint octave grid behind lines
		if (tab === 1) note4DrawGrid(c);

		note4DrawLines(c, false);

		// COND tab: small condition abbreviations below each conditioned step
		if (tab === 2) {
			for (let s = 0; s < 16; s++) {
				const cond = TRACKS[0].conds[s] ?? 0;
				if (cond > 0 && Boolean(TRACKS[0].gates[s])) {
					const cx = 8 + s * 16;
					c.setColorValue(s === NOTE_CURSOR ? Color.Bright : Color.MediumLow);
					c.drawTextAligned(cx - 8, NOTE4_BOT - 5, 16, 5, HAlign.Center, VAlign.Center, COND_SHORT4[cond] ?? '?');
				}
			}
		}

		// REPT tab: repeat count above cursor tick for steps with repeats > 1
		if (tab === 3) {
			for (let s = 0; s < 16; s++) {
				const rep = TRACKS[0].repeats[s] ?? 1;
				if (rep > 1 && Boolean(TRACKS[0].gates[s])) {
					const cx = 8 + s * 16;
					c.setColorValue(s === NOTE_CURSOR ? Color.Bright : Color.MediumLow);
					c.drawTextAligned(cx - 8, NOTE4_TOP + 4, 16, 5, HAlign.Center, VAlign.Center, rep + 'x');
				}
			}
		}

		drawFooterTabs(c, NOTE_TABS, tab);
	}

	// Step-held interaction states: user is pressing step 5 and turning encoder.
	// Base line view stays visible; the held parameter appears as a prominent overlay.

	function drawNote_4_holdNote(c: Canvas): void {
		note4Header(c);
		note4DrawGrid(c);
		note4DrawLines(c, true); // non-cursor steps dimmed to focus attention

		const noteVal = TRACKS[0].notes[NOTE_CURSOR] || 60;
		const noteName = NOTE_NAMES_12[noteVal % 12] + String(Math.floor(noteVal / 12) - 1);
		c.setFont(Font.Normal);
		c.setColorValue(Color.Bright);
		c.drawTextAligned(0, 18, 256, 18, HAlign.Center, VAlign.Center, noteName);
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumLow);
		c.drawTextAligned(0, 44, 256, 8, HAlign.Center, VAlign.Center, 'STEP ' + (NOTE_CURSOR + 1) + '   ←  →');

		drawFooterTabs(c, NOTE_TABS, 1);
	}

	function drawNote_4_holdCond(c: Canvas): void {
		note4Header(c);
		note4DrawLines(c, true);

		for (let s = 0; s < 16; s++) {
			const cond = TRACKS[0].conds[s] ?? 0;
			if (cond > 0 && Boolean(TRACKS[0].gates[s]) && s !== NOTE_CURSOR) {
				const cx = 8 + s * 16;
				c.setColorValue(Color.MediumLow);
				c.drawTextAligned(cx - 8, NOTE4_BOT - 5, 16, 5, HAlign.Center, VAlign.Center, COND_SHORT4[cond] ?? '?');
			}
		}

		const cursorCond = TRACKS[0].conds[NOTE_CURSOR] ?? 0;
		c.setFont(Font.Normal);
		c.setColorValue(Color.Bright);
		c.drawTextAligned(0, 18, 256, 18, HAlign.Center, VAlign.Center, COND_FULL[cursorCond] ?? 'ALWAYS');
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumLow);
		c.drawTextAligned(0, 44, 256, 8, HAlign.Center, VAlign.Center, 'STEP ' + (NOTE_CURSOR + 1) + '   ←  →');

		drawFooterTabs(c, NOTE_TABS, 2);
	}

	function drawNote_4_holdRept(c: Canvas): void {
		note4Header(c);
		note4DrawLines(c, true);

		for (let s = 0; s < 16; s++) {
			const rep = TRACKS[0].repeats[s] ?? 1;
			if (rep > 1 && Boolean(TRACKS[0].gates[s]) && s !== NOTE_CURSOR) {
				const cx = 8 + s * 16;
				c.setColorValue(Color.MediumLow);
				c.drawTextAligned(cx - 8, NOTE4_TOP + 4, 16, 5, HAlign.Center, VAlign.Center, rep + 'x');
			}
		}

		const cursorRep = TRACKS[0].repeats[NOTE_CURSOR] ?? 1;
		c.setFont(Font.Normal);
		c.setColorValue(Color.Bright);
		c.drawTextAligned(0, 16, 256, 16, HAlign.Center, VAlign.Center, cursorRep + '×');
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumBright);
		c.drawTextAligned(0, 34, 256, 8, HAlign.Center, VAlign.Center, 'FREE');
		c.setColorValue(Color.MediumLow);
		c.drawTextAligned(0, 44, 256, 8, HAlign.Center, VAlign.Center, 'STEP ' + (NOTE_CURSOR + 1) + '   ←  →');

		drawFooterTabs(c, NOTE_TABS, 3);
	}

	function ledsNote_1(): LedState {
		const s = defaultLeds();
		s.play = 'green'; s.track[0] = 'green';
		TRACKS[0].gates.forEach((on, i) => {
			s.step[i] = i === NOTE_CURSOR ? 'amber' : (on ? 'green' : 'off');
		});
		s.fkey[0] = 'green'; // GATE tab active
		return s;
	}
	function ledsNote_2(): LedState {
		const s = ledsNote_1(); s.fkey[0] = 'off'; s.fkey[1] = 'green'; return s;
	}
	function ledsNote_3(): LedState { return ledsNote_1(); }
	function ledsNote_4_note(): LedState { const s = ledsNote_1(); s.fkey[0] = 'off'; s.fkey[1] = 'green'; return s; }
	function ledsNote_4_cond(): LedState { const s = ledsNote_1(); s.fkey[0] = 'off'; s.fkey[2] = 'green'; return s; }
	function ledsNote_4_rept(): LedState { const s = ledsNote_1(); s.fkey[0] = 'off'; s.fkey[3] = 'green'; return s; }


	// ── Firmware UI Recreation ───────────────────────────────────────────────
	// Faithful reproduction of NoteSequenceEditPage::draw() from C++.
	// Source: src/apps/sequencer/ui/pages/NoteSequenceEditPage.cpp
	// Tabs: GATE | RETRIG | LEN | NOTE | COND (current firmware tab labels).
	// Layers shown per tab are post-cut: RetrigProbability / LenVar / NoteVar /
	// BypassScale removed; StageRepeats / StageRepeatsMode kept.

	const FW_TABS = ['GATE', 'RETRIG', 'LEN', 'NOTE', 'COND'];
	const FW_CURSOR = 4;   // edit cursor on step 5 (index 4)
	const FW_FIRST = 0;    // sequence.firstStep()
	const FW_LAST = 15;    // sequence.lastStep()
	// Extra per-step fixture data for sub-layer display
	const FW_LENS   = [8,0,0,0,4,0,0,0,12,0,0,0,6,0,0,0];  // length 0–15
	const FW_RETRIG = [0,0,0,0,2,0,0,0,0,0,0,0,1,0,0,0];   // retrigger 0–7
	const FW_CONDS  = [0,0,0,0,2,0,0,0,0,0,0,0,3,0,0,0];   // condition index

	// Port of WindowPainter::drawInvertedText — fills bright rect, subs text out
	function fwInvertText(c: Canvas, x: number, y: number, text: string): void {
		c.setFont(Font.Tiny);
		c.setBlendMode(BlendMode.Set);
		c.setColorValue(Color.Bright);
		c.fillRect(x - 1, y - 5, c.textWidth(text) + 1, 7);
		c.setBlendMode(BlendMode.Sub);
		c.drawText(x, y, text);
		c.setBlendMode(BlendMode.Set);
	}

	// Port of WindowPainter::drawHeader + drawActiveFunction
	function fwDrawHeader(c: Canvas, layerName: string): void {
		c.setFont(Font.Tiny);
		c.setBlendMode(BlendMode.Set);

		// Clock mode "A" — inverted (mimics recording/clock-source indicator)
		fwInvertText(c, 2, 6, 'A');

		// BPM at x=10
		c.setColorValue(Color.Bright);
		c.drawText(10, 6, '120.0');

		// Track name at x=40
		c.drawText(40, 6, 'T1');

		// Play pattern P01 — inverted (song active = plain, play==edit = inverted)
		fwInvertText(c, 84, 6, 'P01');

		// Edit pattern E01 — inverted (play == edit in fixture)
		fwInvertText(c, 103, 6, 'E01');

		// Active layer name at x=130 (drawActiveFunction)
		c.setColorValue(Color.Bright);
		c.drawText(130, 6, layerName);

		// Mode "STEPS" at far right (drawActiveMode)
		const sw = c.textWidth('STEPS');
		c.drawText(256 - sw - 2, 6, 'STEPS');

		// Header separator
		c.setColorValue(Color.Medium);
		c.hline(0, 9, 256);
	}

	// Port of WindowPainter::drawFunctionKeys — firmware-style footer
	function fwDrawFooter(c: Canvas, tabs: string[], active: number): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.Medium);
		// Separator line (PageHeight-FooterHeight-1 = 64-9-1 = 54)
		c.hline(0, 54, 256);
		// Vertical dividers between slots
		for (let i = 0; i < tabs.length; i++) {
			if (tabs[i] || (i + 1 < tabs.length && tabs[i + 1])) {
				const x = Math.floor((256 * (i + 1)) / tabs.length);
				c.vline(x, 55, 9);
			}
		}
		// Labels (PageHeight-3 = 61)
		for (let i = 0; i < tabs.length; i++) {
			if (!tabs[i]) continue;
			const x0 = Math.floor((256 * i) / tabs.length);
			const x1 = Math.floor((256 * (i + 1)) / tabs.length);
			const w = x1 - x0 + 1;
			c.setColorValue(i === active ? Color.Bright : Color.Medium);
			c.drawText(x0 + Math.floor((w - c.textWidth(tabs[i])) / 2), 61, tabs[i]);
		}
	}

	// Port of SequencePainter::drawProbability
	function fwDrawProbability(c: Canvas, x: number, y: number, w: number, h: number, prob: number, maxProb: number): void {
		const pw = Math.round((w * prob) / maxProb);
		c.setColorValue(Color.Bright); c.fillRect(x, y, pw, h);
		c.setColorValue(Color.Medium); c.fillRect(x + pw, y, w - pw, h);
	}

	// Port of SequencePainter::drawRetrigger
	function fwDrawRetrigger(c: Canvas, x: number, y: number, w: number, h: number, retrigger: number, maxRetrigger: number): void {
		const bw = Math.floor(w / maxRetrigger);
		let rx = x + Math.floor((w - bw * retrigger) / 2);
		c.setColorValue(Color.Bright);
		for (let i = 0; i < retrigger; i++) {
			c.fillRect(rx, y, Math.max(1, Math.floor(bw / 2)), h);
			rx += bw;
		}
	}

	// Port of SequencePainter::drawLength (gate-length L-bracket indicator)
	function fwDrawLength(c: Canvas, x: number, y: number, w: number, h: number, length: number, maxLength: number): void {
		const gw = Math.round(((w - 1) * length) / maxLength);
		c.setColorValue(Color.Bright);
		c.vline(x, y, h);
		c.hline(x, y, gw);
		c.vline(x + gw, y, h);
		c.hline(x + gw, y + h - 1, w - gw);
	}

	// Port of SequencePainter::drawOffset (gate-offset bar)
	function fwDrawOffset(c: Canvas, x: number, y: number, w: number, h: number, offset: number, minOff: number, maxOff: number): void {
		const remap = (v: number) => Math.round(((w - 1) * (v - minOff)) / (maxOff - minOff));
		c.setColorValue(Color.Medium); c.fillRect(x, y, w, h);
		c.setColorValue(Color.None);   c.vline(x + remap(0), y, h);
		c.setColorValue(Color.Bright); c.vline(x + remap(offset), y, h);
	}

	// Port of SequencePainter::drawSlide
	function fwDrawSlide(c: Canvas, x: number, y: number, w: number, h: number, active: boolean): void {
		c.setColorValue(Color.Bright);
		if (active) c.line(x, y + h, x + w, y);
		else c.hline(x, y + h, w);
	}

	// Main firmware Note track edit page reproduction
	function drawFirmwareNote(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);

		const layerNames = ['GATE', 'RETRIG', 'LEN', 'NOTE', 'COND'];
		fwDrawHeader(c, layerNames[tab]);

		const SW = 16;     // stepWidth = 256 / 16
		const loopY = 16;
		const y = 20;      // step cells baseline

		// Loop start bracket (drawLoopStart at x = firstStep*SW+1, w = SW-2)
		c.setColorValue(Color.Bright);
		const lsx = FW_FIRST * SW + 1;
		c.vline(lsx, loopY - 1, 3);
		c.point(lsx + 1, loopY);

		// Loop end bracket (drawLoopEnd: x += w-1)
		const lex = (FW_LAST * SW + 1) + (SW - 2) - 1;  // = lastStep*SW + SW - 2
		c.vline(lex, loopY - 1, 3);
		c.point(lex - 1, loopY);

		// Loop continuation dots between first and last step
		for (let i = 0; i < 16; i++) {
			if (i > FW_FIRST && i <= FW_LAST) {
				c.point(i * SW, loopY);
			}
		}

		// Step cells
		for (let i = 0; i < 16; i++) {
			const x = i * SW;
			const on = Boolean(TRACKS[0].gates[i]);
			const isPlay = i === PLAY_CURSOR;

			// Step number: centered, Bright if cursor, Medium otherwise
			c.setFont(Font.Tiny);
			const numStr = String(i + 1);
			const numW = c.textWidth(numStr);
			c.setColorValue(i === FW_CURSOR ? Color.Bright : Color.Medium);
			c.drawText(x + Math.floor((SW - numW + 1) / 2), y - 2, numStr);

			// Gate box outline: Bright for current play step, Medium otherwise
			c.setColorValue(isPlay ? Color.Bright : Color.Medium);
			c.rect(x + 2, y + 2, 12, 12);

			// Gate fill (8×8 inner fill when gate is on)
			if (on) {
				c.setColorValue(Color.Bright);
				c.fillRect(x + 4, y + 4, 8, 8);
			}

			// Layer-specific indicator at y+18 = 38 (below gate box)
			switch (tab) {
			case 0: // GATE — no sub-indicator in GATE tab
				break;
			case 1: // RETRIG — retrigger pulse marks
				fwDrawRetrigger(c, x, y + 18, SW, 2, FW_RETRIG[i] + 1, 8);
				break;
			case 2: // LEN — gate-length L-bracket
				fwDrawLength(c, x + 2, y + 18, SW - 4, 6, FW_LENS[i] + 1, 16);
				break;
			case 3: { // NOTE — two-line note name (letter + octave)
				const midiNote = TRACKS[0].notes[i];
				if (midiNote && on) {
					c.setFont(Font.Tiny);
					c.setColorValue(Color.Bright);
					const letter = NOTE_NAMES_12[midiNote % 12];
					const octStr = String(Math.floor(midiNote / 12) - 1);
					c.drawText(x + Math.floor((SW - c.textWidth(letter) + 1) / 2), y + 20, letter);
					c.drawText(x + Math.floor((SW - c.textWidth(octStr) + 1) / 2), y + 27, octStr);
				}
				break;
			}
			case 4: { // COND — condition abbreviation
				const cond = FW_CONDS[i];
				if (cond && on) {
					const condStr = COND_SHORT4[cond] ?? '';
					if (condStr) {
						c.setFont(Font.Tiny);
						c.setColorValue(Color.Bright);
						c.drawText(x + Math.floor((SW - c.textWidth(condStr) + 1) / 2), y + 20, condStr);
					}
				}
				break;
			}
			}
		}

		fwDrawFooter(c, FW_TABS, tab);
	}

	function ledsFirmwareNote(): LedState {
		const s = defaultLeds();
		s.play = 'green';
		s.track[0] = 'green';
		TRACKS[0].gates.forEach((on, i) => {
			s.step[i] = i === FW_CURSOR ? 'amber' : (on ? 'green' : 'off');
		});
		s.fkey[0] = 'green'; // GATE tab active by default
		return s;
	}

	// ── New Note V1 — note lines, no header ────────────────────────────────
	// Based on firmware trunk. Changes:
	//   • No header bar (reclaimed ~9px for content)
	//   • Gate boxes removed → horizontal note lines at pitch-mapped Y
	//   • Notes stored as scale degree indices (like firmware), not MIDI
	//   • NOTE tab: octave (Medium) then degree 1–7 (Bright) side by side
	// Layout (top→bottom): step nums | note lines | tab values | footer

	const NV1_LINE_TOP = 20;
	const NV1_LINE_BOT = 43;

	// Scale degree index fixture — varied to show octave display clearly:
	//   idx 0  → degree 1, oct  0  (root)
	//   idx 4  → degree 5, oct  0  (fifth)
	//   idx 7  → degree 1, oct +1  (root, one octave up)
	//   idx -3 → degree 5, oct -1  (fifth, one octave down)
	const V1_NOTE_IDX = [0, 0, 0, 0, 4, 0, 0, 0, 7, 0, 0, 0, -3, 0, 0, 0];

	// Major scale semitone intervals (7 notes per octave)
	const MAJOR_IVLS = [0, 2, 4, 5, 7, 9, 11];

	// Scale degree index → MIDI (for line Y position), root = C4 = 60
	function v1IdxToMidi(idx: number): number {
		const oct = Math.floor(idx / 7);
		const deg0 = ((idx % 7) + 7) % 7;  // 0-indexed degree, always 0–6
		return 60 + oct * 12 + MAJOR_IVLS[deg0];
	}

	// Scale degree index → [octave, degree 1-indexed]
	function v1IdxToDisplay(idx: number): [number, number] {
		const oct = Math.floor(idx / 7);
		const deg = ((idx % 7) + 7) % 7 + 1;  // 1–7
		return [oct, deg];
	}

	// Line Y: maps MIDI pitch (C3=48 … C6=84) to pixel range
	function nv1LineY(midi: number): number {
		return Math.round(NV1_LINE_BOT - ((midi - 48) / (84 - 48)) * (NV1_LINE_BOT - NV1_LINE_TOP));
	}

	function drawNewNoteV1(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);
		const SW = 16;
		const LINE_W = 10;

		fwDrawHeader(c, FW_TABS[tab]);

		// ── 1. Step numbers (y=11–18) ────────────────────────────────────────
		for (let i = 0; i < 16; i++) {
			c.setColorValue(i === FW_CURSOR ? Color.Bright : Color.Medium);
			c.drawTextAligned(i * SW, 11, SW, 8, HAlign.Center, VAlign.Center, String(i + 1));
		}

		// ── 2. Note lines at pitch-mapped Y (y=20–43) ───────────────────────
		for (let i = 0; i < 16; i++) {
			const x = i * SW;
			const cx = x + SW / 2;
			const on = Boolean(TRACKS[0].gates[i]);
			const isCursor = i === FW_CURSOR;
			const isPlay = i === PLAY_CURSOR;

			if (on) {
				const midi = v1IdxToMidi(V1_NOTE_IDX[i]);
				const lineY = nv1LineY(midi);
				c.setColorValue(isCursor || isPlay ? Color.Bright : Color.MediumBright);
				c.hline(cx - (LINE_W >> 1), lineY, LINE_W);
			}

			// Cursor tick at top of area (position reference)
			if (isCursor) {
				c.setColorValue(on ? Color.Bright : Color.MediumLow);
				c.vline(cx, NV1_LINE_TOP, 3);
			}
		}

		// ── 3. Tab value area (y=45–53) ──────────────────────────────────────
		for (let i = 0; i < 16; i++) {
			const x = i * SW;
			const on = Boolean(TRACKS[0].gates[i]);
			if (!on) continue;
			c.setFont(Font.Tiny);

			switch (tab) {
			case 0: // GATE — gate-probability mini bar
				fwDrawProbability(c, x + 1, 48, SW - 2, 3, 15, 15);
				break;
			case 1: { // RETRIG
				const ret = FW_RETRIG[i];
				if (ret > 0) {
					c.setColorValue(Color.MediumBright);
					c.drawTextAligned(x, 45, SW, 8, HAlign.Center, VAlign.Center, String(ret + 1));
				}
				break;
			}
			case 2: { // LEN — proportional width bar
				const lw = Math.max(1, Math.round(((SW - 2) * (FW_LENS[i] + 1)) / 16));
				c.setColorValue(Color.MediumBright);
				c.fillRect(x + 1, 49, lw, 2);
				break;
			}
			case 3: { // NOTE — octave (Medium) then degree 1–7 (Bright), side by side
				const [oct, deg] = v1IdxToDisplay(V1_NOTE_IDX[i]);
				const octStr = oct === 0 ? '0' : (oct > 0 ? '+' + oct : String(oct));
				const degStr = String(deg);
				const octW = c.textWidth(octStr);
				const degW = c.textWidth(degStr);
				const gap = 1;
				const totalW = octW + gap + degW;
				const tx = x + Math.floor((SW - totalW) / 2);
				c.setColorValue(Color.Medium);   c.drawText(tx, 52, octStr);
				c.setColorValue(Color.Bright);   c.drawText(tx + octW + gap, 52, degStr);
				break;
			}
			case 4: { // COND
				const cond = FW_CONDS[i];
				if (cond) {
					c.setColorValue(Color.MediumBright);
					c.drawTextAligned(x, 45, SW, 8, HAlign.Center, VAlign.Center,
						COND_SHORT4[cond] ?? '');
				}
				break;
			}
			}
		}

		fwDrawFooter(c, FW_TABS, tab);
	}

	function ledsNewNoteV1(): LedState { return ledsFirmwareNote(); }

	// ── V2 — Firmware Plus: piano-roll pitch strip ────────────────────────────
	// Research synthesis:
	//   • Piano roll (DAW): pitch = Y position, gate length = bar width
	//   • Elektron: gate cells stay as primary rhythm view; pitch is additive layer
	//   • Beat markers: extra top tick on steps 0/4/8/12 (design-language spec)
	//   • Absence principle: nothing drawn for gate-off steps
	//   • Tab stability: pitch strip always visible regardless of active tab
	//   • Layer detail only for gate-on steps

	function drawNoteV2(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);
		const SW = 16;
		const loopY = 16;
		const cellY = 20;

		// 1. Header (TRUNK)
		fwDrawHeader(c, FW_TABS[tab]);

		// 2. Loop brackets (TRUNK exact)
		c.setColorValue(Color.Bright);
		const lsx = FW_FIRST * SW + 1;
		c.vline(lsx, loopY - 1, 3);
		c.point(lsx + 1, loopY);
		const lex = FW_LAST * SW + SW - 2;
		c.vline(lex, loopY - 1, 3);
		c.point(lex - 1, loopY);
		for (let i = 0; i < 16; i++) {
			if (i > FW_FIRST && i <= FW_LAST) c.point(i * SW, loopY);
		}

		// 3. Gate cells — TRUNK layout + beat markers
		for (let i = 0; i < 16; i++) {
			const x = i * SW;
			const on = Boolean(TRACKS[0].gates[i]);
			const isPlay = i === PLAY_CURSOR;
			const isCursor = i === FW_CURSOR;
			const isBeat = i % 4 === 0;

			// Step number
			const numStr = String(i + 1);
			c.setColorValue(isCursor ? Color.Bright : Color.Medium);
			c.drawText(x + Math.floor((SW - c.textWidth(numStr) + 1) / 2), cellY - 2, numStr);

			// Gate box outline
			c.setColorValue(isPlay ? Color.Bright : Color.Medium);
			c.rect(x + 2, cellY + 2, 12, 12);

			// Beat marker: extra 1px row above outline for steps 0/4/8/12
			if (isBeat) {
				c.setColorValue(isPlay ? Color.Bright : Color.Medium);
				c.hline(x + 2, cellY + 1, 12);
			}

			// Gate fill
			if (on) {
				c.setColorValue(Color.Bright);
				c.fillRect(x + 4, cellY + 4, 8, 8);
			}
		}

		// 4. Pitch strip (y=35–47, 13px tall)
		// Bars: Y = pitch (adaptive to sequence range), width = gate length
		const PITCH_TOP = 35;
		const PITCH_BOT = 47;
		const PITCH_H = PITCH_BOT - PITCH_TOP; // 12px usable

		// Adaptive MIDI range from active gate-on steps
		const activeMidis: number[] = [];
		for (let i = 0; i < 16; i++) {
			if (TRACKS[0].gates[i]) activeMidis.push(v1IdxToMidi(V1_NOTE_IDX[i]));
		}
		let midiMin = activeMidis.length > 0 ? Math.min(...activeMidis) : 57;
		let midiMax = activeMidis.length > 0 ? Math.max(...activeMidis) : 69;
		// Add 1 semitone padding so notes never sit flush on the edges
		if (midiMax === midiMin) { midiMin -= 3; midiMax += 3; }
		else { midiMin -= 1; midiMax += 1; }
		const midiRange = midiMax - midiMin;

		for (let i = 0; i < 16; i++) {
			if (!TRACKS[0].gates[i]) continue;
			const x = i * SW;
			const isCursor = i === FW_CURSOR;
			const isPlay = i === PLAY_CURSOR;

			const midi = v1IdxToMidi(V1_NOTE_IDX[i]);
			const pitchT = (midi - midiMin) / midiRange; // 0 = lowest, 1 = highest
			const barY = Math.round(PITCH_BOT - pitchT * PITCH_H);

			// Bar width = gate length proportion (min 2px so it's always visible)
			const barW = Math.max(2, Math.round(((SW - 4) * (FW_LENS[i] + 1)) / 16));

			c.setColorValue(isCursor || isPlay ? Color.Bright : Color.MediumBright);
			c.hline(x + 2, barY, barW);
			c.hline(x + 2, barY + 1, barW); // 2px tall bar
		}

		// 5. Layer detail strip (y=49–53, tab-specific, gate-on steps only)
		for (let i = 0; i < 16; i++) {
			if (!TRACKS[0].gates[i]) continue;
			const x = i * SW;
			c.setFont(Font.Tiny);

			switch (tab) {
			case 0: // GATE — probability mini bar
				fwDrawProbability(c, x + 2, 50, SW - 4, 2, 15, 15);
				break;
			case 1: { // RETRIG — count
				const ret = FW_RETRIG[i];
				if (ret > 0) {
					c.setColorValue(Color.MediumBright);
					c.drawTextAligned(x, 48, SW, 6, HAlign.Center, VAlign.Center, String(ret + 1));
				}
				break;
			}
			case 2: // LEN — already visible as bar width in pitch strip; no redundant indicator
				break;
			case 3: { // NOTE — dim octave + bright scale degree
				const [oct, deg] = v1IdxToDisplay(V1_NOTE_IDX[i]);
				const octStr = oct === 0 ? '0' : (oct > 0 ? '+' + oct : String(oct));
				const degStr = String(deg);
				const octW = c.textWidth(octStr);
				const totalW = octW + 1 + c.textWidth(degStr);
				const tx = x + Math.floor((SW - totalW) / 2);
				c.setColorValue(Color.Medium);  c.drawText(tx, 53, octStr);
				c.setColorValue(Color.Bright);   c.drawText(tx + octW + 1, 53, degStr);
				break;
			}
			case 4: { // COND
				const cond = FW_CONDS[i];
				if (cond) {
					c.setColorValue(Color.MediumBright);
					c.drawTextAligned(x, 48, SW, 6, HAlign.Center, VAlign.Center, COND_SHORT4[cond] ?? '');
				}
				break;
			}
			}
		}

		fwDrawFooter(c, FW_TABS, tab);
	}

	function ledsNoteV2(): LedState { return ledsFirmwareNote(); }

	// ── V3 — 2×8 grid layout ─────────────────────────────────────────────────
	// Mirrors the hardware's physical 2-row × 8-column button layout.
	// Each step column is 32px wide (vs 16px in 1×16) — enough for legible text,
	// pitch bars inside cells, and proper visual encoding per tab.
	// Cost: the time axis wraps at step 8→9 (top row to bottom row).

	function drawNoteV3(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);

		const SW = 32;         // step column width (256 / 8)
		const R1Y = 10;        // row 1: loop indicator at this y
		const R2Y = 32;        // row 2: loop indicator at this y
		// cell outer: x+1, rowY+1, 30×19
		// cell inner: x+3, rowY+3, 26×15
		const CELL_W = 30;
		const CELL_H = 19;
		const INNER_W = 26;
		const INNER_H = 15;

		// 1. Header
		fwDrawHeader(c, FW_TABS[tab]);

		// 2. Loop indicators — 1px above each in-loop cell, L-brackets at endpoints
		c.setColorValue(Color.Bright);
		for (let i = 0; i < 16; i++) {
			if (i < FW_FIRST || i > FW_LAST) continue;
			const col = i % 8;
			const rowY = i < 8 ? R1Y : R2Y;
			const x = col * SW;
			if (i === FW_FIRST) {
				c.vline(x + 1, rowY, 3);
				c.point(x + 2, rowY + 1);
			} else if (i === FW_LAST) {
				c.vline(x + SW - 2, rowY, 3);
				c.point(x + SW - 3, rowY + 1);
			} else {
				c.point(x + SW / 2, rowY + 1);
			}
		}

		// 3. Adaptive MIDI range for NOTE tab (computed once)
		const activeMidis: number[] = [];
		for (let j = 0; j < 16; j++) {
			if (TRACKS[0].gates[j]) activeMidis.push(v1IdxToMidi(V1_NOTE_IDX[j]));
		}
		let midiMin = activeMidis.length > 0 ? Math.min(...activeMidis) : 57;
		let midiMax = activeMidis.length > 0 ? Math.max(...activeMidis) : 69;
		if (midiMax === midiMin) { midiMin -= 3; midiMax += 3; }
		else { midiMin -= 1; midiMax += 1; }
		const midiRange = midiMax - midiMin;

		// 4. Step cells
		for (let i = 0; i < 16; i++) {
			const col = i % 8;
			const rowY = i < 8 ? R1Y : R2Y;
			const x = col * SW;
			const cellY = rowY + 1;
			const innerX = x + 3;
			const innerY = rowY + 3;
			const on = Boolean(TRACKS[0].gates[i]);
			const isCursor = i === FW_CURSOR;
			const isPlay = i === PLAY_CURSOR;
			const isBeat = i % 4 === 0;

			// Beat marker: extra top row on cells at steps 0,4,8,12
			if (isBeat) {
				c.setColorValue(isPlay ? Color.Bright : Color.MediumLow);
				c.hline(x + 1, cellY - 1, CELL_W);
			}

			// ── Tab-specific cell rendering ──────────────────────────────────

			if (tab === 0) {
				// GATE — gate fill / outline + probability bar
				if (isCursor) {
					c.setColorValue(Color.Bright);
					c.fillRect(x + 1, cellY, CELL_W, CELL_H);
					c.setColorValue(Color.None);
					c.rect(innerX, innerY, INNER_W, INNER_H);
				} else {
					c.setColorValue(isPlay ? Color.Bright : Color.MediumLow);
					c.rect(x + 1, cellY, CELL_W, CELL_H);
					if (on) {
						c.setColorValue(Color.Bright);
						c.fillRect(innerX, innerY, INNER_W, INNER_H);
					}
				}
				// Probability bar (only when < max, to avoid noise on full-prob steps)
				// Shown as 2px bar at top of inner area, bright/dim proportional split
				// (in our fixture all probs = 15, so this stays hidden — real data shows it)

			} else if (tab === 3) {
				// NOTE — pitch bar at height-mapped Y + scale degree label
				c.setColorValue(Color.MediumLow);
				c.rect(x + 1, cellY, CELL_W, CELL_H);
				if (on) {
					const BAR_H = 3;
					const BAR_W = 22;
					const midi = v1IdxToMidi(V1_NOTE_IDX[i]);
					const pitchT = (midi - midiMin) / midiRange;
					const barY = innerY + Math.round((1 - pitchT) * (INNER_H - BAR_H));

					c.setColorValue(isCursor || isPlay ? Color.Bright : Color.MediumBright);
					c.fillRect(x + 5, barY, BAR_W, BAR_H);

					// Scale degree: dim octave + bright degree below bar
					const [oct, deg] = v1IdxToDisplay(V1_NOTE_IDX[i]);
					const octStr = oct === 0 ? '0' : (oct > 0 ? '+' + oct : String(oct));
					const degStr = String(deg);
					const octW = c.textWidth(octStr);
					const totalW = octW + 1 + c.textWidth(degStr);
					const tx = x + 1 + Math.floor((SW - 2 - totalW) / 2);
					const textY = Math.min(barY + BAR_H + 2, innerY + INNER_H - 5);
					c.setColorValue(Color.Medium);
					c.drawText(tx, textY, octStr);
					c.setColorValue(isCursor || isPlay ? Color.Bright : Color.MediumBright);
					c.drawText(tx + octW + 1, textY, degStr);
				}

			} else if (tab === 1) {
				// RETRIG — count centered in cell
				if (on) {
					c.setColorValue(Color.MediumLow);
					c.rect(x + 1, cellY, CELL_W, CELL_H);
					const ret = FW_RETRIG[i];
					if (ret > 0) {
						const retStr = 'x' + String(ret + 1);
						c.setColorValue(isCursor || isPlay ? Color.Bright : Color.MediumBright);
						c.drawTextAligned(x + 1, cellY, CELL_W, CELL_H, HAlign.Center, VAlign.Center, retStr);
					}
				}

			} else if (tab === 2) {
				// LEN — proportional-width fill = gate length
				c.setColorValue(Color.MediumLow);
				c.rect(x + 1, cellY, CELL_W, CELL_H);
				if (on) {
					const fillW = Math.max(2, Math.round(INNER_W * (FW_LENS[i] + 1) / 16));
					c.setColorValue(isCursor || isPlay ? Color.Bright : Color.MediumBright);
					c.fillRect(innerX, innerY, fillW, INNER_H);
				}

			} else if (tab === 4) {
				// COND — condition abbreviation centered in cell
				if (on) {
					c.setColorValue(Color.MediumLow);
					c.rect(x + 1, cellY, CELL_W, CELL_H);
					const cond = FW_CONDS[i];
					if (cond) {
						const condStr = COND_SHORT4[cond] ?? '';
						if (condStr) {
							c.setColorValue(isCursor || isPlay ? Color.Bright : Color.MediumBright);
							c.drawTextAligned(x + 1, cellY, CELL_W, CELL_H, HAlign.Center, VAlign.Center, condStr);
						}
					}
				}
			}
		}

		fwDrawFooter(c, FW_TABS, tab);
	}

	function ledsNoteV3(): LedState { return ledsFirmwareNote(); }

	// ── Curve Track Trunk ────────────────────────────────────────────────────
	// Faithful port of CurveSequenceEditPage::draw() from C++.
	// Tabs: SHPE | MIN | MAX | GATE | —

	// Curve shape functions — ported from src/apps/sequencer/model/Curve.cpp
	// Array index matches Curve::Type enum order exactly (0=Low … 38=Trigger).
	const CURVE_FNS: ((x: number) => number)[] = [
		() => 0,                                                              // 0  Low
		() => 1,                                                              // 1  High
		x => x < 0.5 ? 0 : 1,                                                // 2  StepUp
		x => x < 0.5 ? 1 : 0,                                                // 3  StepDown
		x => x,                                                               // 4  RampUp
		x => 1 - x,                                                           // 5  RampDown
		x => x < 0.5 ? x * 2 : 0,                                            // 6  rampUpHalf
		x => x < 0.5 ? 1 - x * 2 : 0,                                        // 7  rampDownHalf
		x => (x * 2) % 1,                                                     // 8  doubleRampUpHalf
		x => 1 - (x * 2) % 1,                                                 // 9  doubleRampDownHalf
		x => x * x,                                                           // 10 ExpUp
		x => (1 - x) * (1 - x),                                               // 11 ExpDown
		x => x < 0.5 ? (x * 2) * (x * 2) : 0,                               // 12 expUpHalf
		x => x < 0.5 ? (1 - x * 2) * (1 - x * 2) : 0,                       // 13 expDownHalf
		x => { const t = (x * 2) % 1; return t * t; },                       // 14 doubleExpUpHalf
		x => { const t = 1 - (x * 2) % 1; return x < 1 ? t * t : 0; },      // 15 doubleExpDownHalf
		x => Math.sqrt(x),                                                    // 16 LogUp
		x => Math.sqrt(1 - x),                                                // 17 LogDown
		x => x < 0.5 ? Math.sqrt(x * 2) : 0,                                 // 18 logUpHalf
		x => x < 0.5 ? Math.sqrt(1 - x * 2) : 0,                             // 19 logDownHalf
		x => Math.sqrt((x * 2) % 1),                                          // 20 doubleLogUpHalf
		x => { const t = 1 - (x * 2) % 1; return x < 1 ? Math.sqrt(t) : 0; }, // 21 doubleLogDownHalf
		x => x * x * (3 - 2 * x),                                            // 22 SmoothUp (smoothstep)
		x => 1 - x * x * (3 - 2 * x),                                        // 23 SmoothDown
		x => { const t = x * 2; return x < 0.5 ? t * t * (3 - 2 * t) : 0; }, // 24 smoothUpHalf
		x => { const t = x * 2; return x < 0.5 ? 1 - t * t * (3 - 2 * t) : 0; }, // 25 smoothDownHalf
		x => { const t = (x * 2) % 1; return t * t * (3 - 2 * t); },        // 26 doubleSmoothUpHalf
		x => { const t = (x * 2) % 1; return x < 1 ? 1 - t * t * (3 - 2 * t) : 0; }, // 27 doubleSmoothDownHalf
		x => (x < 0.5 ? x : 1 - x) * 2,                                     // 28 Triangle
		x => 1 - (x < 0.5 ? x : 1 - x) * 2,                                 // 29 RevTriangle
		x => 0.5 - 0.5 * Math.cos(x * Math.PI * 2),                          // 30 Bell
		x => 1 - (0.5 - 0.5 * Math.cos(x * Math.PI * 2)),                    // 31 RevBell
		x => { const t = 1 - (x * 2) % 1; return x < 1 ? t * t : 0; },      // 32 ExpDown2x
		x => { const t = (x * 2) % 1; return x < 1 ? t * t : 0; },          // 33 ExpUp2x
		x => { const t = 1 - (x * 3) % 1; return x < 1 ? t * t : 0; },      // 34 ExpDown3x
		x => { const t = (x * 3) % 1; return x < 1 ? t * t : 0; },          // 35 ExpUp3x
		x => { const t = 1 - (x * 4) % 1; return x < 1 ? t * t : 0; },      // 36 ExpDown4x
		x => { const t = (x * 4) % 1; return x < 1 ? t * t : 0; },          // 37 ExpUp4x
		x => x < 0.1 ? 1 : x < 0.3 ? (1 - (x * 2) % 1) ** 2 : 0,           // 38 Trigger
	];

	const CURVE_TABS = ['SHPE', 'MIN', 'MAX', 'GATE', '—'];

	// Fixture: varied shapes across all 16 steps to show the display well
	const CV_SHAPES = [4, 10, 16, 22, 5, 11, 17, 23, 28, 30,  2, 38, 33, 32, 35,  4];
	const CV_MINS   = [0,  0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0];
	const CV_MAXS   = [1,  1,  1,  1, 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1];
	// Gate bitmask: 4 bits = 4 sub-steps. 0b1111=all on, 0b1010=alt, etc.
	const CV_GATES  = [0xf, 0xa, 0xc, 0x9, 0xf, 0x5, 0xe, 0x3,
	                   0xf, 0xa, 0xc, 0x9, 0xf, 0x5, 0xe, 0x3];
	const CV_CURSOR = 4;
	const CV_FIRST  = 0;
	const CV_LAST   = 15;
	const CV_PLAY   = 4;

	function drawCurveTrunk(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);

		const SW      = 16;
		const loopY   = 16;
		const stepY   = 20;
		const curveY  = 24;
		const curveH  = 20;
		const bottomY = 48;

		// Header
		fwDrawHeader(c, CURVE_TABS[tab]);

		// Loop brackets (identical to Note trunk logic)
		c.setColorValue(Color.Bright);
		const lsx = CV_FIRST * SW + 1;
		c.vline(lsx, loopY - 1, 3);
		c.point(lsx + 1, loopY);
		const lex = CV_LAST * SW + SW - 2;
		c.vline(lex, loopY - 1, 3);
		c.point(lex - 1, loopY);
		for (let i = 0; i < 16; i++) {
			if (i > CV_FIRST && i <= CV_LAST) c.point(i * SW, loopY);
		}

		// Dotted vertical grid between steps (SHPE/MIN/MAX tabs only)
		if (tab !== 3) {
			c.setColorValue(Color.Low);
			for (let si = 1; si < 16; si++) {
				const gx = si * SW;
				for (let gy = 0; gy <= curveH; gy += 2) c.point(gx, curveY + gy);
			}
		}

		// Curves + per-step indicators
		let lastY = -1;
		for (let i = 0; i < 16; i++) {
			const x      = i * SW;
			const shape  = Math.min(CV_SHAPES[i], CURVE_FNS.length - 1);
			const min    = CV_MINS[i];
			const max    = CV_MAXS[i];
			const isCursor = i === CV_CURSOR;

			// Step number
			const numStr = String(i + 1);
			c.setColorValue(isCursor ? Color.Bright : Color.Medium);
			c.setBlendMode(BlendMode.Set);
			c.drawText(x + Math.floor((SW - c.textWidth(numStr) + 1) / 2), stepY - 2, numStr);

			// Draw curve segment in Add blend so crossings stay bright
			const fn = CURVE_FNS[shape];
			const evalY = (fx: number) => (1 - (fn(fx) * (max - min) + min)) * curveH;

			c.setColorValue(Color.Bright);
			c.setBlendMode(BlendMode.Add);

			let fy0 = curveY + evalY(0);
			if (lastY >= 0 && Math.abs(lastY - fy0) > 0.5) c.line(x, lastY, x, fy0);
			for (let px = 0; px < SW; px++) {
				const fy1 = curveY + evalY((px + 1) / SW);
				c.line(x + px, fy0, x + px + 1, fy1);
				fy0 = fy1;
			}
			lastY = fy0;

			c.setBlendMode(BlendMode.Set);

			// Tab-specific layer indicators
			switch (tab) {
			case 1: { // MIN — dim hline at min Y
				c.setColorValue(Color.MediumLow);
				c.setBlendMode(BlendMode.Add);
				c.hline(x, curveY + Math.round((1 - min) * curveH), SW);
				c.setBlendMode(BlendMode.Set);
				break;
			}
			case 2: { // MAX — dim hline at max Y
				c.setColorValue(Color.MediumLow);
				c.setBlendMode(BlendMode.Add);
				c.hline(x, curveY + Math.round((1 - max) * curveH), SW);
				c.setBlendMode(BlendMode.Set);
				break;
			}
			case 3: { // GATE — 4-bit sub-step pattern blocks at bottomY
				const gate = CV_GATES[i];
				const gs = SW / 4;
				const gw = Math.max(1, Math.floor(SW / 8));
				for (let g = 0; g < 4; g++) {
					c.setColorValue((gate & (1 << g)) ? Color.Bright : Color.Medium);
					c.fillRect(x + g * gs, bottomY, gw, 2);
				}
				break;
			}
			}
		}

		// Play cursor: full-height vertical scanline through curve area
		c.setColorValue(Color.Bright);
		c.setBlendMode(BlendMode.Set);
		c.vline(CV_PLAY * SW, curveY, curveH);

		fwDrawFooter(c, CURVE_TABS, tab);
	}

	function ledsCurveTrunk(): LedState {
		const s = defaultLeds();
		s.play = 'green';
		s.track[0] = 'green';
		s.fkey[0] = 'green';
		return s;
	}

	// ── Curve V1 — segment assembler ─────────────────────────────────────────

	const SEG_TABS    = ['SKEW', 'LEN', 'SHPE', 'LVL', 'OFST'];
	const SEG_COUNT   = 7;
	const SEG_SHAPES  = [0.08, 0.5,  0.9,  0.25, 0.5,  1.0,  0.15, 0,0,0,0,0,0,0,0,0];
	const SEG_SKEWS   = [0.5,  0.3,  0.5,  0.5,  0.2,  0.5,  0.8,  0,0,0,0,0,0,0,0,0];
	const SEG_LENGTHS = [3,    2,    4,    3,    2,    3,    3,    0,0,0,0,0,0,0,0,0];
	const SEG_LEVELS  = [0.9,  0.55, 0.75, 0.45, 0.65, 0.85, 0.4,  0,0,0,0,0,0,0,0,0];
	const SEG_OFFSETS = [0.0,  0.1,  0.0,  0.15, 0.0,  0.0,  0.05, 0,0,0,0,0,0,0,0,0];
	const SEG_CURSOR  = 2;
	const SEG_PLAY    = 6;

	function evalSegment(phase: number, shape: number, skew: number): number {
		const sk = Math.max(0.02, Math.min(0.98, skew));
		const t  = phase < sk
			? (phase / sk) * 0.5
			: 0.5 + ((phase - sk) / (1 - sk)) * 0.5;
		const s = Math.sin(t * Math.PI);
		const p = shape <= 0.5
			? 1 + (1 - shape * 2) * 7
			: Math.pow(1 - (shape - 0.5) * 2, 2);
		return p <= 0 ? 1 : Math.pow(Math.max(0, s), p);
	}

	function drawCurveV1(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);

		const NUM_Y   = 9;
		const CURVE_Y = 16;
		const CURVE_H = 28;
		const IND_Y   = 45;

		fwDrawHeader(c, SEG_TABS[tab]);

		// Compute segment pixel ranges
		const totalLen = SEG_LENGTHS.slice(0, SEG_COUNT).reduce((a, b) => a + b, 0);
		const segX: number[] = [];
		const segW: number[] = [];
		let xAcc = 0;
		for (let i = 0; i < SEG_COUNT; i++) {
			const x0 = Math.round(xAcc);
			xAcc += (SEG_LENGTHS[i] / totalLen) * 256;
			const x1 = Math.round(xAcc);
			segX.push(x0);
			segW.push(x1 - x0);
		}

		// Segment numbers
		c.setBlendMode(BlendMode.Set);
		for (let i = 0; i < SEG_COUNT; i++) {
			const isCursor = i === SEG_CURSOR;
			c.setColorValue(isCursor ? Color.Bright : Color.Low);
			const str = String(i + 1);
			const tw  = c.textWidth(str);
			c.drawText(segX[i] + Math.floor((segW[i] - tw) / 2), NUM_Y + 5, str);
		}

		// Segment boundary lines (behind curves)
		c.setBlendMode(BlendMode.Set);
		c.setColorValue(Color.Low);
		for (let i = 1; i < SEG_COUNT; i++) {
			c.vline(segX[i], CURVE_Y, CURVE_H);
		}

		// Draw curves per segment
		for (let i = 0; i < SEG_COUNT; i++) {
			const x0     = segX[i];
			const w      = segW[i];
			const isCursor = i === SEG_CURSOR;

			c.setColorValue(isCursor ? Color.Bright : Color.Low);
			c.setBlendMode(BlendMode.Add);

			let prevY = -1;
			for (let px = 0; px <= w; px++) {
				const phase = w > 1 ? px / w : 0.5;
				const amp   = evalSegment(phase, SEG_SHAPES[i], SEG_SKEWS[i]);
				const yPx   = CURVE_Y + CURVE_H - 1
					- Math.round((SEG_OFFSETS[i] + SEG_LEVELS[i] * amp) * (CURVE_H - 1));
				if (prevY >= 0) c.line(x0 + px - 1, prevY, x0 + px, yPx);
				prevY = yPx;
			}
		}

		// Play cursor scanline
		let playPulse = 0;
		let playX = 0;
		for (let i = 0; i < SEG_COUNT; i++) {
			if (SEG_PLAY >= playPulse && SEG_PLAY < playPulse + SEG_LENGTHS[i]) {
				const frac = (SEG_PLAY - playPulse) / SEG_LENGTHS[i];
				playX = segX[i] + Math.round(frac * segW[i]);
				break;
			}
			playPulse += SEG_LENGTHS[i];
		}
		c.setColorValue(Color.Bright);
		c.setBlendMode(BlendMode.Add);
		c.vline(playX, CURVE_Y, CURVE_H);

		// Indicator strip separator
		c.setBlendMode(BlendMode.Set);
		c.setColorValue(Color.Low);
		c.hline(0, IND_Y, 256);

		// Indicator strip per segment
		for (let i = 0; i < SEG_COUNT; i++) {
			const x0 = segX[i];
			const w  = segW[i];
			const isCursor = i === SEG_CURSOR;
			c.setColorValue(isCursor ? Color.Bright : Color.Low);

			// Boundary tick
			if (i > 0) c.vline(x0, IND_Y + 1, 3);

			if (tab === 0) {
				// SKEW: vertical mark at peak position
				const peakX = x0 + Math.round(SEG_SKEWS[i] * w);
				c.vline(peakX, IND_Y + 2, 5);
			} else if (w >= 10) {
				// Numeric label centered in segment
				let str: string;
				switch (tab) {
				case 1: str = String(SEG_LENGTHS[i]); break;
				case 2: str = String(Math.round(SEG_SHAPES[i] * 100)); break;
				case 3: str = String(Math.round(SEG_LEVELS[i] * 100)); break;
				case 4: str = String(Math.round(SEG_OFFSETS[i] * 100)); break;
				default: str = '';
				}
				if (str) {
					const tw = c.textWidth(str);
					c.drawText(x0 + Math.floor((w - tw) / 2), IND_Y + 7, str);
				}
			}
		}

		c.setBlendMode(BlendMode.Set);
		fwDrawFooter(c, SEG_TABS, tab);
	}

	function ledsCurveV1(): LedState {
		const s = defaultLeds();
		s.play = 'green';
		s.track[0] = 'green';
		for (let i = 0; i < SEG_COUNT; i++) s.step[i] = 'green';
		s.step[SEG_CURSOR] = 'amber';
		s.fkey[0] = 'green';
		return s;
	}

	// ── Stochastic track — Trunk (current firmware port) ─────────────────────
	// 12 steps at 16px each (same stepWidth as Note track), centered:
	//   x = i * 16 + 32   →   steps span x=32..208, leaving 32px margins each side.
	// This centering is the visual signature of the Stochastic firmware page.

	const STCH_TRUNK_TABS   = ['GATE', 'RETRIG', 'LEN', 'NOTE', 'COND'];
	const STCH_TRUNK_CURSOR = 3;
	const STCH_TRUNK_PLAY   = 1;
	const STCH_TRUNK_NSTEPS = 12;
	const STCH_TRUNK_SW     = 16;
	const STCH_TRUNK_XOFF   = ((16 - 12) / 2) * 16;  // = 32

	const STCH_TRUNK_GATES = [1,0,1,0,1,0,0,1,0,1,0,0];
	const STCH_TRUNK_GPROB = [15,0, 8,0,12,0,0, 4,0,15,0,0];  // GateProbability 0–15
	const STCH_TRUNK_RPROB = [ 0,0, 6,0,10,0,0, 8,0, 0,0,0];  // RetriggerProbability 0–15
	const STCH_TRUNK_LEN   = [ 8,0, 4,0,12,0,0, 6,0, 8,0,0];  // Length 0–15
	const STCH_TRUNK_NOTE  = [60,0,62,0,64,0,0,67,0,60,0,0];  // MIDI note
	const STCH_TRUNK_COND  = [ 0,0, 0,0, 2,0,0, 0,0, 0,0,0];  // condition index

	function drawStochasticTrunk(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);

		const SW   = STCH_TRUNK_SW;
		const XOFF = STCH_TRUNK_XOFF;
		const y    = 20;

		fwDrawHeader(c, STCH_TRUNK_TABS[tab]);

		for (let i = 0; i < STCH_TRUNK_NSTEPS; i++) {
			const x        = i * SW + XOFF;
			const on       = Boolean(STCH_TRUNK_GATES[i]);
			const isCursor = i === STCH_TRUNK_CURSOR;
			const isPlay   = i === STCH_TRUNK_PLAY;

			// Step number (same as Note track)
			const numStr = String(i + 1);
			c.setColorValue(isCursor ? Color.Bright : Color.Medium);
			c.drawText(x + (SW - c.textWidth(numStr) + 1) / 2, y - 2, numStr);

			// Gate box 12×12 (identical to Note track)
			c.setColorValue(isPlay ? Color.Bright : Color.Medium);
			c.rect(x + 2, y + 2, SW - 4, SW - 4);

			if (on) {
				c.setColorValue(Color.Bright);
				c.fillRect(x + 4, y + 4, SW - 8, SW - 8);
			}

			// Sub-indicator at y+18 (identical Y to Note track)
			switch (tab) {
			case 0:  // GATE → GateProbability bar
				fwDrawProbability(c, x + 2, y + 18, SW - 4, 2, STCH_TRUNK_GPROB[i], 15);
				break;
			case 1:  // RETRIG → RetriggerProbability bar
				fwDrawProbability(c, x + 2, y + 18, SW - 4, 2, STCH_TRUNK_RPROB[i], 15);
				break;
			case 2:  // LEN → gate length bracket
				if (on) fwDrawLength(c, x + 2, y + 18, SW - 4, 6, STCH_TRUNK_LEN[i], 15);
				break;
			case 3:  // NOTE → note name two-line (Short1 + Short2)
				if (on) {
					const midi   = STCH_TRUNK_NOTE[i];
					const letter = NOTE_NAMES_12[midi % 12];
					const oct    = String(Math.floor(midi / 12) - 1);
					c.setColorValue(Color.Bright);
					c.drawText(x + (SW - c.textWidth(letter) + 1) / 2, y + 20, letter);
					c.drawText(x + (SW - c.textWidth(oct) + 1) / 2,    y + 27, oct);
				}
				break;
			case 4:  // COND → condition abbreviation
				if (on && STCH_TRUNK_COND[i]) {
					const s = COND_SHORT4[STCH_TRUNK_COND[i]] ?? '';
					c.setColorValue(Color.Bright);
					c.drawText(x + (SW - c.textWidth(s) + 1) / 2, y + 20, s);
				}
				break;
			}
		}

		fwDrawFooter(c, STCH_TRUNK_TABS, tab);
	}

	function ledsStochasticTrunk(): LedState {
		const s = defaultLeds();
		s.play = 'green';
		s.track[3] = 'green';
		for (let i = 0; i < STCH_TRUNK_NSTEPS; i++) {
			s.step[i] = i === STCH_TRUNK_CURSOR ? 'amber' : (STCH_TRUNK_GATES[i] ? 'green' : 'off');
		}
		s.fkey[0] = 'green';
		return s;
	}

	// ── Stochastic track ──────────────────────────────────────────────────────

	const STOCH_TABS   = ['GATE', 'LEN', 'NOTE', 'PROB', '—'];
	const STOCH_CURSOR = 4;
	const STOCH_PLAY   = 2;
	const STOCH_FIRST  = 0;
	const STOCH_LAST   = 15;
	const STOCH_SEED   = 42;
	const STOCH_RESTPR = 3;   // sequence-level rest probability 0–8

	const STOCH_GATES  = [1,0,1,0,1,0,0,1,0,1,0,0,1,0,1,0];
	const STOCH_GPROB  = [15,0, 8,0,12,0,0, 4,0,15,0,0, 6,0,10,0];
	const STOCH_LEN    = [ 8,0, 4,0,12,0,0, 6,0, 8,0,0, 4,0,10,0];
	const STOCH_NOTE   = [ 0,0, 4,0, 2,0,0, 7,0, 0,0,0, 5,0, 2,0];
	const STOCH_REST2  = [ 0,0, 2,0, 1,0,0, 3,0, 0,0,0, 2,0, 1,0];

	function drawStochastic(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);

		const SW    = 16;
		const loopY = 16;
		const numY  = 13;
		const cellY = 19;
		const cellH = 20;
		const indY  = 42;

		const LAYER_NAMES = ['GATE', 'LEN', 'NOTE', 'PROB'];
		fwDrawHeader(c, LAYER_NAMES[tab]);

		// Seed + global rest probability in spare header space
		c.setColorValue(Color.MediumLow);
		c.drawText(162, 6, 'S:' + String(STOCH_SEED));
		c.drawText(195, 6, 'R:' + String(STOCH_RESTPR));

		// Loop brackets
		c.setColorValue(Color.Bright);
		const lsx = STOCH_FIRST * SW + 1;
		c.vline(lsx, loopY - 1, 3);
		c.point(lsx + 1, loopY);
		const lex = STOCH_LAST * SW + SW - 2;
		c.vline(lex, loopY - 1, 3);
		c.point(lex - 1, loopY);
		for (let i = 0; i < 16; i++) {
			if (i > STOCH_FIRST && i <= STOCH_LAST) c.point(i * SW, loopY);
		}

		// Step cells
		for (let i = 0; i < 16; i++) {
			const x        = i * SW;
			const gate     = Boolean(STOCH_GATES[i]);
			const prob     = STOCH_GPROB[i];
			const isCursor = i === STOCH_CURSOR;

			c.setBlendMode(BlendMode.Set);

			// Step number — Bright for cursor, MediumLow otherwise
			const numStr = String(i + 1);
			const nw = c.textWidth(numStr);
			c.setColorValue(isCursor ? Color.Bright : Color.MediumLow);
			c.drawText(x + Math.floor((SW - nw) / 2), numY, numStr);

			if (!gate) continue;  // absence: no cell for gate-off steps

			// Probability fill from bottom up
			const fillH  = Math.max(1, Math.round(cellH * prob / 15));
			const emptyH = cellH - fillH;

			c.setColorValue(Color.Bright);
			c.fillRect(x + 1, cellY + emptyH, SW - 2, fillH);
		}

		// Play cursor: vertical scanline through cell area
		c.setColorValue(Color.Bright);
		c.setBlendMode(BlendMode.Add);
		c.vline(STOCH_PLAY * SW, cellY - 1, cellH + 2);
		c.setBlendMode(BlendMode.Set);

		// Indicator strip separator
		c.setColorValue(Color.MediumLow);
		c.hline(0, indY, 256);

		// Indicator strip — tab-specific secondary layer
		for (let i = 0; i < 16; i++) {
			if (!STOCH_GATES[i]) continue;
			const x        = i * SW;
			const isCursor = i === STOCH_CURSOR;

			c.setBlendMode(BlendMode.Set);

			switch (tab) {
			case 0: {  // GATE — probability bar (reinforces cell fill at precise scale)
				const bw = Math.max(1, Math.round((SW - 2) * STOCH_GPROB[i] / 15));
				c.setColorValue(isCursor ? Color.Bright : Color.MediumLow);
				c.fillRect(x + 1, indY + 3, bw, 4);
				break;
			}
			case 1: {  // LEN — length proportion bar
				const bw = Math.max(1, Math.round((SW - 2) * STOCH_LEN[i] / 15));
				c.setColorValue(isCursor ? Color.Bright : Color.MediumLow);
				c.fillRect(x + 1, indY + 3, bw, 4);
				break;
			}
			case 2: {  // NOTE — scale degree label
				const s  = String(STOCH_NOTE[i]);
				const tw = c.textWidth(s);
				c.setColorValue(isCursor ? Color.Bright : Color.MediumLow);
				c.drawText(x + Math.floor((SW - tw) / 2), indY + 8, s);
				break;
			}
			case 3: {  // PROB — rest probability 2× bar
				const bw = Math.max(1, Math.round((SW - 2) * STOCH_REST2[i] / 8));
				c.setColorValue(isCursor ? Color.Bright : Color.MediumLow);
				c.fillRect(x + 1, indY + 3, bw, 4);
				break;
			}
			}
		}

		c.setBlendMode(BlendMode.Set);
		fwDrawFooter(c, STOCH_TABS, tab);
	}

	function ledsStochastic(): LedState {
		const s = defaultLeds();
		s.play = 'green';
		s.track[3] = 'green';
		STOCH_GATES.forEach((on, i) => {
			s.step[i] = i === STOCH_CURSOR ? 'amber' : (on ? 'green' : 'off');
		});
		s.fkey[0] = 'green';
		return s;
	}

	// ── Stochastic V2 — SIG-inspired (NOTE|OCT|LEN|ART|LOOP) ────────────────
	// Core concept from the Stochastic Inspiration Generator:
	// 7 scale-degree slots with probability weights, no step positions.
	// Bar height = selection weight + automatic density (taller = more likely & denser).

	const STCH_V2_TABS        = ['NOTE', 'OCT', 'LEN', 'LOOP'];
	const STCH_V2_CURSOR      = 2;    // degree 3 is the edit cursor
	const STCH_V2_PLAY        = 4;    // degree 5 is currently playing

	//                                  1   2   3   4   5   6   7
	const STCH_V2_NPROB: number[]     = [15,  5, 12,  8, 15,  6,  4];
	const STCH_V2_OPROB: number[]     = [ 0,  3, 15,  5,  0];   // -2  -1   0  +1  +2
	const STCH_V2_DPROB: number[]     = [15,  8,  5,  2,  0,  0]; // 1/16 1/8 1/4 1/2 1 2
	const STCH_V2_DUR_LABELS          = ['1/16','1/8','1/4','1/2','1','2'];
	const STCH_V2_OCT_LABELS          = ['-2','-1',' 0','+1','+2'];
	// Loop state: step buttons set length, encoder sets chance (0=locked, 1=fully random)
	const STCH_V2_LOOP_LEN    =  8;    // active loop length (1–16 steps)
	const STCH_V2_LOOP_CHANCE = 0.3;   // mutation chance: 0.0=locked, 1.0=fully random
	const STCH_V2_LOOP_GATES  = [true, false, true, true, false, true, true, false,
	                              false, false, false, false, false, false, false, false];

	function drawStochasticV2(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);

		const SW        = 36;   // column width: 7×36=252, 2px margins each side
		const XOFF      = 2;
		const barFloor  = 48;   // y of bar bottom — leave room for degree labels
		const barTop    = 10;   // y of bar top (fwDrawHeader separator is at y=9)
		const barHMax   = barFloor - barTop + 1;  // 39px

		// ── header ──────────────────────────────────────────────────────────
		fwDrawHeader(c, STCH_V2_TABS[tab]);

		// ── content ─────────────────────────────────────────────────────────
		if (tab === 0) {
			// NOTE — 7 scale-degree probability bars
			for (let i = 0; i < 7; i++) {
				const x        = XOFF + i * SW;
				const prob     = STCH_V2_NPROB[i];
				const barH     = Math.round(prob * barHMax / 15);
				const isPlay   = i === STCH_V2_PLAY;
				const isCursor = i === STCH_V2_CURSOR;

				// Cursor: inverted column (Bright background, dark bar via Sub blend)
				if (isCursor) {
					c.setColorValue(Color.Bright);
					c.fillRect(x, barTop, SW, barHMax);
					if (barH > 0) {
						c.setBlendMode(BlendMode.Sub);
						c.setColorValue(Color.Bright);
						c.fillRect(x + 2, barFloor - barH + 1, SW - 4, barH);
						c.setBlendMode(BlendMode.Set);
					}
				} else {
					// Normal bar: Bright fill from bottom up
					if (barH > 0) {
						c.setColorValue(Color.Bright);
						c.fillRect(x + 2, barFloor - barH + 1, SW - 4, barH);
					}
				}

				// Playing note: bright outline rect around column
				if (isPlay) {
					c.setColorValue(Color.Bright);
					c.rect(x, barTop, SW, barHMax);
				}

				// Degree label (1–7) below bar
				const lbl = String(i + 1);
				c.setColorValue(Color.MediumLow);
				c.drawText(x + Math.floor((SW - c.textWidth(lbl)) / 2), 53, lbl);
			}

		} else if (tab === 1) {
			// OCT — 5 octave probability bars, centered
			const bw   = 36;
			const xOff = Math.round((256 - 5 * bw) / 2);
			for (let i = 0; i < 5; i++) {
				const x    = xOff + i * bw;
				const prob = STCH_V2_OPROB[i];
				const barH = Math.round(prob * barHMax / 15);
				if (barH > 0) {
					c.setColorValue(Color.Bright);
					c.fillRect(x + 2, barFloor - barH + 1, bw - 4, barH);
				}
				// "0 octave" column: Low tick at top marks the default centre
				if (i === 2) {
					c.setColorValue(Color.MediumLow);
					c.hline(x, barTop, bw);
				}
				const lbl = STCH_V2_OCT_LABELS[i];
				c.setColorValue(Color.MediumLow);
				c.drawText(x + Math.floor((bw - c.textWidth(lbl)) / 2), 53, lbl);
			}

		} else if (tab === 2) {
			// LEN — 6 duration probability bars, centered
			const bw   = 36;
			const xOff = Math.round((256 - 6 * bw) / 2);
			for (let i = 0; i < 6; i++) {
				const x    = xOff + i * bw;
				const prob = STCH_V2_DPROB[i];
				const barH = Math.round(prob * barHMax / 15);
				if (barH > 0) {
					c.setColorValue(Color.Bright);
					c.fillRect(x + 2, barFloor - barH + 1, bw - 4, barH);
				}
				const lbl = STCH_V2_DUR_LABELS[i];
				c.setColorValue(Color.MediumLow);
				c.drawText(x + Math.floor((bw - c.textWidth(lbl)) / 2), 53, lbl);
			}

		} else {
			// LOOP — left: chance box (encoder target), right: 8×2 step grid
			const cY = 10;
			const cH = 45;

			// ── Chance box (left) ────────────────────────────────────────────
			const boxX = 1;
			const boxW = 50;
			c.setColorValue(Color.Bright);
			c.rect(boxX, cY, boxW, cH);

			const chcLbl = 'CHC';
			c.setColorValue(Color.Low);
			c.drawText(boxX + Math.floor((boxW - c.textWidth(chcLbl)) / 2), cY + 7, chcLbl);

			// Vertical fill bar inside box
			const barX  = boxX + 12;
			const barW  = 26;
			const barY  = cY + 11;
			const barH  = 22;
			c.setColorValue(Color.Low);
			c.rect(barX, barY, barW, barH);
			const fillH = Math.round(STCH_V2_LOOP_CHANCE * (barH - 2));
			if (fillH > 0) {
				c.setColorValue(Color.Bright);
				c.fillRect(barX + 1, barY + barH - 1 - fillH, barW - 2, fillH);
			}

			// Percentage label below bar
			const pct = Math.round(STCH_V2_LOOP_CHANCE * 100) + '%';
			c.setColorValue(Color.Bright);
			c.drawText(boxX + Math.floor((boxW - c.textWidth(pct)) / 2), cY + 38, pct);

			// ── Step grid (right) ────────────────────────────────────────────
			// 8 columns × 2 rows; step buttons 1–16 map 1:1 to cells
			const gridX  = 56;
			const colW   = 25;
			const rowH   = 20;
			const rowGap =  4;
			const row0Y  = cY + 1;
			const row1Y  = row0Y + rowH + rowGap;

			for (let i = 0; i < 16; i++) {
				const col    = i % 8;
				const row    = Math.floor(i / 8);
				const x      = gridX + col * colW;
				const y      = row === 0 ? row0Y : row1Y;
				const inLoop = i < STCH_V2_LOOP_LEN;
				const gate   = STCH_V2_LOOP_GATES[i];

				if (!inLoop) continue;

				if (gate) {
					c.setColorValue(Color.Bright);
					c.fillRect(x + 1, y + 1, colW - 2, rowH - 2);
				} else {
					// Rest: dim outline only — step is in loop but silent
					c.setColorValue(Color.Low);
					c.rect(x + 1, y + 1, colW - 2, rowH - 2);
				}
			}
		}

		fwDrawFooter(c, STCH_V2_TABS, tab);
	}

	function ledsStochasticV2(): LedState {
		const s = defaultLeds();
		s.play     = 'green';
		s.track[3] = 'green';
		s.fkey[0]  = 'green';
		for (let i = 0; i < 7; i++) {
			s.step[i] = i === STCH_V2_PLAY   ? 'amber'
			          : i === STCH_V2_CURSOR  ? 'amber'
			          : STCH_V2_NPROB[i] > 0  ? 'green'
			          :                          'off';
		}
		return s;
	}

	// ── Arp Track — TRUNK (firmware port) ─────────────────────────────────────
	// 12 steps at 16px each, centered: x = i*16 + 32, leaving 32px margins each side.
	// GATE tab shows note name below box (Short1). Sidebar at x=224 shows arp engine params.

	const ARP_TRUNK_TABS   = ['GATE', 'RETRIG', 'LEN', 'NOTE', 'COND'] as const;
	const ARP_TRUNK_CURSOR = 2;
	const ARP_TRUNK_PLAY   = 5;
	const ARP_TRUNK_NSTEPS = 12;
	const ARP_TRUNK_SW     = 16;
	const ARP_TRUNK_XOFF   = ((16 - 12) / 2) * 16;  // = 32

	const ARP_TRUNK_GATES  = [1,0,1,1,0,1,0,1,0,1,1,0];
	const ARP_TRUNK_RETRIG = [0,0,0,1,0,0,0,0,0,2,0,0];  // retrigger count 0–7
	const ARP_TRUNK_LEN    = [8,0,6,4,0,10,0,8,0,6,12,0]; // length 0–15
	const ARP_TRUNK_NOTES  = [0,0,4,7,0,12,0,4,0,7,0,0];  // scale degree indices

	const ARP_TRUNK_DIV   = '1/16';
	const ARP_TRUNK_MODE  = 'UP';
	const ARP_TRUNK_OCT   = '2';
	const ARP_TRUNK_SCALE = 'CHROM';
	const ARP_TRUNK_ROOT  = 'C';

	function drawArpTrunk(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);

		const SW   = ARP_TRUNK_SW;
		const XOFF = ARP_TRUNK_XOFF;
		const y    = 20;

		fwDrawHeader(c, ARP_TRUNK_TABS[tab]);

		for (let i = 0; i < ARP_TRUNK_NSTEPS; i++) {
			const x        = i * SW + XOFF;
			const on       = Boolean(ARP_TRUNK_GATES[i]);
			const isCursor = i === ARP_TRUNK_CURSOR;
			const isPlay   = i === ARP_TRUNK_PLAY;

			// Step number
			const numStr = String(i + 1);
			c.setColorValue(isCursor ? Color.Bright : Color.Medium);
			c.drawText(x + (SW - c.textWidth(numStr) + 1) / 2, y - 2, numStr);

			// Gate box 12×12
			c.setColorValue(isPlay ? Color.Bright : Color.Medium);
			c.rect(x + 2, y + 2, SW - 4, SW - 4);
			if (on) {
				c.setColorValue(Color.Bright);
				c.fillRect(x + 4, y + 4, SW - 8, SW - 8);
			}

			switch (tab) {
			case 0: {  // GATE — note name (Short1) at y+27
				if (on) {
					const letter = NOTE_NAMES_12[((ARP_TRUNK_NOTES[i] % 12) + 12) % 12];
					c.setColorValue(Color.Bright);
					c.drawText(x + (SW - c.textWidth(letter) + 1) / 2, y + 27, letter);
				}
				break;
			}
			case 1:  // RETRIG — retrigger pulse marks
				fwDrawRetrigger(c, x, y + 18, SW, 2, ARP_TRUNK_RETRIG[i] + 1, 8);
				break;
			case 2:  // LEN — gate-length bracket
				if (on) fwDrawLength(c, x + 2, y + 18, SW - 4, 6, ARP_TRUNK_LEN[i] + 1, 16);
				break;
			case 3: {  // NOTE — two-line: letter + octave
				if (on) {
					const deg    = ARP_TRUNK_NOTES[i];
					const letter = NOTE_NAMES_12[((deg % 12) + 12) % 12];
					const oct    = String(Math.floor(deg / 12) + 4);
					c.setColorValue(Color.Bright);
					c.drawText(x + (SW - c.textWidth(letter) + 1) / 2, y + 20, letter);
					c.drawText(x + (SW - c.textWidth(oct) + 1) / 2,    y + 27, oct);
				}
				break;
			}
			case 4: {  // COND — placeholder dash
				if (on) {
					c.setColorValue(Color.Bright);
					c.drawText(x + (SW - c.textWidth('—') + 1) / 2, y + 20, '—');
				}
				break;
			}
			}
		}

		// Arp engine sidebar at x = 12*SW + XOFF = 224
		const sx = 12 * SW + XOFF;
		c.setColorValue(Color.Bright);
		c.drawText(sx + 2, y - 2,      ARP_TRUNK_DIV);
		c.drawText(sx + 2, y - 2 + 8,  ARP_TRUNK_MODE);
		c.drawText(sx + 2, y - 2 + 16, ARP_TRUNK_OCT);
		c.drawText(sx + 2, y - 2 + 24, ARP_TRUNK_SCALE);
		c.drawText(sx + 2, y - 2 + 32, ARP_TRUNK_ROOT);

		fwDrawFooter(c, [...ARP_TRUNK_TABS], tab);
	}

	function ledsArpTrunk(): LedState {
		const s = defaultLeds();
		s.play = 'green';
		s.track[4] = 'green';
		for (let i = 0; i < ARP_TRUNK_NSTEPS; i++) {
			s.step[i] = i === ARP_TRUNK_CURSOR ? 'amber' : (ARP_TRUNK_GATES[i] ? 'green' : 'off');
		}
		s.fkey[0] = 'green';
		return s;
	}

	// ── Arp Track — V2 (note pool + euclidean rhythm + accent) ───────────────

	function euclideanRhythm(n: number, k: number, r: number): boolean[] {
		if (k <= 0) return new Array(n).fill(false);
		if (k >= n) return new Array(n).fill(true);
		const out: boolean[] = [];
		let bucket = 0;
		for (let i = 0; i < n; i++) {
			bucket += k;
			if (bucket >= n) { bucket -= n; out.push(true); }
			else { out.push(false); }
		}
		const rot = ((r % n) + n) % n;
		return [...out.slice(rot), ...out.slice(0, rot)];
	}

	const ARP_V2_TABS = ['NOTE', 'RHYTHM', 'MOD', 'ARP'] as const;

	// NOTE tab — scale degrees 1–7, on/off (no note names — just numbered 1–7)
	const ARP_V2_DEGREES  = [true, false, true, false, true, true, false];
	const ARP_V2_PLAY_DEG = 0;  // degree 1 currently playing

	// RHYTHM tab — euclidean parameters
	// Step buttons 1–4: hold + encoder to edit STEPS / GATES / ROTATE / GATE LEN
	const ARP_V2_N           = 16;  // STEPS: total euclidean length
	const ARP_V2_K           = 5;   // GATES: number of hits
	const ARP_V2_R           = 0;   // ROTATE: rotation offset
	const ARP_V2_GATE_LEN_PCT = 40; // GATE LEN: percentage (5–100, step 5)
	const ARP_V2_PARAM       = 1;   // 0=STEPS, 1=GATES, 2=ROTATE, 3=GATE LEN (currently editing)
	const ARP_V2_RPLAY       = 3;   // current play position in pattern

	// MOD tab — secondary independent euclidean with a mode that shapes its interaction
	// Step buttons 1–4: hold + encoder to edit STEPS / PULSES / ROTATE / MODE
	// Modes: 0=OFF, 1=ACCENT, 2=MASK, 3=COMBINE, 4=RATCHET, 5=HOLD
	const ARP_V2_MOD_N          = 12;  // STEPS: mod pattern length (independent from rhythm)
	const ARP_V2_MOD_K          = 3;   // PULSES: mod hits within its own N steps
	const ARP_V2_MOD_R          = 0;   // ROTATE: mod rotation offset
	const ARP_V2_MOD_MODE_NAMES = ['OFF', 'ACCENT', 'MASK', 'COMBINE', 'RATCHET', 'HOLD'];
	const ARP_V2_MOD_MODE       = 1;   // ACCENT by default
	const ARP_V2_MOD_PARAM      = 3;   // 0=STEPS, 1=PULSES, 2=ROTATE, 3=MODE (currently editing)

	// ARP tab — play order + octave span + arp reset length
	// Step buttons 1–3: hold + encoder to edit ORDER / OCTAVES / LENGTH
	// Page+Step9=ORDER, Page+Step10=OCTAVES, Page+Step11=LENGTH, Page+Step12=DIVISOR (quick access)
	const ARP_V2_ORDER_NAMES = ['UP', 'DOWN', 'UPDN', 'DNUP', 'CONV', 'DIV', 'RAND'];
	const ARP_V2_ORDER       = 0;   // index into ARP_V2_ORDER_NAMES
	const ARP_V2_OCTAVES     = 2;   // 1–4
	const ARP_V2_LENGTH      = 16;  // arp reset length: after N steps, note pool + euclid hard-reset
	const ARP_V2_ARP_PARAM   = 0;   // 0=ORDER, 1=OCTAVES, 2=LENGTH (currently editing)

	function drawArpV2(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);

		// ── shared 2-row euclidean display ──────────────────────────────────────
		// Row 1 (rhythm): fill state + optional border encode the mode effect.
		//   Border = "mod pattern affects this step."
		//   Empty interior = "step is silenced."  Fill = "step fires."
		// Row 2 (mod): plain fill only — informational, no borders.
		// Play cursor tick below row 2.
		function drawCombinedEuclidean(playStep: number, mode: number): void {
			const rhythmPat = euclideanRhythm(ARP_V2_N, ARP_V2_K, ARP_V2_R);
			const modPat    = euclideanRhythm(ARP_V2_MOD_N, ARP_V2_MOD_K, ARP_V2_MOD_R);
			const cw = 16;
			const rh = 13;  // row height
			const r1 = 8;   // row 1 y (rhythm)
			const r2 = 23;  // row 2 y (mod)

			// Dim separator between rows
			c.setColorValue(Color.Low);
			c.hline(0, r2 - 1, 256);

			for (let i = 0; i < 16; i++) {
				const x       = i * cw;
				const hit     = rhythmPat[i % ARP_V2_N];
				const mod     = modPat[i % ARP_V2_MOD_N];
				const overlap = hit && mod;

				// Row 2: mod pattern — plain fill, no decoration
				if (mod) {
					c.setColorValue(Color.Low);
					c.fillRect(x + 2, r2, 12, rh);
				}

				// Row 1: rhythm with mode-encoded border/fill
				if (!hit) continue;

				let border  = false;
				let fires   = true;
				let ratchet = false;

				switch (mode) {
				case 0: // OFF — normal, no border
					break;
				case 1: // ACCENT — overlap fires with longer gate: border + fill
					border = overlap; break;
				case 2: // MASK — overlap muted: border + empty
					border = overlap; fires = !overlap; break;
				case 3: // COMBINE — rhythm-only muted: border+empty; overlap plays normally
					border = !overlap; fires = overlap; break;
				case 4: // RATCHET — overlap fires twice: border + double-bar
					border = overlap; ratchet = overlap; break;
				case 5: // HOLD — overlap legato: border + fill
					border = overlap; break;
				}

				c.setColorValue(Color.MediumBright);
				if (border) {
					c.rect(x + 2, r1, 12, rh);   // border same color as fill
				}
				if (fires) {
					if (ratchet) {
						// Two trigger bars with 1px border + 1px gap inset (2px total)
						c.fillRect(x + 4, r1 + 2, 3, rh - 4);   // left trigger bar
						c.fillRect(x + 9, r1 + 2, 3, rh - 4);   // right trigger bar
					} else if (border) {
						c.fillRect(x + 4, r1 + 2, 8, rh - 4);   // inner fill (1px border + 1px gap)
					} else {
						c.fillRect(x + 2, r1, 12, rh);           // full fill, no border
					}
				}
				// !fires + border = outline only (muted step)
			}

			// Play cursor: bright tick below row 2
			if (playStep >= 0) {
				const px = playStep * cw;
				c.setColorValue(Color.Bright);
				c.fillRect(px + 5, r2 + rh + 1, cw - 10, 2);
			}
		}

		// ── shared param-bar helper ──────────────────────────────────────────────
		// Draws a row of equal-width cells at y=38..53. Each cell: label + value.
		// Selected cell (selIdx) = bright fill, inverted text. Others = dim text.
		function drawParamBar(cells: { label: string; value: string }[], selIdx: number): void {
			const barY = 38;
			const barH = 16;
			const cellW = Math.floor(256 / cells.length);
			c.setColorValue(Color.Low);
			c.hline(0, barY - 1, 256);   // separator above bar
			for (let p = 0; p < cells.length; p++) {
				const bx  = p * cellW;
				const sel = p === selIdx;
				// Cell background
				if (sel) {
					c.setColorValue(Color.Bright);
					c.fillRect(bx, barY, cellW, barH);
				}
				// Left divider (skip first)
				if (p > 0) {
					c.setColorValue(sel ? Color.Bright : Color.Low);
					c.vline(bx, barY, barH);
				}
				// Label (top line of cell, y=42)
				const lbl = cells[p].label;
				c.setColorValue(sel ? Color.None : Color.Low);
				c.drawText(bx + Math.floor((cellW - c.textWidth(lbl)) / 2), barY + 5, lbl);
				// Value (bottom line of cell, y=50)
				const val = cells[p].value;
				c.setColorValue(sel ? Color.None : Color.Bright);
				c.drawText(bx + Math.floor((cellW - c.textWidth(val)) / 2), barY + 13, val);
			}
		}

		if (tab === 0) {
			// ── NOTE: 7 on/off degree boxes ────────────────────────────────────
			const SW   = 36;
			const XOFF = 2;
			const boxS = 22;   // square size
			const boxY = 14;   // top of box, centered vertically in available space
			const boxX = (SW - boxS) / 2;  // horizontal offset within column (7px)

			for (let i = 0; i < 7; i++) {
				const x    = XOFF + i * SW;
				const on   = ARP_V2_DEGREES[i];
				const play = i === ARP_V2_PLAY_DEG;
				const bx   = x + boxX;

				// Square box: filled = active, outline = inactive
				if (on) {
					c.setColorValue(Color.Bright);
					c.fillRect(bx, boxY, boxS, boxS);
				} else {
					c.setColorValue(Color.Low);
					c.rect(bx, boxY, boxS, boxS);
				}

				// Play indicator: small bright tick below box
				if (play) {
					c.setColorValue(Color.Bright);
					c.fillRect(x + Math.floor(SW / 2) - 2, boxY + boxS + 3, 4, 2);
				}

				// Degree label 1–7
				const lbl = String(i + 1);
				c.setColorValue(Color.MediumLow);
				c.drawText(x + Math.floor((SW - c.textWidth(lbl)) / 2), 53, lbl);
			}

		} else if (tab === 1) {
			// ── RHYTHM: 3-row combined view + 4-cell param bar ─────────────────
			drawCombinedEuclidean(ARP_V2_RPLAY, ARP_V2_MOD_MODE);
			drawParamBar([
				{ label: 'STEPS',    value: String(ARP_V2_N) },
				{ label: 'GATES',    value: String(ARP_V2_K) },
				{ label: 'ROTATE',   value: String(ARP_V2_R) },
				{ label: 'GATE LEN', value: `${ARP_V2_GATE_LEN_PCT}%` },
			], ARP_V2_PARAM);

		} else if (tab === 2) {
			// ── MOD: 3-row combined view + 4-cell param bar (mode as last cell) ─
			drawCombinedEuclidean(ARP_V2_RPLAY, ARP_V2_MOD_MODE);
			drawParamBar([
				{ label: 'STEPS',  value: String(ARP_V2_MOD_N) },
				{ label: 'PULSES', value: String(ARP_V2_MOD_K) },
				{ label: 'ROTATE', value: String(ARP_V2_MOD_R) },
				{ label: 'MODE',   value: ARP_V2_MOD_MODE_NAMES[ARP_V2_MOD_MODE] ?? 'OFF' },
			], ARP_V2_MOD_PARAM);

		} else if (tab === 3) {
			// ── ARP: ORDER | OCTAVES | LENGTH — 3 columns, no redundant param bar
			const modeName = ARP_V2_ORDER_NAMES[ARP_V2_ORDER] ?? 'UP';
			const colW = Math.floor(256 / 3);  // 85px

			// Column dividers
			c.setColorValue(Color.Low);
			c.vline(colW,     8, 46);
			c.vline(colW * 2, 8, 46);

			// ── Column 0: PLAY ORDER ─────────────────────────────────────────────
			{
				const cx = Math.floor(colW / 2);
				const sel = ARP_V2_ARP_PARAM === 0;
				c.setFont(Font.Tiny);
				c.setColorValue(Color.Low);
				c.drawText(cx - Math.floor(c.textWidth('PLAY ORDER') / 2), 16, 'PLAY ORDER');
				c.setFont(Font.Normal);
				c.setColorValue(sel ? Color.Bright : Color.Medium);
				c.drawText(cx - Math.floor(c.textWidth(modeName) / 2), 36, modeName);
				c.setFont(Font.Tiny);
			}

			// ── Column 1: OCTAVES ────────────────────────────────────────────────
			{
				const cx = colW + Math.floor(colW / 2);
				const sel = ARP_V2_ARP_PARAM === 1;
				c.setFont(Font.Tiny);
				c.setColorValue(Color.Low);
				c.drawText(cx - Math.floor(c.textWidth('OCTAVES') / 2), 16, 'OCTAVES');
				// 4-segment bar
				const segW = 10;
				const segH = 10;
				const totalW = 4 * segW + 3 * 2;
				const sx = cx - Math.floor(totalW / 2);
				for (let o = 0; o < 4; o++) {
					c.setColorValue(o < ARP_V2_OCTAVES
						? (sel ? Color.Bright : Color.MediumBright)
						: Color.Low);
					c.fillRect(sx + o * (segW + 2), 22, segW, segH);
				}
				const octLbl = `${ARP_V2_OCTAVES} oct`;
				c.setColorValue(sel ? Color.Bright : Color.Medium);
				c.drawText(cx - Math.floor(c.textWidth(octLbl) / 2), 40, octLbl);
			}

			// ── Column 2: LENGTH ─────────────────────────────────────────────────
			{
				const cx = colW * 2 + Math.floor(colW / 2);
				const sel = ARP_V2_ARP_PARAM === 2;
				c.setFont(Font.Tiny);
				c.setColorValue(Color.Low);
				c.drawText(cx - Math.floor(c.textWidth('LENGTH') / 2), 16, 'LENGTH');
				c.setFont(Font.Normal);
				c.setColorValue(sel ? Color.Bright : Color.Medium);
				c.drawText(cx - Math.floor(c.textWidth(String(ARP_V2_LENGTH)) / 2), 36, String(ARP_V2_LENGTH));
			}
		}

		fwDrawFooter(c, [...ARP_V2_TABS], tab);
	}

	function ledsArpV2(): LedState {
		const s = defaultLeds();
		s.play = 'green';
		s.track[4] = 'green';
		// Step buttons 1–4 lit green = hold-and-edit affordance for RHYTHM tab params
		s.step[0] = 'green';  // STEPS
		s.step[1] = 'amber';  // GATES (currently selected)
		s.step[2] = 'green';  // ROTATE
		s.step[3] = 'green';  // GATE LEN
		// Euclidean rhythm shown on remaining step LEDs 5–16
		const rhythm = euclideanRhythm(ARP_V2_N, ARP_V2_K, ARP_V2_R);
		for (let i = 4; i < 16; i++) {
			s.step[i] = i === ARP_V2_RPLAY ? 'amber'
			           : (i < ARP_V2_N && rhythm[i]) ? 'green'
			           : 'off';
		}
		s.fkey[1] = 'green';   // RHYTHM tab
		return s;
	}

	// ── Quantizer Track — TRUNK (firmware port) ─────────────────────────────────
	// Faithful port of QuantizerSequenceEditPage.cpp:
	// single GATE layer, no F-key tabs in firmware.

	const QUANT_TRUNK_FIRST  = 0;
	const QUANT_TRUNK_LAST   = 15;
	const QUANT_TRUNK_CURSOR = 3;
	const QUANT_TRUNK_PLAY   = 7;
	const QUANT_TRUNK_GATES  = [1,0,1,1,0,1,0,1,0,1,0,0,1,0,0,1];

	function drawQuantizerTrunk(c: Canvas): void {
		c.setFont(Font.Tiny);
		fwDrawHeader(c, 'GATE');

		const SW    = 16;
		const loopY = 16;
		const y     = 20;

		c.setColorValue(Color.Bright);
		const lsx = QUANT_TRUNK_FIRST * SW + 1;
		c.vline(lsx, loopY - 1, 3);
		c.point(lsx + 1, loopY);
		const lex = QUANT_TRUNK_LAST * SW + SW - 2;
		c.vline(lex, loopY - 1, 3);
		c.point(lex - 1, loopY);
		for (let i = 1; i <= QUANT_TRUNK_LAST; i++) c.point(i * SW, loopY);

		for (let i = 0; i < 16; i++) {
			const x        = i * SW;
			const on       = Boolean(QUANT_TRUNK_GATES[i]);
			const isCursor = i === QUANT_TRUNK_CURSOR;
			const isPlay   = i === QUANT_TRUNK_PLAY;

			const numStr = String(i + 1);
			c.setColorValue(isCursor ? Color.Bright : Color.Medium);
			c.drawText(x + Math.floor((SW - c.textWidth(numStr) + 1) / 2), y - 2, numStr);

			c.setColorValue(isPlay ? Color.Bright : Color.Medium);
			c.rect(x + 2, y + 2, 12, 12);
			if (on) {
				c.setColorValue(Color.Bright);
				c.fillRect(x + 4, y + 4, 8, 8);
			}
		}

		fwDrawFooter(c, ['', '', '', '', ''], 0);
	}

	function ledsQuantizerTrunk(): LedState {
		const s = defaultLeds();
		s.play = 'green';
		s.track[3] = 'green';
		QUANT_TRUNK_GATES.forEach((on, i) => {
			s.step[i] = i === QUANT_TRUNK_CURSOR ? 'amber' : (on ? 'green' : 'off');
		});
		return s;
	}

	// ── Quantizer Track — V1 (GATE | SRCE | TRIG | SCLE) ─────────────────────────
	// Only one step layer (Gate). Tabs F1–F4 surface the most-changed config
	// as in-place selectors rather than sending the user to Track Config.
	// GATE tab: gate pattern + live CV output readout.
	// SRCE tab: input source selector (CV1–CV4).
	// TRIG tab: trigger mode (FREE / INT / EXT) + trigger track when EXT.
	// SCLE tab: scale + root note (two-column scrollable list).

	const QUANT_V1_TABS        = ['GATE', 'SRCE', 'TRIG', 'TUNE', ''] as const;
	const QUANT_V1_GATES       = [1,0,1,1,0,1,0,1,0,1,0,0,1,0,0,1];
	const QUANT_V1_CURSOR      = 3;
	const QUANT_V1_PLAY        = 7;
	const QUANT_V1_FIRST       = 0;
	const QUANT_V1_LAST        = 15;
	const QUANT_V1_SOURCE      = 6;   // 0–3=CvIn1–4, 4–11=Track1–8 (Track3 = index 6 selected)
	const QUANT_V1_TRIG: number = 2;  // 0=FREE, 1=INT, 2=EXT
	const QUANT_V1_TRIG_TRK    = 1;   // trigger track index when INT (T2)
	const QUANT_V1_TRIG_CV     = 0;   // trigger CV input index when EXT (CV1)
	const QUANT_V1_OCTAVE      = 0;   // -10..+10
	const QUANT_V1_TRANSPOSE   = 7;   // -100..+100 (non-zero to show bar in demo)
	const QUANT_V1_TUNE_PARAM: number = 1;  // 0=OCTAVE, 1=TRANSPOSE

	const QUANT_TRIG_NAMES = ['FREE', 'INT', 'EXT'];

	function drawQuantizerV1(c: Canvas, tab = 0): void {
		c.setFont(Font.Tiny);
		fwDrawHeader(c, QUANT_V1_TABS[tab]);

		const SW    = 16;
		const loopY = 16;
		const y     = 20;

		if (tab === 0) {
			// ── GATE: step grid + live CV output readout ──────────────────────────
			c.setColorValue(Color.Bright);
			const lsx = QUANT_V1_FIRST * SW + 1;
			c.vline(lsx, loopY - 1, 3);
			c.point(lsx + 1, loopY);
			const lex = QUANT_V1_LAST * SW + SW - 2;
			c.vline(lex, loopY - 1, 3);
			c.point(lex - 1, loopY);
			for (let i = 1; i <= QUANT_V1_LAST; i++) {
				c.setColorValue(Color.Low);
				c.point(i * SW, loopY);
			}

			for (let i = 0; i < 16; i++) {
				const x        = i * SW;
				const on       = Boolean(QUANT_V1_GATES[i]);
				const isCursor = i === QUANT_V1_CURSOR;
				const isPlay   = i === QUANT_V1_PLAY;

				const numStr = String(i + 1);
				c.setColorValue(isCursor ? Color.Bright : Color.Low);
				c.drawText(x + Math.floor((SW - c.textWidth(numStr) + 1) / 2), y - 2, numStr);

				c.setColorValue(isPlay ? Color.Bright : Color.Low);
				c.rect(x + 2, y + 2, 12, 12);
				if (on) {
					c.setColorValue(Color.Bright);
					c.fillRect(x + 4, y + 4, 8, 8);
				}
			}

			// Live output readout (lower-right)
			c.setFont(Font.Tiny);
			c.setColorValue(Color.Low);
			c.drawText(188, 48, 'OUT');
			c.setFont(Font.Normal);
			c.setColorValue(Color.Bright);
			c.drawText(210, 51, 'F4');
			c.setFont(Font.Tiny);

		} else if (tab === 1) {
			// ── SRCE: 12 sources — CV IN row (top) + TRACK row (bottom) ──────────

			// CV IN section label
			c.setColorValue(Color.Low);
			c.drawText(4, 16, 'CV IN');

			// Top row: CvIn1–4 (indices 0–3), 4 boxes of 64px each
			const cvW = 64;
			const cvLabels = ['CV 1', 'CV 2', 'CV 3', 'CV 4'];
			for (let i = 0; i < 4; i++) {
				const bx  = i * cvW + 6;
				const bw  = cvW - 12;
				const sel = QUANT_V1_SOURCE === i;
				if (sel) {
					c.setColorValue(Color.Bright);
					c.fillRect(bx, 19, bw, 16);
					c.setColorValue(Color.None);
				} else {
					c.setColorValue(Color.Low);
					c.rect(bx, 19, bw, 16);
					c.setColorValue(Color.Medium);
				}
				c.setFont(Font.Normal);
				c.drawText(i * cvW + Math.floor((cvW - c.textWidth(cvLabels[i])) / 2), 29, cvLabels[i]);
				c.setFont(Font.Tiny);
			}

			// TRACK section label
			c.setColorValue(Color.Low);
			c.drawText(4, 41, 'TRACK');

			// Bottom row: Track1–8 (indices 4–11), 8 boxes centered
			const trkW  = 28;
			const trkGap = 2;
			const trkX0 = Math.floor((256 - (8 * trkW + 7 * trkGap)) / 2);
			for (let t = 0; t < 8; t++) {
				const tx  = trkX0 + t * (trkW + trkGap);
				const sel = QUANT_V1_SOURCE === t + 4;
				const lbl = `T${t + 1}`;
				const ltx = tx + Math.floor((trkW - c.textWidth(lbl)) / 2);
				if (sel) {
					c.setColorValue(Color.Bright);
					c.fillRect(tx, 43, trkW, 10);
					c.setColorValue(Color.None);
					c.drawText(ltx, 50, lbl);
				} else {
					c.setColorValue(Color.Low);
					c.rect(tx, 43, trkW, 10);
					c.setColorValue(Color.Medium);
					c.drawText(ltx, 50, lbl);
				}
			}

		} else if (tab === 2) {
			// ── TRIG: trigger mode + conditional sub-selector ─────────────────────
			c.setColorValue(Color.Low);
			c.drawText(Math.floor((256 - c.textWidth('TRIGGER MODE')) / 2), 15, 'TRIGGER MODE');

			const modeW = Math.floor(256 / 3);
			for (let i = 0; i < 3; i++) {
				const bx  = i * modeW + 4;
				const bw  = modeW - 8;
				const sel = i === QUANT_V1_TRIG;
				if (sel) {
					c.setColorValue(Color.Bright);
					c.fillRect(bx, 19, bw, 14);
					c.setColorValue(Color.None);
				} else {
					c.setColorValue(Color.Low);
					c.rect(bx, 19, bw, 14);
					c.setColorValue(Color.Medium);
				}
				c.drawText(i * modeW + Math.floor((modeW - c.textWidth(QUANT_TRIG_NAMES[i])) / 2), 29, QUANT_TRIG_NAMES[i]);
			}

			if (QUANT_V1_TRIG === 1) {
				// INT: show track selector
				c.setColorValue(Color.Low);
				c.drawText(4, 40, 'TRIGGER TRACK');
				const trkW  = 28;
				const trkGap = 2;
				const trkX0 = Math.floor((256 - (8 * trkW + 7 * trkGap)) / 2);
				for (let t = 0; t < 8; t++) {
					const tx  = trkX0 + t * (trkW + trkGap);
					const sel = t === QUANT_V1_TRIG_TRK;
					const lbl = `T${t + 1}`;
					const ltx = tx + Math.floor((trkW - c.textWidth(lbl)) / 2);
					if (sel) {
						c.setColorValue(Color.Bright);
						c.fillRect(tx, 43, trkW, 10);
						c.setColorValue(Color.None);
						c.drawText(ltx, 50, lbl);
					} else {
						c.setColorValue(Color.Low);
						c.rect(tx, 43, trkW, 10);
						c.setColorValue(Color.Medium);
						c.drawText(ltx, 50, lbl);
					}
				}
			} else if (QUANT_V1_TRIG === 2) {
				// EXT: show CV input selector
				c.setColorValue(Color.Low);
				c.drawText(4, 40, 'TRIGGER CV IN');
				const cvW = 64;
				const cvLabels = ['CV 1', 'CV 2', 'CV 3', 'CV 4'];
				for (let i = 0; i < 4; i++) {
					const bx  = i * cvW + 6;
					const bw  = cvW - 12;
					const sel = i === QUANT_V1_TRIG_CV;
					if (sel) {
						c.setColorValue(Color.Bright);
						c.fillRect(bx, 43, bw, 10);
						c.setColorValue(Color.None);
					} else {
						c.setColorValue(Color.Low);
						c.rect(bx, 43, bw, 10);
						c.setColorValue(Color.Medium);
					}
					c.drawText(i * cvW + Math.floor((cvW - c.textWidth(cvLabels[i])) / 2), 50, cvLabels[i]);
				}
			}

		} else if (tab === 3) {
			// ── TUNE: OCTAVE + TRANSPOSE, bipolar bar + value ─────────────────────
			// Follows the Arp V2 ARP-tab column pattern: dim label → bar → value

			c.setColorValue(Color.Low);
			c.vline(128, 10, 43);

			function drawTuneColumn(
				cx: number, label: string,
				value: number, maxVal: number, sel: boolean
			): void {
				const barW   = 80;
				const barH   = 5;
				const barY   = 26;
				const barX   = cx - Math.floor(barW / 2);
				const midX   = cx;

				// Label
				c.setFont(Font.Tiny);
				c.setColorValue(Color.Low);
				c.drawText(cx - Math.floor(c.textWidth(label) / 2), 16, label);

				// Bar background
				c.setColorValue(Color.Low);
				c.fillRect(barX, barY, barW, barH);

				// Filled section from center toward value
				const halfW = Math.floor(barW / 2);
				const fillPx = Math.round(halfW * Math.abs(value) / maxVal);
				if (fillPx > 0) {
					c.setColorValue(sel ? Color.Bright : Color.Medium);
					if (value >= 0) {
						c.fillRect(midX, barY, fillPx, barH);
					} else {
						c.fillRect(midX - fillPx, barY, fillPx, barH);
					}
				}

				// Zero notch: dark tick at center to make 0 legible
				c.setColorValue(Color.None);
				c.vline(midX, barY, barH);

				// Numeric value
				const valStr = value > 0 ? `+${value}` : String(value);
				c.setFont(Font.Normal);
				c.setColorValue(sel ? Color.Bright : Color.Medium);
				c.drawText(cx - Math.floor(c.textWidth(valStr) / 2), 44, valStr);
				c.setFont(Font.Tiny);
			}

			drawTuneColumn(64,  'OCTAVE',    QUANT_V1_OCTAVE,    10,  QUANT_V1_TUNE_PARAM === 0);
			drawTuneColumn(192, 'TRANSPOSE', QUANT_V1_TRANSPOSE, 100, QUANT_V1_TUNE_PARAM === 1);
		}

		fwDrawFooter(c, [...QUANT_V1_TABS], tab);
	}

	function ledsQuantizerV1(): LedState {
		const s = defaultLeds();
		s.play = 'green';
		s.track[3] = 'green';
		QUANT_V1_GATES.forEach((on, i) => {
			s.step[i] = i === QUANT_V1_CURSOR ? 'amber' : (on ? 'green' : 'off');
		});
		s.fkey[0] = 'green';
		return s;
	}

	// ── routing ───────────────────────────────────────────────────────────────

	type PageId =
		| 'dashboard' | 'perform' | 'trackedit' | 'curvetrack' | 'stochastic' | 'arp' | 'quantizer'
		| 'trackconfig' | 'song' | 'settings' | 'tempo' | 'quickedit';

	interface Variation {
		label: string;
		draw: (c: Canvas, tab?: number) => void;
		tabs?: string[];        // if set, F1–F5 switch tabs; length ≤ 5
		leds?: () => LedState;  // optional — falls back to all-off if absent
	}

	const VARIATIONS: Record<PageId, Variation[]> = {
		dashboard: [
			{ label: 'Option 1 — inverted focus row, gate dots', draw: drawDash_trunk, leds: ledsDash_trunk },
			{ label: 'Option 2 — focus bars + dot strip (TE)', draw: drawDash_TE2, leds: ledsDash_TE2 },
			{ label: 'Option 3 — phase bars, type glyph (Hapax)', draw: drawDash_Hapax4, leds: ledsDash_Hapax4 },
		],
		perform: [
			{ label: 'Option 1 — 8 rows, 3-level, footer', draw: drawPerf_trunk, leds: ledsPerf_trunk },
			{ label: 'Option 2 — brightness only, no labels (Norns)', draw: drawPerf_Norns3, leds: ledsPerf_Norns3 },
			{ label: 'Option 3 — pat# + phase bar (Hapax)', draw: drawPerf_Hapax4, leds: ledsPerf_Hapax4 },
		],
		trackedit: [
			{ label: '★ TRUNK — current firmware UI (GATE|RETRIG|LEN|NOTE|COND)', draw: drawFirmwareNote, tabs: FW_TABS, leds: ledsFirmwareNote },
			{ label: 'V2 — Firmware Plus: piano-roll pitch strip + beat markers', draw: drawNoteV2, tabs: FW_TABS, leds: ledsNoteV2 },
			{ label: 'V3 — 2×8 grid: physical layout, rich cells, all 5 tabs', draw: drawNoteV3, tabs: FW_TABS, leds: ledsNoteV3 },
			{ label: 'V1 — note lines, no header, tab values', draw: drawNewNoteV1, tabs: FW_TABS, leds: ledsNewNoteV1 },
		],
		curvetrack: [
			{ label: '★ TRUNK — current firmware UI (SHPE|MIN|MAX|GATE)', draw: drawCurveTrunk, tabs: CURVE_TABS, leds: ledsCurveTrunk },
			{ label: 'V1 — segment assembler (SKEW|LEN|SHPE|LVL|OFST)', draw: drawCurveV1, tabs: SEG_TABS, leds: ledsCurveV1 },
		],
		stochastic: [
			{ label: '★ TRUNK — current firmware UI (GATE|RETRIG|LEN|NOTE|COND)', draw: drawStochasticTrunk, tabs: STCH_TRUNK_TABS, leds: ledsStochasticTrunk },
			{ label: 'V1 — probability fill cells (GATE|LEN|NOTE|PROB)', draw: drawStochastic, tabs: STOCH_TABS, leds: ledsStochastic },
			{ label: 'V2 — SIG-inspired: 12 note sliders + piano keyboard (NOTE|OCT|LEN|ART|LOOP)', draw: drawStochasticV2, tabs: STCH_V2_TABS, leds: ledsStochasticV2 },
		],
		arp: [
			{ label: '★ TRUNK — current firmware UI (GATE|RETRIG|LEN|NOTE|COND)', draw: drawArpTrunk, tabs: [...ARP_TRUNK_TABS], leds: ledsArpTrunk },
			{ label: 'V2 — note pool + euclidean rhythm + accent (NOTE|RHYTHM|ACCENT|ARP)', draw: drawArpV2, tabs: [...ARP_V2_TABS], leds: ledsArpV2 },
		],
		quantizer: [
			{ label: '★ TRUNK — current firmware UI (GATE only, no tabs)', draw: drawQuantizerTrunk, leds: ledsQuantizerTrunk },
			{ label: 'V1 — gate grid + config tabs (GATE|SRCE|TRIG|SCLE)', draw: drawQuantizerV1, tabs: [...QUANT_V1_TABS], leds: ledsQuantizerV1 },
		],
		trackconfig: [
			{ label: 'Option 1 — flat list',             draw: drawCfg_A },
			{ label: 'Option 2 — grouped sections',      draw: drawCfg_B },
			{ label: 'Option 3 — tab sections',          draw: drawCfg_D },
		],
		song: [
			{ label: 'Option 1 — slot × track grid',     draw: drawSong_A },
			{ label: 'Option 2 — timeline blocks',       draw: drawSong_B },
			{ label: 'Option 3 — focused slot detail',   draw: drawSong_D },
		],
		settings: [
			{ label: 'Option 1 — two-panel',             draw: drawSet_A },
			{ label: 'Option 2 — flat list + dividers',  draw: drawSet_B },
			{ label: 'Option 3 — breadcrumb + big value',draw: drawSet_D },
		],
		tempo: [
			{ label: 'Option 1 — large centered BPM',    draw: drawTempo_A },
			{ label: 'Option 2 — BPM + swing',           draw: drawTempo_B },
			{ label: 'Option 3 — minimal, number only',  draw: drawTempo_C },
		],
		quickedit: [
			{ label: 'Option 1 — floating overlay',      draw: drawQE_A },
			{ label: 'Option 2 — bottom bar',            draw: drawQE_B },
			{ label: 'Option 3 — full-screen',           draw: drawQE_C },
		],
	};

	const PAGE_NAMES: Record<PageId, string> = {
		dashboard: 'Dashboard', perform: 'Perform', trackedit: 'Note Track Edit',
		curvetrack: 'Curve Track Edit', stochastic: 'Stochastic Track Edit', arp: 'Arp Track Edit',
		quantizer: 'Quantizer Track Edit',
		trackconfig: 'Track Config', song: 'Song', settings: 'Settings',
		tempo: 'Tempo', quickedit: 'Quick Edit',
	};

	const NAV: { label: string; target: PageId }[] = [
		{ label: 'PERF key',  target: 'dashboard' },
		{ label: 'PATT key',  target: 'perform' },
		{ label: 'PG+STP0',   target: 'trackedit' },
		{ label: 'PG+STP1',   target: 'curvetrack' },
		{ label: 'PG+STP2',   target: 'stochastic' },
		{ label: 'PG+STP3',   target: 'arp' },
		{ label: 'PG+STP4',   target: 'quantizer' },
		{ label: 'PG+STP5',   target: 'trackconfig' },
		{ label: 'PG+STP6',   target: 'song' },
		{ label: 'PG+TRK0',   target: 'settings' },
		{ label: 'TEMPO',     target: 'tempo' },
		{ label: 'PG+STP8',   target: 'quickedit' },
	];

	let currentId = $state<PageId>('trackedit');
	let selectedVar = $state(0);
	let varTabs = $state<number[]>(new Array(50).fill(0));

	const currentVars = $derived(VARIATIONS[currentId]);
	const liveVar = $derived(currentVars[Math.min(selectedVar, currentVars.length - 1)]);
	const liveDraw = $derived((c: Canvas) => liveVar.draw(c, varTabs[selectedVar]));
	const liveLeds = $derived((liveVar.leds ?? defaultLeds)());

	function navigateTo(id: PageId): void {
		currentId = id;
		selectedVar = 0;
		varTabs = new Array(50).fill(0);
		if (typeof window !== 'undefined') window.location.hash = id;
	}

	function selectVariation(i: number): void {
		selectedVar = i;
	}

	function pressF(t: number): void {
		if (liveVar.tabs && t < liveVar.tabs.length) varTabs[selectedVar] = t;
	}

	$effect(() => {
		if (typeof window === 'undefined') return;
		const hash = window.location.hash.slice(1) as PageId;
		if (hash && hash in VARIATIONS) { currentId = hash; selectedVar = 0; varTabs = new Array(50).fill(0); }
	});
</script>

<svelte:head><title>PER|FORMER — {PAGE_NAMES[currentId]}</title></svelte:head>

<div class="sandbox">

	<!-- ── LEFT: variation list ───────────────────────────────────────────── -->
	<aside class="variations">
		<div class="page-nav">
			{#each NAV as n}
				<button
					class="navbtn"
					class:active={currentId === n.target}
					onclick={() => navigateTo(n.target)}
				>{n.label}</button>
			{/each}
		</div>
		<h2>{PAGE_NAMES[currentId]}</h2>
		{#each currentVars as variation, i}
			<button
				class="specimen"
				class:selected={selectedVar === i}
				onclick={() => selectVariation(i)}
			>
				<span class="var-label">{variation.label}</span>
				<Display draw={(c) => variation.draw(c, varTabs[i])} scale={2} />
			</button>
		{/each}
	</aside>

	<!-- ── RIGHT: interactive dummy ──────────────────────────────────────── -->
	<main class="clickdummy">
		<Display draw={liveDraw} scale={3} />
		<LedPanel state={liveLeds} />
		<div class="hardware">
			<div class="btn-row">
				<span class="lbl">F</span>
				{#each Array(5) as _, i}
					<button
						class="fkey"
						class:active={liveVar.tabs && varTabs[selectedVar] === i}
						class:has-tabs={!!liveVar.tabs}
						onclick={() => pressF(i)}
					>F{i + 1}</button>
				{/each}
				<span style="width:.5rem"></span>
				<button class="gkey">PLAY</button>
				<button class="gkey">PAGE</button>
				<button class="gkey">SHIFT</button>
			</div>
		</div>
	</main>

</div>

<style>
	:global(body) { margin: 0; background: #0e0e0e; color: #bbb; font-family: monospace; }

	.sandbox {
		display: grid;
		grid-template-columns: 1fr 1fr;
		gap: 2rem;
		padding: 1.5rem;
		min-height: 100vh;
		box-sizing: border-box;
	}

	.page-nav {
		display: flex;
		gap: 4px;
		flex-wrap: wrap;
		margin-bottom: 1rem;
	}

	h2 {
		font-size: 0.75rem;
		text-transform: uppercase;
		letter-spacing: 0.15em;
		color: #555;
		margin: 0 0 0.75rem;
	}

	.variations {
		display: flex;
		flex-direction: column;
		gap: 0.75rem;
	}

	.specimen {
		display: inline-flex;
		flex-direction: column;
		gap: 0.3rem;
		background: none;
		border: 1px solid transparent;
		border-radius: 3px;
		padding: 0.4rem;
		cursor: pointer;
		text-align: left;
	}

	.specimen:hover { border-color: #2a2a2a; }
	.specimen.selected { border-color: #555; background: #141414; }

	.var-label {
		font-size: 0.6rem;
		color: #444;
		font-family: monospace;
	}

	.specimen.selected .var-label { color: #888; }

	.clickdummy {
		display: flex;
		flex-direction: column;
		gap: 0.75rem;
		position: sticky;
		top: 1.5rem;
		align-self: start;
	}

	.hardware { display: flex; flex-direction: column; gap: 4px; }

	.btn-row {
		display: flex;
		align-items: center;
		gap: 3px;
		flex-wrap: wrap;
	}

	.lbl { font-size: 0.6rem; color: #444; width: 3.2rem; flex-shrink: 0; }

	button {
		background: #1a1a1a; border: 1px solid #2a2a2a; color: #666;
		font-family: monospace; cursor: pointer; border-radius: 2px;
	}
	button:hover  { background: #252525; color: #bbb; }
	button:active { background: #333; }

	.navbtn        { font-size: 0.6rem; padding: 3px 5px; }
	.navbtn.active { border-color: #666; color: #ddd; background: #222; }
	.fkey          { font-size: 0.6rem; padding: 3px 5px; }
	.fkey.has-tabs { border-color: #3a3a3a; color: #888; }
	.fkey.active   { border-color: #aaa; color: #eee; background: #2a2a2a; }
	.gkey          { font-size: 0.6rem; padding: 3px 5px; }
</style>
