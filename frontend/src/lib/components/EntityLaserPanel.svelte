<script lang="ts">
	import { untrack } from 'svelte';
	import LaserPropertiesPanel from '$lib/components/LaserPropertiesPanel.svelte';
	import { clampLaserProperties, defaultLaserProperties } from '$lib/scene/laserProperties';
	import {
		DEFAULT_LASER_GROUP_ID,
		effectiveLaserForEntity,
		nextLaserGroupId,
		type LaserGroupV1,
		type SceneEntity
	} from '$lib/scene/sceneV1';

	interface Props {
		entities?: SceneEntity[];
		selectedIndex?: number | null;
		laserGroups?: LaserGroupV1[];
		defaultLaserGroupId?: string;
	}

	let {
		entities = $bindable<SceneEntity[]>([]),
		selectedIndex = null,
		laserGroups = $bindable<LaserGroupV1[]>([]),
		defaultLaserGroupId = $bindable(DEFAULT_LASER_GROUP_ID)
	}: Props = $props();

	let draftLaser = $state(defaultLaserProperties());

	let ent = $derived(selectedIndex !== null ? entities[selectedIndex!] : null);
	let useCustomLaser = $derived(!!ent?.laser);
	let presetIdx = $derived(
		ent && laserGroups.length
			? (() => {
					const i = laserGroups.findIndex((g) => g.id === ent!.laser_group_id);
					return i >= 0 ? i : 0;
				})()
			: -1
	);

	$effect(() => {
		const i = selectedIndex;
		if (i === null) return;
		untrack(() => {
			const e = entities[i]!;
			if (e.laser) draftLaser = { ...e.laser };
		});
	});

	function persistDraft() {
		if (selectedIndex === null) return;
		const i = selectedIndex;
		entities = entities.map((row, k) =>
			k === i ? { ...row, laser: clampLaserProperties(draftLaser) } : row
		) as SceneEntity[];
	}

	function setGroupId(gid: string) {
		if (selectedIndex === null) return;
		const i = selectedIndex;
		entities = entities.map((e, j) =>
			j === i ? { ...e, laser_group_id: gid } : e
		) as SceneEntity[];
	}

	function toggleCustom(on: boolean) {
		if (selectedIndex === null) return;
		const i = selectedIndex;
		const e = entities[i]!;
		if (on) {
			const { laser: _drop, ...base } = e;
			void _drop;
			const eff = effectiveLaserForEntity(base as SceneEntity, laserGroups, defaultLaserGroupId);
			draftLaser = { ...eff };
			entities = entities.map((row, k) =>
				k === i ? { ...row, laser: clampLaserProperties(draftLaser) } : row
			) as SceneEntity[];
		} else {
			const { laser: _omit, ...rest } = e;
			void _omit;
			entities = entities.map((row, k) => (k === i ? (rest as SceneEntity) : row)) as SceneEntity[];
		}
	}

	function addPreset() {
		const id = nextLaserGroupId();
		const n = laserGroups.length + 1;
		laserGroups = [
			...laserGroups,
			{ id, name: `Preset ${n}`, laser: defaultLaserProperties() }
		];
		if (selectedIndex !== null) {
			const i = selectedIndex;
			entities = entities.map((e, j) =>
				j === i ? { ...e, laser_group_id: id } : e
			) as SceneEntity[];
		}
	}

	function removeCurrentPreset() {
		if (selectedIndex === null || !ent || laserGroups.length <= 1) return;
		const rid = ent.laser_group_id;
		const others = laserGroups.filter((g) => g.id !== rid);
		if (others.length === laserGroups.length) return;
		const fallback =
			defaultLaserGroupId !== rid
				? defaultLaserGroupId
				: (others[0]?.id ?? DEFAULT_LASER_GROUP_ID);
		entities = entities.map((e) =>
			e.laser_group_id === rid ? { ...e, laser_group_id: fallback } : e
		) as SceneEntity[];
		if (defaultLaserGroupId === rid) defaultLaserGroupId = fallback;
		laserGroups = others;
	}
</script>

<div class="ldk-card ldk-entity-laser" data-testid="editor-entity-laser">
	<h3 class="ldk-el-title">Parameters</h3>
	<p class="ldk-muted" style="margin:0 0 0.65rem;font-size:0.82rem">
		<strong>Presets</strong> are shared parameter sets: every entity that uses the same preset gets the same power,
		speed, and timing. Edit the preset here (or pick another in the job list). Use <strong>override</strong> only when
		one shape must differ.
	</p>

	<label class="ldk-el-field">
		<span>Default preset for new shapes</span>
		<select
			class="ldk-el-select"
			bind:value={defaultLaserGroupId}
			data-testid="editor-default-laser-group"
		>
			{#each laserGroups as g (g.id)}
				<option value={g.id}>{g.name}</option>
			{/each}
		</select>
	</label>

	{#if ent && selectedIndex !== null}
		<hr class="ldk-el-sep" />
		<p class="ldk-el-sub">Selected entity</p>

		<label class="ldk-el-field">
			<span>Preset</span>
			<select
				class="ldk-el-select"
				value={ent.laser_group_id}
				onchange={(ev) => setGroupId((ev.currentTarget as HTMLSelectElement).value)}
				data-testid="editor-entity-laser-group"
			>
				{#each laserGroups as g (g.id)}
					<option value={g.id}>{g.name}</option>
				{/each}
			</select>
		</label>

		<label class="ldk-el-check">
			<input
				type="checkbox"
				checked={useCustomLaser}
				data-testid="editor-entity-laser-custom"
				onchange={(ev) => toggleCustom((ev.currentTarget as HTMLInputElement).checked)}
			/>
			<span>Override preset for this entity only</span>
		</label>

		{#if !useCustomLaser && presetIdx >= 0 && laserGroups[presetIdx]}
			<div class="ldk-el-preset-editor">
				{#key `${ent.laser_group_id}-${selectedIndex}`}
					<LaserPropertiesPanel
						embedded
						heading={`Preset “${laserGroups[presetIdx]!.name}” (shared)`}
						bind:laser={laserGroups[presetIdx].laser}
					/>
				{/key}
			</div>
		{/if}

		{#if useCustomLaser && ent.laser && selectedIndex !== null}
			<div class="ldk-el-custom">
				{#key selectedIndex}
					<LaserPropertiesPanel
						embedded
						heading="Override for this entity"
						bind:laser={draftLaser}
						onLaserChange={persistDraft}
					/>
				{/key}
			</div>
		{/if}

		<div class="ldk-preset-actions">
			<button type="button" class="ldk-btn secondary" data-testid="editor-preset-add" onclick={() => addPreset()}>
				Add preset
			</button>
			<button
				type="button"
				class="ldk-btn secondary danger"
				disabled={laserGroups.length <= 1}
				data-testid="editor-preset-remove"
				onclick={() => removeCurrentPreset()}>Remove this preset</button
			>
		</div>
	{:else}
		<p class="ldk-muted" style="margin:0 0 0.65rem;font-size:0.82rem" data-testid="editor-params-empty">
			Select an entity in the job tree or on the canvas to assign a preset or override.
		</p>
		<div class="ldk-preset-actions">
			<button type="button" class="ldk-btn secondary" data-testid="editor-preset-add" onclick={() => addPreset()}>
				Add preset
			</button>
		</div>
	{/if}
</div>

<style>
	.ldk-entity-laser {
		min-width: 0;
		max-width: 100%;
		align-self: stretch;
	}
	.ldk-el-title {
		margin: 0 0 0.5rem;
		font-size: 1rem;
		font-weight: 600;
	}
	.ldk-el-sub {
		margin: 0 0 0.5rem;
		font-size: 0.85rem;
		font-weight: 600;
		color: #334155;
	}
	.ldk-el-sep {
		border: none;
		border-top: 1px solid #e8ecf0;
		margin: 0.5rem 0 0.65rem;
	}
	.ldk-el-field {
		display: flex;
		flex-direction: column;
		gap: 0.25rem;
		margin-bottom: 0.65rem;
		font-size: 0.82rem;
	}
	.ldk-el-field span {
		color: #475569;
	}
	.ldk-el-select {
		width: 100%;
		min-width: 0;
		padding: 0.35rem 0.45rem;
		border: 1px solid #c5ced9;
		border-radius: 6px;
		font-size: 0.9rem;
	}
	.ldk-el-check {
		display: flex;
		align-items: flex-start;
		gap: 0.45rem;
		margin-bottom: 0.5rem;
		font-size: 0.82rem;
		cursor: pointer;
		color: #334155;
	}
	.ldk-el-check input {
		margin-top: 0.15rem;
	}
	.ldk-el-preset-editor,
	.ldk-el-custom {
		margin-top: 0.5rem;
		padding-top: 0.5rem;
		border-top: 1px solid #e8ecf0;
	}
	.ldk-preset-actions {
		display: flex;
		flex-wrap: wrap;
		gap: 0.35rem;
		margin-top: 0.65rem;
	}
	.ldk-btn.danger:hover:not(:disabled) {
		border-color: #b91c1c;
		color: #b91c1c;
	}
</style>
