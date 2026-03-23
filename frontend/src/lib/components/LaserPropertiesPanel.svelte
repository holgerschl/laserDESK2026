<script lang="ts">
	import {
		clampLaserProperties,
		defaultLaserProperties,
		type LaserPropertiesV1
	} from '$lib/scene/laserProperties';

	interface Props {
		laser?: LaserPropertiesV1;
		embedded?: boolean;
		heading?: string;
		onLaserChange?: () => void;
	}

	let {
		laser = $bindable(defaultLaserProperties() satisfies LaserPropertiesV1),
		embedded = false,
		heading = 'Laser / process',
		onLaserChange
	}: Props = $props();

	function sync() {
		laser = clampLaserProperties(laser);
		onLaserChange?.();
	}
</script>

<div
	class="ldk-laser-panel"
	class:ldk-card={!embedded}
	data-testid="editor-laser-panel"
>
	<h3 class="ldk-laser-title">{heading}</h3>
	{#if !embedded}
		<p class="ldk-muted" style="margin:0 0 0.75rem;font-size:0.82rem">
			Stored in <code>scene_v1</code> when you submit (backend ignores laser for geometry parsing).
		</p>
	{/if}
	<div class="ldk-laser-grid">
		<label class="ldk-field">
			<span>Power (%)</span>
			<input
				type="number"
				min="0"
				max="100"
				step="0.5"
				bind:value={laser.power_percent}
				onchange={sync}
				data-testid="editor-laser-power"
			/>
		</label>
		<label class="ldk-field">
			<span>Frequency (kHz)</span>
			<input
				type="number"
				min="0.1"
				max="4000"
				step="0.1"
				bind:value={laser.frequency_khz}
				onchange={sync}
				data-testid="editor-laser-frequency"
			/>
		</label>
		<label class="ldk-field">
			<span>Mark speed (mm/s)</span>
			<input
				type="number"
				min="0.1"
				step="1"
				bind:value={laser.mark_speed_mm_s}
				onchange={sync}
				data-testid="editor-laser-mark-speed"
			/>
		</label>
		<label class="ldk-field">
			<span>Jump speed (mm/s)</span>
			<input
				type="number"
				min="0.1"
				step="10"
				bind:value={laser.jump_speed_mm_s}
				onchange={sync}
				data-testid="editor-laser-jump-speed"
			/>
		</label>
		<label class="ldk-field ldk-field-span2">
			<span>Pulse width (ns)</span>
			<input
				type="number"
				min="0"
				step="1"
				bind:value={laser.pulse_width_ns}
				onchange={sync}
				data-testid="editor-laser-pulse"
			/>
		</label>
	</div>
</div>

<style>
	.ldk-laser-title {
		margin: 0 0 0.35rem;
		font-size: 0.92rem;
		font-weight: 600;
	}
	.ldk-laser-panel {
		min-width: 0;
		max-width: 100%;
	}
	.ldk-laser-grid {
		display: grid;
		grid-template-columns: repeat(2, minmax(0, 1fr));
		gap: 0.4rem 0.55rem;
	}
	.ldk-field {
		display: flex;
		flex-direction: column;
		gap: 0.25rem;
		font-size: 0.82rem;
		min-width: 0;
	}
	.ldk-field span {
		color: #475569;
	}
	.ldk-field input {
		width: 100%;
		min-width: 0;
		max-width: 100%;
		padding: 0.28rem 0.4rem;
		border: 1px solid #c5ced9;
		border-radius: 6px;
		font-size: 0.9rem;
	}
	.ldk-field-span2 {
		grid-column: 1 / -1;
	}
</style>
