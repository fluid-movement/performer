<script lang="ts">
	import { Canvas } from './Canvas.js';
	import { FrameBuffer } from './FrameBuffer.js';

	let {
		draw,
		scale = 4,
	}: {
		draw: (c: Canvas) => void;
		scale?: number;
	} = $props();

	let canvasEl: HTMLCanvasElement | undefined = $state(undefined);

	const fb = new FrameBuffer();
	const canvas = new Canvas(fb);

	$effect(() => {
		if (!canvasEl) return;

		fb.clear();
		draw(canvas);

		const W = FrameBuffer.WIDTH;
		const H = FrameBuffer.HEIGHT;
		const ctx = canvasEl.getContext('2d')!;
		const imageData = ctx.createImageData(W * scale, H * scale);
		const buf = imageData.data;

		for (let y = 0; y < H; y++) {
			for (let x = 0; x < W; x++) {
				const gray = fb.data[y * W + x] * 17;
				for (let sy = 0; sy < scale; sy++) {
					for (let sx = 0; sx < scale; sx++) {
						const px = ((y * scale + sy) * W * scale + (x * scale + sx)) * 4;
						buf[px] = gray;
						buf[px + 1] = gray;
						buf[px + 2] = gray;
						buf[px + 3] = 255;
					}
				}
			}
		}

		ctx.putImageData(imageData, 0, 0);
	});
</script>

<canvas
	bind:this={canvasEl}
	width={FrameBuffer.WIDTH * scale}
	height={FrameBuffer.HEIGHT * scale}
	style="image-rendering: pixelated; display: block;"
></canvas>
