// LED state model for the 32 bi-color LEDs on PER|FORMER hardware.
// Mapping: Track0-7 (8), Step0-15 (16), F0-F4 (5), Play + Pattern + Performer (3) = 32

export type LedColor = 'off' | 'green' | 'red' | 'amber' | 'blink-green' | 'blink-red';

export interface LedState {
	track: LedColor[];    // [0..7]  — Track buttons
	step: LedColor[];     // [0..15] — Step buttons
	fkey: LedColor[];     // [0..4]  — F0–F4
	play: LedColor;       // Play button
	pattern: LedColor;    // Pattern button
	performer: LedColor;  // Performer button
}

export function defaultLeds(): LedState {
	return {
		track:     Array<LedColor>(8).fill('off'),
		step:      Array<LedColor>(16).fill('off'),
		fkey:      Array<LedColor>(5).fill('off'),
		play:      'off',
		pattern:   'off',
		performer: 'off',
	};
}
