<script lang="ts">
	import type { Action } from 'svelte/action';
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
		/** Indices of selected entities (same as job tree / canvas). */
		selectedIndices?: number[];
		/** Number of selected entities (job tree / canvas); when &gt; 1, show multi-select hint. */
		selectionCount?: number;
		laserGroups?: LaserGroupV1[];
		defaultLaserGroupId?: string;
	}

	let {
		entities = $bindable<SceneEntity[]>([]),
		selectedIndex = null,
		selectedIndices = [] as number[],
		selectionCount = 1,
		laserGroups = $bindable<LaserGroupV1[]>([]),
		defaultLaserGroupId = $bindable(DEFAULT_LASER_GROUP_ID)
	}: Props = $props();

	const setIndeterminate: Action<HTMLInputElement, boolean> = (node, param) => {
		node.indeterminate = param;
		return {
			update(p) {
				node.indeterminate = p;
			}
		};
	};

	let draftLaser = $state(defaultLaserProperties());

	/** Which preset’s laser block is shown at the top (always; independent of entity selection). */
	let presetEditId = $state(DEFAULT_LASER_GROUP_ID);

	let ent = $derived(selectedIndex !== null ? entities[selectedIndex!] : null);

	let presetEditIdx = $derived(laserGroups.findIndex((g) => g.id === presetEditId));

	$effect(() => {
		if (laserGroups.length === 0) return;
		if (!laserGroups.some((g) => g.id === presetEditId)) {
			presetEditId = laserGroups[0]!.id;
		}
	});

	/** Entity indices to apply laser changes to (all selected, or single). */
	function targets(): number[] {
		if (selectedIndex === null) return [];
		if (selectedIndices.length > 0) return selectedIndices;
		return [selectedIndex];
	}

	let allCustom = $derived(
		selectedIndex !== null && targets().length > 0
			? targets().every((i) => !!entities[i]?.laser)
			: false
	);
	let someCustom = $derived(
		selectedIndex !== null && targets().length > 0
			? targets().some((i) => !!entities[i]?.laser)
			: false
	);
	let useCustomMixed = $derived(
		selectedIndex !== null && targets().length > 1 ? someCustom && !allCustom : false
	);
	let showOverridePanel = $derived(
		selectedIndex !== null && (allCustom || someCustom || useCustomMixed)
	);

	$effect(() => {
		const i = selectedIndex;
		if (i === null) return;
		untrack(() => {
			const e = entities[i]!;
			if (e.laser) draftLaser = { ...e.laser };
			else if (useCustomMixed) {
				const eff = effectiveLaserForEntity(e as SceneEntity, laserGroups, defaultLaserGroupId);
				draftLaser = { ...eff };
			}
		});
	});

	function persistDraft() {
		if (selectedIndex === null) return;
		const idxs = targets();
		if (idxs.length === 0) return;
		const L = clampLaserProperties(draftLaser);
		entities = entities.map((row, k) =>
			idxs.includes(k) ? { ...row, laser: L } : row
		) as SceneEntity[];
	}

	function setGroupId(gid: string) {
		if (selectedIndex === null) return;
		const idxs = targets();
		if (idxs.length === 0) return;
		entities = entities.map((e, j) =>
			idxs.includes(j) ? { ...e, laser_group_id: gid } : e
		) as SceneEntity[];
	}

	function toggleCustom(on: boolean) {
		if (selectedIndex === null) return;
		const idxs = targets();
		if (idxs.length === 0) return;
		if (on) {
			entities = entities.map((row, k) => {
				if (!idxs.includes(k)) return row;
				const { laser: _drop, ...base } = row;
				void _drop;
				const eff = effectiveLaserForEntity(base as SceneEntity, laserGroups, defaultLaserGroupId);
				return { ...row, laser: clampLaserProperties(eff) };
			}) as SceneEntity[];
			const e = entities[selectedIndex]!;
			if (e.laser) draftLaser = { ...e.laser };
		} else {
			entities = entities.map((row, k) => {
				if (!idxs.includes(k)) return row;
				const { laser: _omit, ...rest } = row;
				void _omit;
				return rest as SceneEntity;
			}) as SceneEntity[];
		}
	}

	function addPreset() {
		const id = nextLaserGroupId();
		const n = laserGroups.length + 1;
		laserGroups = [
			...laserGroups,
			{ id, name: `Preset ${n}`, laser: defaultLaserProperties() }
		];
		presetEditId = id;
		if (selectedIndex === null) return;
		const idxs = targets();
		if (idxs.length === 0) return;
		entities = entities.map((e, j) =>
			idxs.includes(j) ? { ...e, laser_group_id: id } : e
		) as SceneEntity[];
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

	function setPresetName(i: number, raw: string) {
		laserGroups = laserGroups.map((g, j) => (j === i ? { ...g, name: raw } : g));
	}

	function blurPresetName(i: number, raw: string) {
		const v = raw.trim();
		if (v) return;
		laserGroups = laserGroups.map((g, j) =>
			j === i ? { ...g, name: `Preset ${i + 1}` } : g
		);
	}
</script>

<div class="ldk-card ldk-entity-laser" data-testid="editor-entity-laser">
	<h3 class="ldk-el-title">Parameters</h3>
	{#if selectionCount > 1}
		<p class="ldk-muted ldk-el-hint" data-testid="editor-multi-select-hint">
			<strong>{selectionCount}</strong> entities selected — <strong>Preset</strong>, <strong>override</strong>, and
			<strong>override values</strong> apply to <strong>all selected</strong>. Shared preset parameters still update
			that preset for every entity that uses it.
		</p>
	{/if}
	<p class="ldk-muted ldk-el-hint">
		<strong>Presets</strong> are shared parameter sets: every entity that uses the same preset gets the same power,
		speed, and timing. Edit the preset here (or pick another in the job list). Use <strong>override</strong> only when
		one shape must differ.
	</p>

	<div class="ldk-preset-names" data-testid="editor-preset-names">
		<p class="ldk-el-sub ldk-el-sub-tight">Preset names</p>
		<p class="ldk-muted ldk-el-micro">
			Shown in dropdowns and the job tree. The internal id (left) is fixed; the display name is editable.
		</p>
		<ul class="ldk-preset-names-list">
			{#each laserGroups as g, i (g.id)}
				<li class="ldk-preset-name-row">
					<label class="ldk-preset-name-label">
						<span class="ldk-preset-name-id" title="Internal id (not editable)">{g.id}</span>
						<input
							type="text"
							class="ldk-preset-name-input"
							value={g.name}
							aria-label={`Display name for preset ${g.id}`}
							data-testid="editor-preset-name"
							data-preset-id={g.id}
							oninput={(ev) =>
								setPresetName(i, (ev.currentTarget as HTMLInputElement).value)}
							onblur={(ev) =>
								blurPresetName(i, (ev.currentTarget as HTMLInputElement).value)}
						/>
					</label>
				</li>
			{/each}
		</ul>
	</div>

	<div class="ldk-preset-laser-block" data-testid="editor-preset-laser-params">
		<p class="ldk-el-sub ldk-el-sub-tight">Preset parameters</p>
		<p class="ldk-muted ldk-el-micro">
			Power, speed, and timing for each shared preset. Choose which preset to edit below.
		</p>
		<label class="ldk-el-field">
			<span>Edit parameters for</span>
			<select
				class="ldk-el-select"
				bind:value={presetEditId}
				data-testid="editor-preset-edit-target"
			>
				{#each laserGroups as g (g.id)}
					<option value={g.id}>{g.name}</option>
				{/each}
			</select>
		</label>
		{#if presetEditIdx >= 0 && laserGroups[presetEditIdx]}
			<div class="ldk-el-preset-editor">
				{#key presetEditId}
					<LaserPropertiesPanel
						embedded
						heading={`Preset “${laserGroups[presetEditIdx]!.name}” (shared)`}
						bind:laser={laserGroups[presetEditIdx].laser}
					/>
				{/key}
			</div>
		{/if}
	</div>

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
		<p class="ldk-el-sub">{selectionCount > 1 ? 'Selected entities' : 'Selected entity'}</p>

		<label class="ldk-el-field">
			<span>Preset{selectionCount > 1 ? ' (all selected)' : ''}</span>
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
				checked={allCustom}
				use:setIndeterminate={useCustomMixed}
				data-testid="editor-entity-laser-custom"
				onchange={(ev) => toggleCustom((ev.currentTarget as HTMLInputElement).checked)}
			/>
			<span
				>{selectionCount > 1
					? 'Override preset (applies to all selected)'
					: 'Override preset for this entity only'}</span
			>
		</label>

		{#if showOverridePanel && selectedIndex !== null}
			<div class="ldk-el-custom">
				{#key `${selectedIndex}-${selectionCount}`}
					<LaserPropertiesPanel
						embedded
						heading={selectionCount > 1
							? 'Override for all selected entities'
							: 'Override for this entity'}
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
		<p class="ldk-muted ldk-el-hint" data-testid="editor-params-empty">
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
		margin: 0 0 0.35rem;
		font-size: 0.95rem;
		font-weight: 600;
	}
	.ldk-el-hint {
		margin: 0 0 0.45rem;
		font-size: 0.8rem;
		line-height: 1.32;
	}
	.ldk-el-micro {
		margin: 0 0 0.35rem;
		font-size: 0.76rem;
		line-height: 1.3;
	}
	.ldk-el-sub {
		margin: 0 0 0.4rem;
		font-size: 0.82rem;
		font-weight: 600;
		color: #334155;
	}
	.ldk-el-sub-tight {
		margin-bottom: 0.25rem;
	}
	.ldk-el-sep {
		border: none;
		border-top: 1px solid #e8ecf0;
		margin: 0.4rem 0 0.45rem;
	}
	.ldk-el-field {
		display: flex;
		flex-direction: column;
		gap: 0.18rem;
		margin-bottom: 0.45rem;
		font-size: 0.8rem;
	}
	.ldk-el-field span {
		color: #475569;
	}
	.ldk-el-select {
		width: 100%;
		min-width: 0;
		padding: 0.28rem 0.4rem;
		border: 1px solid #c5ced9;
		border-radius: 6px;
		font-size: 0.9rem;
	}
	.ldk-el-check {
		display: flex;
		align-items: flex-start;
		gap: 0.35rem;
		margin-bottom: 0.4rem;
		font-size: 0.8rem;
		cursor: pointer;
		color: #334155;
	}
	.ldk-el-check input {
		margin-top: 0.15rem;
	}
	.ldk-el-preset-editor,
	.ldk-el-custom {
		margin-top: 0.35rem;
		padding-top: 0.4rem;
		border-top: 1px solid #e8ecf0;
	}
	.ldk-preset-names {
		margin-bottom: 0.5rem;
		padding-bottom: 0.45rem;
		border-bottom: 1px solid #e8ecf0;
	}
	.ldk-preset-laser-block {
		margin-bottom: 0.5rem;
		padding-bottom: 0.45rem;
		border-bottom: 1px solid #e8ecf0;
	}
	.ldk-preset-names-list {
		list-style: none;
		margin: 0;
		padding: 0;
		display: flex;
		flex-direction: column;
		gap: 0.28rem;
	}
	.ldk-preset-name-row {
		margin: 0;
	}
	.ldk-preset-name-label {
		display: grid;
		grid-template-columns: minmax(4.5rem, 6rem) minmax(0, 1fr);
		align-items: center;
		gap: 0.4rem;
		font-size: 0.76rem;
	}
	.ldk-preset-name-id {
		font-family: ui-monospace, monospace;
		font-size: 0.72rem;
		color: #64748b;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.ldk-preset-name-input {
		width: 100%;
		min-width: 0;
		padding: 0.24rem 0.4rem;
		border: 1px solid #c5ced9;
		border-radius: 6px;
		font-size: 0.88rem;
	}
	.ldk-preset-actions {
		display: flex;
		flex-wrap: wrap;
		gap: 0.28rem;
		margin-top: 0.45rem;
	}
	.ldk-btn.danger:hover:not(:disabled) {
		border-color: #b91c1c;
		color: #b91c1c;
	}
</style>
