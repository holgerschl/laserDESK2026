<script lang="ts">
	import {
		clampLaserProperties,
		defaultLaserProperties,
		type LaserPropertiesV1
	} from '$lib/scene/laserProperties';

	let { laser = $bindable<LaserPropertiesV1>(defaultLaserProperties()) } = $props();

	function sync() {
		laser = clampLaserProperties(laser);
	}
</script>

<div class="ldk-card ldk-laser-panel" data-testid="editor-laser-panel">
	<h3 class="ldk-laser-title">Laser / process</h3>
	<p class="ldk-muted" style="margin:0 0 0.75rem;font-size:0.82rem">
		Stored in <code>scene_v1.laser</code> when you submit (backend ignores it for geometry parsing).
	</p>
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
		margin: 0 0 0.5rem;
		font-size: 1rem;
		font-weight: 600;
	}
	.ldk-laser-grid {
		display: grid;
		grid-template-columns: 1fr 1fr;
		gap: 0.65rem 0.85rem;
	}
	.ldk-field {
		display: flex;
		flex-direction: column;
		gap: 0.25rem;
		font-size: 0.82rem;
	}
	.ldk-field span {
		color: #475569;
	}
	.ldk-field input {
		padding: 0.35rem 0.45rem;
		border: 1px solid #c5ced9;
		border-radius: 6px;
		font-size: 0.9rem;
	}
	.ldk-field-span2 {
		grid-column: 1 / -1;
	}
</style>
