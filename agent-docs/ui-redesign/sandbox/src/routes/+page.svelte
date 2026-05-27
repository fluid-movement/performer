<script lang="ts">
	import Display from '$lib/canvas/Display.svelte';
	import LedPanel from '$lib/canvas/LedPanel.svelte';
	import { Canvas, Color, Font, HAlign, VAlign } from '$lib/canvas/Canvas.js';
	import { defaultLeds, type LedState } from '$lib/canvas/LedState.js';

	// ── fixture data ─────────────────────────────────────────────────────────

	const TRACKS = [
		{ num: 1, type: 'NOTE', pat: 1, mute: false, gates: [1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0], notes: [60,0,0,0,62,0,0,0,64,0,0,0,65,0,0,0] },
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

	// ── Note Track Edit — 3 numbered variations ─────────────────────────────
	// All three share the same footer tab set: GATE | NOTE | COND | VELO | LEN.
	// CURSOR=4 (step 5, gate on, D4). PLAY_CURSOR=4 coincides for this fixture.

	const NOTE_CURSOR = 4;

	// Option 1 — full-height gate cells + embedded pitch bar at bottom of each on-step
	function drawNote_1(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumLow);
		c.drawText(2, 6, 'TRK 1  NOTE  P01'); c.hline(0, 7, 256);

		for (let s = 0; s < 16; s++) {
			const x = 1 + s * 16;
			const on = Boolean(TRACKS[0].gates[s]);
			const isBeat = s % 4 === 0;
			const isCursor = s === NOTE_CURSOR;
			const noteVal = TRACKS[0].notes[s] || 0;
			// pitch bar: 2-10px, mapped MIDI 36-84; placed just above step label
			const barH = (on && noteVal) ? Math.max(2, Math.round(((noteVal - 36) / 48) * 10)) : 0;

			if (isCursor) {
				c.setColorValue(Color.Bright); c.fillRect(x, 9, 14, 44);
				// pitch bar as dark cutout inside bright cell
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

		c.setColorValue(Color.MediumLow); c.hline(0, 55, 256);
		const tabs = ['GATE', 'NOTE', 'COND', 'VELO', 'LEN'];
		for (let i = 0; i < 5; i++) {
			const w = i < 4 ? 51 : 52;
			if (i === 0) { c.setColorValue(Color.Bright); c.fillRect(i * 51, 56, w, 8); c.setColorValue(Color.None); }
			else { c.setColorValue(Color.MediumLow); }
			c.drawTextAligned(i * 51, 56, w, 8, HAlign.Center, VAlign.Center, tabs[i]);
		}
	}

	// Option 2 — compact gate strip + large dominant note value for cursor step
	function drawNote_2(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumLow);
		c.drawText(2, 6, 'TRK 1  NOTE  P01'); c.hline(0, 7, 256);

		// Compact gate row (8px tall)
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

		// Dominant: large note name for cursor step
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

		// Pitch position bar
		if (gateOn && noteVal) {
			const fraction = Math.min(1, (noteVal - 36) / 48);
			const barW = Math.round(fraction * 220);
			c.setColorValue(Color.MediumLow); c.rect(16, 42, 224, 4);
			if (barW > 1) { c.setColorValue(Color.Bright); c.fillRect(17, 43, barW - 1, 2); }
		}

		c.setColorValue(Color.MediumLow);
		c.drawTextAligned(0, 47, 256, 7, HAlign.Center, VAlign.Center,
			'STEP ' + (NOTE_CURSOR + 1) + (gateOn ? '  GATE ON  ' + noteName : '  GATE OFF'));

		c.setColorValue(Color.MediumLow); c.hline(0, 55, 256);
		const tabs = ['GATE', 'NOTE', 'COND', 'VELO', 'LEN'];
		for (let i = 0; i < 5; i++) {
			const w = i < 4 ? 51 : 52;
			if (i === 1) { c.setColorValue(Color.Bright); c.fillRect(i * 51, 56, w, 8); c.setColorValue(Color.None); }
			else { c.setColorValue(Color.MediumLow); }
			c.drawTextAligned(i * 51, 56, w, 8, HAlign.Center, VAlign.Center, tabs[i]);
		}
	}

	// Option 3 — bottom-anchored pitch bars: height = note pitch; cursor inverted
	function drawNote_3(c: Canvas): void {
		c.setFont(Font.Tiny);
		c.setColorValue(Color.MediumLow);
		c.drawText(2, 6, 'TRK 1  NOTE  P01'); c.hline(0, 7, 256);

		const CELL_Y = 9, CELL_H = 44, CELL_BOT = CELL_Y + CELL_H;

		for (let s = 0; s < 16; s++) {
			const x = 1 + s * 16;
			const on = Boolean(TRACKS[0].gates[s]);
			const isBeat = s % 4 === 0;
			const isCursor = s === NOTE_CURSOR;
			const noteVal = TRACKS[0].notes[s] || 0;
			const barH = (on && noteVal) ? Math.max(4, Math.round(((noteVal - 36) / 48) * 40)) : 4;

			if (isCursor && on) {
				// Inverted: bright cell + dark bar at bottom (pitch height)
				c.setColorValue(Color.Bright); c.fillRect(x, CELL_Y, 14, CELL_H);
				c.setColorValue(Color.None); c.fillRect(x, CELL_BOT - barH, 14, barH);
				if (isBeat) { c.hline(x, CELL_Y, 14); }
			} else if (isCursor) {
				// Cursor on gate-off step: solid bright fill
				c.setColorValue(Color.Bright); c.fillRect(x, CELL_Y, 14, CELL_H);
			} else if (on) {
				// Bright bar rising from bottom
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

		c.setColorValue(Color.MediumLow); c.hline(0, 55, 256);
		const tabs = ['GATE', 'NOTE', 'COND', 'VELO', 'LEN'];
		for (let i = 0; i < 5; i++) {
			const w = i < 4 ? 51 : 52;
			if (i === 0) { c.setColorValue(Color.Bright); c.fillRect(i * 51, 56, w, 8); c.setColorValue(Color.None); }
			else { c.setColorValue(Color.MediumLow); }
			c.drawTextAligned(i * 51, 56, w, 8, HAlign.Center, VAlign.Center, tabs[i]);
		}
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

	// ── routing ───────────────────────────────────────────────────────────────

	type PageId =
		| 'dashboard' | 'perform' | 'trackedit' | 'trackconfig'
		| 'song' | 'settings' | 'tempo' | 'quickedit';

	interface Variation {
		label: string;
		draw: (c: Canvas) => void;
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
			{ label: 'Option 1 — gate cells + embedded pitch bar', draw: drawNote_1, leds: ledsNote_1 },
			{ label: 'Option 2 — compact strip + dominant note value', draw: drawNote_2, leds: ledsNote_2 },
			{ label: 'Option 3 — bottom-anchored pitch bars (graphical)', draw: drawNote_3, leds: ledsNote_3 },
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
		trackconfig: 'Track Config', song: 'Song', settings: 'Settings',
		tempo: 'Tempo', quickedit: 'Quick Edit',
	};

	const NAV: { label: string; target: PageId }[] = [
		{ label: 'PERF key',  target: 'dashboard' },
		{ label: 'PATT key',  target: 'perform' },
		{ label: 'PG+STP0',   target: 'trackedit' },
		{ label: 'PG+STP2',   target: 'trackconfig' },
		{ label: 'PG+STP3',   target: 'song' },
		{ label: 'PG+TRK0',   target: 'settings' },
		{ label: 'TEMPO',     target: 'tempo' },
		{ label: 'PG+STP8',   target: 'quickedit' },
	];

	let currentId = $state<PageId>('dashboard');
	let selectedVar = $state(0);

	const currentVars = $derived(VARIATIONS[currentId]);
	const liveVar = $derived(currentVars[Math.min(selectedVar, currentVars.length - 1)]);
	const liveDraw = $derived(liveVar.draw);
	const liveLeds = $derived((liveVar.leds ?? defaultLeds)());

	function navigateTo(id: PageId): void {
		currentId = id;
		selectedVar = 0;
		if (typeof window !== 'undefined') window.location.hash = id;
	}

	$effect(() => {
		if (typeof window === 'undefined') return;
		const hash = window.location.hash.slice(1) as PageId;
		if (hash && hash in VARIATIONS) { currentId = hash; selectedVar = 0; }
	});
</script>

<svelte:head><title>PER|FORMER — {PAGE_NAMES[currentId]}</title></svelte:head>

<div class="sandbox">

	<!-- ── LEFT: variations for current page ─────────────────────────────── -->
	<aside class="variations">
		<h2>{PAGE_NAMES[currentId]}</h2>
		{#each currentVars as variation, i}
			<button
				class="specimen"
				class:selected={selectedVar === i}
				onclick={() => (selectedVar = i)}
			>
				<span class="var-label">{variation.label}</span>
				<Display draw={variation.draw} scale={2} />
			</button>
		{/each}
	</aside>

	<!-- ── RIGHT: click dummy ────────────────────────────────────────────── -->
	<main class="clickdummy">
		<h2>Click dummy</h2>

		<Display draw={liveDraw} scale={3} />

		<LedPanel state={liveLeds} />

		<div class="hardware">
			<div class="btn-row">
				<span class="lbl">NAV</span>
				{#each NAV as n}
					<button
						class="navbtn"
						class:active={currentId === n.target}
						onclick={() => navigateTo(n.target)}
					>{n.label}</button>
				{/each}
			</div>
			<div class="btn-row">
				<span class="lbl">STEP</span>
				{#each Array(16) as _, i}
					<button class="step">{i + 1}</button>
				{/each}
			</div>
			<div class="btn-row">
				<span class="lbl">TRACK</span>
				{#each Array(8) as _, i}
					<button class="track">{i + 1}</button>
				{/each}
			</div>
			<div class="btn-row">
				<span class="lbl">F</span>
				{#each Array(5) as _, i}<button class="fkey">F{i}</button>{/each}
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

	h2 {
		font-size: 0.75rem;
		text-transform: uppercase;
		letter-spacing: 0.15em;
		color: #888;
		margin: 0 0 1rem;
	}

	.variations {
		display: flex;
		flex-direction: column;
		gap: 1rem;
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
		font-size: 0.62rem;
		color: #555;
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
	.step          { font-size: 0.6rem; padding: 3px 1px; width: 22px; }
	.track         { font-size: 0.6rem; padding: 3px 2px; width: 28px; }
	.fkey          { font-size: 0.6rem; padding: 3px 5px; }
	.gkey          { font-size: 0.6rem; padding: 3px 5px; }
</style>
