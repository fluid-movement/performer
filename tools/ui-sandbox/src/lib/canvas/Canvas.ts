import { FrameBuffer } from './FrameBuffer.js';
import type { BitmapFont } from './fonts/types.js';
import { tiny5x5, TINY_FONT_HEIGHT, TINY_FONT_OFFSET } from './fonts/tiny5x5.js';
import { ati8x8, ATI_FONT_HEIGHT, ATI_FONT_OFFSET } from './fonts/ati8x8.js';

export enum Color {
	None = 0,
	Low = 0x3,
	MediumLow = 0x5,
	Medium = 0x7,
	MediumBright = 0xa,
	Bright = 0xf,
}

export enum BlendMode {
	Set = 0,
	Add = 1,
	Sub = 2,
}

export enum Font {
	Tiny = 0,
	Normal = 1,
}

export enum HAlign {
	Left = 0,
	Center = 1,
	Right = 2,
}

export enum VAlign {
	Top = 0,
	Center = 1,
	Bottom = 2,
}

const FONTS: BitmapFont[] = [tiny5x5, ati8x8];
const FONT_HEIGHTS = [TINY_FONT_HEIGHT, ATI_FONT_HEIGHT];
const FONT_OFFSETS = [TINY_FONT_OFFSET, ATI_FONT_OFFSET];

export class Canvas {
	private _fb: FrameBuffer;
	private _color = Color.Bright;
	private _brightness = 1.0;
	private _font = Font.Tiny;
	private _blendMode = BlendMode.Set;

	constructor(fb: FrameBuffer) {
		this._fb = fb;
	}

	setColor(color: Color): void {
		this._color = Math.round(color * this._brightness) & 0xf;
	}

	setColorValue(value: number): void {
		this._color = Math.round(value * this._brightness) & 0xf;
	}

	setBrightness(brightness: number): void {
		this._brightness = brightness;
	}

	setFont(font: Font): void {
		this._font = font;
	}

	setBlendMode(mode: BlendMode): void {
		this._blendMode = mode;
	}

	// ── pixel-level operations ──────────────────────────────────────────────

	point(x: number, y: number): void {
		this._blit(Math.round(x), Math.round(y), this._color);
	}

	hline(x: number, y: number, w: number): void {
		const yi = Math.round(y);
		for (let i = 0; i < w; i++) {
			this._blit(x + i, yi, this._color);
		}
	}

	vline(x: number, y: number, h: number): void {
		const xi = Math.round(x);
		for (let i = 0; i < h; i++) {
			this._blit(xi, y + i, this._color);
		}
	}

	// Xiaolin Wu antialiased line
	line(x0: number, y0: number, x1: number, y1: number): void {
		const steep = Math.abs(y1 - y0) > Math.abs(x1 - x0);
		if (steep) {
			[x0, y0] = [y0, x0];
			[x1, y1] = [y1, x1];
		}
		if (x0 > x1) {
			[x0, x1] = [x1, x0];
			[y0, y1] = [y1, y0];
		}

		const dx = x1 - x0;
		const dy = y1 - y0;
		const gradient = dx === 0 ? 1.0 : dy / dx;

		const plot = (px: number, py: number, c: number) => {
			if (steep) this._blitCovered(py, px, c);
			else this._blitCovered(px, py, c);
		};

		const frac = (n: number) => n - Math.floor(n);
		const rfrac = (n: number) => 1 - frac(n);

		// first endpoint
		let xend = Math.round(x0);
		let yend = y0 + gradient * (xend - x0);
		let xgap = rfrac(x0 + 0.5);
		const xpxl1 = xend;
		const ypxl1 = Math.floor(yend);
		plot(xpxl1, ypxl1, rfrac(yend) * xgap);
		plot(xpxl1, ypxl1 + 1, frac(yend) * xgap);
		let intery = yend + gradient;

		// second endpoint
		xend = Math.round(x1);
		yend = y1 + gradient * (xend - x1);
		xgap = frac(x1 + 0.5);
		const xpxl2 = xend;
		const ypxl2 = Math.floor(yend);
		plot(xpxl2, ypxl2, rfrac(yend) * xgap);
		plot(xpxl2, ypxl2 + 1, frac(yend) * xgap);

		for (let x = xpxl1 + 1; x < xpxl2; x++) {
			plot(x, Math.floor(intery), rfrac(intery));
			plot(x, Math.floor(intery) + 1, frac(intery));
			intery += gradient;
		}
	}

	rect(x: number, y: number, w: number, h: number): void {
		this.hline(x, y, w);
		this.hline(x, y + h - 1, w);
		this.vline(x, y, h);
		this.vline(x + w - 1, y, h);
	}

	fillRect(x: number, y: number, w: number, h: number): void {
		for (let row = y; row < y + h; row++) {
			this.hline(x, row, w);
		}
	}

	// ── bitmap rendering ────────────────────────────────────────────────────

	// 1bpp bitmap: LSB-first, rows packed contiguously
	drawBitmap(x: number, y: number, w: number, h: number, bitmap: Uint8Array, offset = 0): void {
		let shift = 0;
		let byteIdx = offset;
		for (let row = 0; row < h; row++) {
			for (let col = 0; col < w; col++) {
				const bit = (bitmap[byteIdx] >> shift) & 1;
				if (bit) this._blit(x + col, y + row, this._color);
				shift++;
				if (shift >= 8) {
					shift = 0;
					byteIdx++;
				}
			}
		}
	}

	// 4bpp bitmap: nibble per pixel, LSB-first
	drawBitmap4bit(x: number, y: number, w: number, h: number, bitmap: Uint8Array, offset = 0): void {
		let shift = 0;
		let byteIdx = offset;
		for (let row = 0; row < h; row++) {
			for (let col = 0; col < w; col++) {
				const nibble = (bitmap[byteIdx] >> shift) & 0xf;
				const pixel = (nibble * this._color) >> 4;
				if (pixel) this._blit(x + col, y + row, pixel);
				shift += 4;
				if (shift >= 8) {
					shift = 0;
					byteIdx++;
				}
			}
		}
	}

	// ── text rendering ──────────────────────────────────────────────────────

	drawText(x: number, y: number, str: string): void {
		const font = FONTS[this._font];
		let cx = x;
		for (const ch of str) {
			const code = ch.charCodeAt(0);
			if (code < font.first || code > font.last) continue;
			const g = font.glyphs[code - font.first];
			if (g.width > 0 && g.height > 0) {
				this.drawBitmap(cx + g.xOffset, y + g.yOffset, g.width, g.height, font.bitmap, g.offset);
			}
			cx += g.xAdvance;
		}
	}

	drawTextAligned(
		x: number,
		y: number,
		w: number,
		h: number,
		hAlign: HAlign,
		vAlign: VAlign,
		str: string,
	): void {
		const tw = this.textWidth(str);
		const th = this.textHeight();

		let tx = x;
		if (hAlign === HAlign.Center) tx = x + Math.floor((w - tw) / 2);
		else if (hAlign === HAlign.Right) tx = x + w - tw;

		let ty = y;
		if (vAlign === VAlign.Center) ty = y + Math.floor((h - th) / 2);
		else if (vAlign === VAlign.Bottom) ty = y + h - th;

		ty += FONT_OFFSETS[this._font];
		this.drawText(tx, ty, str);
	}

	textWidth(str: string): number {
		const font = FONTS[this._font];
		let width = 0;
		for (const ch of str) {
			const code = ch.charCodeAt(0);
			if (code < font.first || code > font.last) continue;
			width += font.glyphs[code - font.first].xAdvance;
		}
		return width;
	}

	textHeight(): number {
		return FONT_HEIGHTS[this._font];
	}

	// ── internals ───────────────────────────────────────────────────────────

	private _blit(x: number, y: number, value: number): void {
		if (this._blendMode === BlendMode.Set) this._fb.blitSet(x, y, value);
		else if (this._blendMode === BlendMode.Add) this._fb.blitAdd(x, y, value);
		else this._fb.blitSub(x, y, value);
	}

	private _blitCovered(x: number, y: number, coverage: number): void {
		const value = Math.round(this._color * coverage);
		if (value > 0) this._blit(x, y, value);
	}
}
