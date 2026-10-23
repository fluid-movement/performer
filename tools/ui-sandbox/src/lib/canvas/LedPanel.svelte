<script lang="ts">
	import type { LedColor, LedState } from './LedState.js';
	import { defaultLeds } from './LedState.js';

	let { state = defaultLeds() }: { state?: LedState } = $props();

	const CSS: Record<LedColor, string> = {
		'off':         '#1c1c1c',
		'green':       '#22bb44',
		'red':         '#cc2222',
		'amber':       '#cc8800',
		'blink-green': '#22bb44',
		'blink-red':   '#cc2222',
	};

	const ANIM: Record<LedColor, string> = {
		'off':         'none',
		'green':       'none',
		'red':         'none',
		'amber':       'none',
		'blink-green': 'blink 0.5s step-start infinite',
		'blink-red':   'blink 0.5s step-start infinite',
	};
</script>

<div class="led-panel">
	<!-- Track row — 8 buttons × 28px = matches .track button width -->
	<div class="led-row">
		<span class="lbl">LED</span>
		{#each state.track as led}
			<span class="dot w28" style="--c:{CSS[led]};animation:{ANIM[led]}"></span>
		{/each}
	</div>

	<!-- Step rows — 16 buttons × 22px = matches .step button width -->
	<div class="led-row">
		<span class="lbl"></span>
		{#each state.step as led}
			<span class="dot w22" style="--c:{CSS[led]};animation:{ANIM[led]}"></span>
		{/each}
	</div>

	<!-- F-keys + globals -->
	<div class="led-row">
		<span class="lbl"></span>
		{#each state.fkey as led}
			<span class="dot fkey" style="--c:{CSS[led]};animation:{ANIM[led]}"></span>
		{/each}
		<span class="gap"></span>
		<span class="dot fkey" style="--c:{CSS[state.play]};animation:{ANIM[state.play]}" title="PLAY"></span>
		<span class="dot fkey" style="--c:{CSS[state.pattern]};animation:{ANIM[state.pattern]}" title="PAT"></span>
		<span class="dot fkey" style="--c:{CSS[state.performer]};animation:{ANIM[state.performer]}" title="PFR"></span>
	</div>
</div>

<style>
	@keyframes blink {
		50% { opacity: 0; }
	}

	.led-panel {
		display: flex;
		flex-direction: column;
		gap: 3px;
	}

	.led-row {
		display: flex;
		align-items: center;
		gap: 3px;
	}

	.lbl {
		font-size: 0.6rem;
		color: #333;
		width: 3.2rem;
		flex-shrink: 0;
	}

	.dot {
		height: 6px;
		border-radius: 50%;
		background: var(--c);
		flex-shrink: 0;
	}

	/* match button widths */
	.w28  { width: 28px; }
	.w22  { width: 22px; }
	.fkey { width: 28px; }

	.gap {
		width: 0.5rem;
		flex-shrink: 0;
	}
</style>
