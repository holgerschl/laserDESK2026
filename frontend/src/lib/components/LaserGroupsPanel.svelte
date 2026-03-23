<script lang="ts">
	import LaserPropertiesPanel from '$lib/components/LaserPropertiesPanel.svelte';
	import { clampLaserProperties, defaultLaserProperties } from '$lib/scene/laserProperties';
	import { nextLaserGroupId, type LaserGroupV1 } from '$lib/scene/sceneV1';

	interface Props {
		laserGroups?: LaserGroupV1[];
		defaultLaserGroupId?: string;
		onBeforeRemoveGroup?: (removedId: string, fallbackId: string) => void;
	}

	let {
		laserGroups = $bindable<LaserGroupV1[]>([]),
		defaultLaserGroupId = $bindable<string>('g0'),
		onBeforeRemoveGroup
	}: Props = $props();

	let editGroupId = $state<string>('');

	$effect(() => {
		if (!laserGroups.length) return;
		if (!editGroupId || !laserGroups.some((g: LaserGroupV1) => g.id === editGroupId)) {
			editGroupId = laserGroups[0]!.id;
		}
	});

	function addGroup() {
		const id = nextLaserGroupId();
		const n = laserGroups.length + 1;
		laserGroups = [
			...laserGroups,
			{ id, name: `Group ${n}`, laser: defaultLaserProperties() }
		];
		editGroupId = id;
	}

	function duplicateGroup(id: string) {
		const src = laserGroups.find((g: LaserGroupV1) => g.id === id);
		if (!src) return;
		const nid = nextLaserGroupId();
		laserGroups = [
			...laserGroups,
			{
				id: nid,
				name: `${src.name} (copy)`,
				laser: clampLaserProperties({ ...src.laser })
			}
		];
		editGroupId = nid;
	}

	function removeGroup(id: string) {
		if (laserGroups.length <= 1) return;
		const fb =
			defaultLaserGroupId === id ? laserGroups.find((g: LaserGroupV1) => g.id !== id)?.id : defaultLaserGroupId;
		const fallback = fb ?? laserGroups.find((g: LaserGroupV1) => g.id !== id)?.id;
		if (!fallback) return;
		onBeforeRemoveGroup?.(id, fallback);
		if (defaultLaserGroupId === id) defaultLaserGroupId = fallback;
		laserGroups = laserGroups.filter((g: LaserGroupV1) => g.id !== id);
		if (editGroupId === id) editGroupId = fallback;
	}

	let editIdx = $derived(laserGroups.findIndex((g: LaserGroupV1) => g.id === editGroupId));
</script>

<div class="ldk-card ldk-laser-groups" data-testid="editor-laser-groups">
	<h3 class="ldk-lg-title">Laser groups</h3>
	<p class="ldk-muted" style="margin:0 0 0.75rem;font-size:0.82rem">
		Entities share parameters by sharing a group. Add groups to define different power/speed presets; assign entities in
		the job tree or under <strong>Selected entity</strong>.
	</p>

	<label class="ldk-lg-field">
		<span>Default for new shapes</span>
		<select
			class="ldk-lg-select"
			bind:value={defaultLaserGroupId}
			data-testid="editor-default-laser-group"
		>
			{#each laserGroups as g (g.id)}
				<option value={g.id}>{g.name} ({g.id})</option>
			{/each}
		</select>
	</label>

	<ul class="ldk-lg-list">
		{#each laserGroups as g, i (g.id)}
			<li class="ldk-lg-row">
				<input
					class="ldk-lg-name"
					type="text"
					bind:value={laserGroups[i].name}
					aria-label="Group name"
					data-testid="editor-laser-group-name"
				/>
				<div class="ldk-lg-actions">
					<button
						type="button"
						class="ldk-btn secondary"
						data-testid="editor-laser-group-dup"
						onclick={() => duplicateGroup(g.id)}>Duplicate</button
					>
					<button
						type="button"
						class="ldk-btn secondary danger"
						disabled={laserGroups.length <= 1}
						data-testid="editor-laser-group-remove"
						onclick={() => removeGroup(g.id)}>Remove</button
					>
				</div>
				<button
					type="button"
					class="ldk-lg-edit"
					class:ldk-lg-edit-active={editGroupId === g.id}
					data-testid="editor-laser-group-edit"
					onclick={() => (editGroupId = g.id)}>Edit parameters</button
				>
			</li>
		{/each}
	</ul>

	<button type="button" class="ldk-btn secondary" data-testid="editor-laser-group-add" onclick={() => addGroup()}
		>Add group</button
	>

	{#if editIdx >= 0}
		<div class="ldk-lg-editor">
			<LaserPropertiesPanel
				embedded
				heading={`Parameters for “${laserGroups[editIdx]!.name}”`}
				bind:laser={laserGroups[editIdx].laser}
			/>
		</div>
	{/if}
</div>

<style>
	.ldk-laser-groups {
		min-width: 0;
		max-width: 100%;
	}
	.ldk-lg-title {
		margin: 0 0 0.5rem;
		font-size: 1rem;
		font-weight: 600;
	}
	.ldk-lg-field {
		display: flex;
		flex-direction: column;
		gap: 0.25rem;
		margin-bottom: 0.65rem;
		font-size: 0.82rem;
	}
	.ldk-lg-field span {
		color: #475569;
	}
	.ldk-lg-select {
		width: 100%;
		min-width: 0;
		padding: 0.35rem 0.45rem;
		border: 1px solid #c5ced9;
		border-radius: 6px;
		font-size: 0.9rem;
	}
	.ldk-lg-list {
		list-style: none;
		margin: 0 0 0.65rem;
		padding: 0;
		border: 1px solid #d8dee6;
		border-radius: 6px;
		overflow: hidden;
		background: #fafbfc;
	}
	.ldk-lg-row {
		display: grid;
		grid-template-columns: 1fr auto;
		grid-template-rows: auto auto;
		gap: 0.35rem 0.5rem;
		padding: 0.5rem 0.6rem;
		border-bottom: 1px solid #e8ecf0;
		align-items: center;
	}
	.ldk-lg-row:last-child {
		border-bottom: none;
	}
	.ldk-lg-name {
		grid-column: 1;
		grid-row: 1;
		min-width: 0;
		padding: 0.3rem 0.4rem;
		border: 1px solid #c5ced9;
		border-radius: 6px;
		font-size: 0.85rem;
	}
	.ldk-lg-actions {
		grid-column: 2;
		grid-row: 1;
		display: flex;
		flex-wrap: wrap;
		gap: 0.25rem;
		justify-content: flex-end;
	}
	.ldk-lg-edit {
		grid-column: 1 / -1;
		grid-row: 2;
		justify-self: start;
		padding: 0.2rem 0.5rem;
		font-size: 0.78rem;
		border: 1px solid #c5ced9;
		border-radius: 6px;
		background: #fff;
		cursor: pointer;
		color: #334155;
	}
	.ldk-lg-edit-active {
		border-color: #246;
		background: #e8eef5;
		font-weight: 600;
	}
	.ldk-lg-editor {
		margin-top: 0.75rem;
		padding-top: 0.75rem;
		border-top: 1px solid #e8ecf0;
	}
	.ldk-btn.danger:hover:not(:disabled) {
		border-color: #b91c1c;
		color: #b91c1c;
	}
</style>
