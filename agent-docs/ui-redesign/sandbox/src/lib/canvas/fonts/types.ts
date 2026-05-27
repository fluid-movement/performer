export interface BitmapFontGlyph {
	offset: number;
	width: number;
	height: number;
	xAdvance: number;
	xOffset: number;
	yOffset: number;
}

export interface BitmapFont {
	bpp: 1 | 4;
	bitmap: Uint8Array;
	glyphs: BitmapFontGlyph[];
	first: number;
	last: number;
	yAdvance: number;
}
