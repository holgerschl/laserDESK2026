<script lang="ts">
	import { untrack } from 'svelte';
	import LaserPropertiesPanel from '$lib/components/LaserPropertiesPanel.svelte';
	import { clampLaserProperties, defaultLaserProperties } from '$lib/scene/laserProperties';
	import {
		DEFAULT_LASER_GROUP_ID,
		effectiveLaserForEntity,
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
		laserGroups = [],
		defaultLaserGroupId = DEFAULT_LASER_GROUP_ID
	}: Props = $props();

	let draftLaser = $state(defaultLaserProperties());

	let ent = $derived(selectedIndex !== null ? entities[selectedIndex!] : null);
	let useCustomLaser = $derived(!!ent?.laser);

	/** Sync draft when selection changes only (avoid resetting fields while editing). */
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
</script>

{#if ent && selectedIndex !== null}
	<div class="ldk-card ldk-entity-laser" data-testid="editor-entity-laser">
		<h3 class="ldk-el-title">Selected entity</h3>
		<p class="ldk-muted" style="margin:0 0 0.65rem;font-size:0.82rem">
			Assign a <strong>group</strong> to match other entities, or enable <strong>custom laser</strong> for this item only.
		</p>

		<label class="ldk-el-field">
			<span>Laser group</span>
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
			<span>Custom laser parameters (override group)</span>
		</label>

		{#if useCustomLaser && ent.laser && selectedIndex !== null}
			<div class="ldk-el-custom">
				{#key selectedIndex}
					<LaserPropertiesPanel
						embedded
						heading="Custom laser"
						bind:laser={draftLaser}
						onLaserChange={persistDraft}
					/>
				{/key}
			</div>
		{/if}
	</div>
{/if}

<style>
	.ldk-entity-laser {
		min-width: 0;
		max-width: 100%;
	}
	.ldk-el-title {
		margin: 0 0 0.5rem;
		font-size: 1rem;
		font-weight: 600;
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
	.ldk-el-custom {
		margin-top: 0.5rem;
		padding-top: 0.5rem;
		border-top: 1px solid #e8ecf0;
	}
</style>
