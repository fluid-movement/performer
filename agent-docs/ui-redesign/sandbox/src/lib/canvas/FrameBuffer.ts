export class FrameBuffer {
	static readonly WIDTH = 256;
	static readonly HEIGHT = 64;

	readonly data: Uint8Array;

	constructor() {
		this.data = new Uint8Array(FrameBuffer.WIDTH * FrameBuffer.HEIGHT);
	}

	clear(): void {
		this.data.fill(0);
	}

	get(x: number, y: number): number {
		if (x < 0 || x >= FrameBuffer.WIDTH || y < 0 || y >= FrameBuffer.HEIGHT) return 0;
		return this.data[y * FrameBuffer.WIDTH + x];
	}

	blitSet(x: number, y: number, value: number): void {
		if (x < 0 || x >= FrameBuffer.WIDTH || y < 0 || y >= FrameBuffer.HEIGHT) return;
		this.data[y * FrameBuffer.WIDTH + x] = value & 0xf;
	}

	blitAdd(x: number, y: number, value: number): void {
		if (x < 0 || x >= FrameBuffer.WIDTH || y < 0 || y >= FrameBuffer.HEIGHT) return;
		const idx = y * FrameBuffer.WIDTH + x;
		this.data[idx] = (this.data[idx] + value) & 0xf;
	}

	blitSub(x: number, y: number, value: number): void {
		if (x < 0 || x >= FrameBuffer.WIDTH || y < 0 || y >= FrameBuffer.HEIGHT) return;
		const idx = y * FrameBuffer.WIDTH + x;
		const cur = this.data[idx];
		this.data[idx] = cur > value ? cur - value : 0;
	}
}
