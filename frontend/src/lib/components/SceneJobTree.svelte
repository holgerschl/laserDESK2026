<script lang="ts">
	import { applyShiftSelection } from '$lib/scene/selection';
	import {
		anchorIndexAfterBlockMoveDown,
		anchorIndexAfterBlockMoveUp,
		assignJobGroupToRange,
		buildJobTreeSegments,
		contiguousJobGroupRun,
		DEFAULT_LASER_GROUP_ID,
		moveEntityBlockDown,
		moveEntityBlockUp,
		nextJobGroupId,
		nextJobGroupLabel,
		remapIndexAfterBlockMoveDown,
		remapIndexAfterBlockMoveUp,
		reorderSelectedIntoContiguousBlock,
		selectionIndicesAfterBlockMoveDown,
		selectionIndicesAfterBlockMoveUp,
		ungroupJobGroupId,
		type LaserGroupV1,
		type SceneEntity
	} from '$lib/scene/sceneV1';

	let {
		entities = $bindable<SceneEntity[]>([]),
		selectedIndices = $bindable<number[]>([]),
		anchorIndex = $bindable<number | null>(null),
		lastClickedIndex = $bindable<number | null>(null),
		laserGroups = [] as LaserGroupV1[]
	} = $props();

	const segments = $derived(buildJobTreeSegments(entities));

	function isSelected(i: number): boolean {
		return selectedIndices.includes(i);
	}

	function handleRowClick(i: number, ev: MouseEvent) {
		const out = applyShiftSelection(entities.length, i, ev.shiftKey, anchorIndex, lastClickedIndex);
		selectedIndices = out.selectedIndices;
		anchorIndex = out.anchorIndex;
		lastClickedIndex = out.lastClickedIndex;
	}

	function selectGroupIndices(indices: number[]) {
		selectedIndices = [...indices];
		anchorIndex = indices[0] ?? null;
		lastClickedIndex = indices[indices.length - 1] ?? null;
	}

	function setEntityGroup(i: number, gid: string) {
		entities = entities.map((e, j) =>
			j === i ? { ...e, laser_group_id: gid } : e
		) as SceneEntity[];
	}

	function label(i: number, e: SceneEntity): string {
		if (e.entity_label) return e.entity_label;
		if (e.type === 'line') return `Line ${i + 1}`;
		return `Rect ${i + 1}`;
	}

	function summary(e: SceneEntity): string {
		if (e.type === 'line') {
			return `${e.x0.toFixed(0)},${e.y0.toFixed(0)} → ${e.x1.toFixed(0)},${e.y1.toFixed(0)} mm`;
		}
		const rot = e.rotation_deg ?? 0;
		const rotS = Math.abs(rot) > 0.01 ? ` ∠${rot.toFixed(0)}°` : '';
		return `${e.width.toFixed(0)}×${e.height.toFixed(0)} @ ${e.x.toFixed(0)},${e.y.toFixed(0)} mm${rotS}`;
	}

	function effectiveBlockForMove(i: number): { start: number; length: number } {
		const run = contiguousJobGroupRun(entities, i);
		if (run) return { start: run.start, length: run.length };
		return { start: i, length: 1 };
	}

	/**
	 * Move is allowed when the selection is exactly one entity, or the full contiguous job-group
	 * block (one member selected moves the whole block; header multi-select is all indices of the group).
	 */
	function isSelectionExactlyBlockAt(i: number): boolean {
		const run = contiguousJobGroupRun(entities, i);
		if (!run) {
			return selectedIndices.length === 1 && selectedIndices[0] === i;
		}
		const block = Array.from({ length: run.length }, (_, k) => run.start + k);
		const sel = new Set(selectedIndices);
		if (selectedIndices.length === 1 && selectedIndices[0] === i) {
			return block.includes(i);
		}
		if (selectedIndices.length === block.length && block.every((idx) => sel.has(idx))) {
			return true;
		}
		return false;
	}

	/** True when the group folder row should show move actions (same indices as segment). */
	function groupFullySelected(seg: { indices: number[] }): boolean {
		if (selectedIndices.length !== seg.indices.length) return false;
		const sel = new Set(selectedIndices);
		return seg.indices.every((idx) => sel.has(idx));
	}

	/** Avoid duplicate ↑↓ on every nested row when the whole group is selected — use folder or first row only. */
	function showNestedRowActions(i: number): boolean {
		if (!isSelectionExactlyBlockAt(i)) return false;
		const run = contiguousJobGroupRun(entities, i);
		if (run && selectedIndices.length > 1) {
			return i === run.start;
		}
		return true;
	}

	function moveUp(i: number) {
		if (!isSelectionExactlyBlockAt(i)) return;
		const { start, length } = effectiveBlockForMove(i);
		if (start <= 0) return;
		entities = moveEntityBlockUp(entities, start, length);
		selectedIndices = selectionIndicesAfterBlockMoveUp(start, length, selectedIndices);
		anchorIndex =
			anchorIndex !== null ? anchorIndexAfterBlockMoveUp(start, length, anchorIndex) : null;
		lastClickedIndex =
			lastClickedIndex !== null
				? remapIndexAfterBlockMoveUp(start, length, lastClickedIndex)
				: null;
	}

	function moveDown(i: number) {
		if (!isSelectionExactlyBlockAt(i)) return;
		const { start, length } = effectiveBlockForMove(i);
		if (start + length >= entities.length) return;
		const n = entities.length;
		entities = moveEntityBlockDown(entities, start, length);
		selectedIndices = selectionIndicesAfterBlockMoveDown(start, length, n, selectedIndices);
		anchorIndex =
			anchorIndex !== null ? anchorIndexAfterBlockMoveDown(start, length, n, anchorIndex) : null;
		lastClickedIndex =
			lastClickedIndex !== null
				? remapIndexAfterBlockMoveDown(start, length, n, lastClickedIndex)
				: null;
	}

	function removeAllSelected() {
		const toRemove = new Set(selectedIndices);
		entities = entities.filter((_, j) => !toRemove.has(j));
		selectedIndices = [];
		anchorIndex = null;
		lastClickedIndex = null;
	}

	function removeAt(i: number) {
		if (selectedIndices.length > 1) {
			removeAllSelected();
			return;
		}
		const gid = entities[i]?.job_group_id;
		entities = entities.filter((_, j) => j !== i);
		if (gid) {
			const run = contiguousJobGroupRun(entities, Math.min(i, entities.length - 1));
			if (run && run.length < 2) {
				entities = ungroupJobGroupId(entities, gid);
			}
		}
		selectedIndices = [];
		anchorIndex = null;
		lastClickedIndex = null;
	}

	function groupSelectedEntities() {
		const out = reorderSelectedIntoContiguousBlock(entities, selectedIndices);
		if (!out) return;
		const id = nextJobGroupId();
		const lab = nextJobGroupLabel(out.merged);
		let next = assignJobGroupToRange(
			out.merged,
			out.blockStart,
			out.blockLength,
			id,
			lab
		);
		entities = next;
		const sel = Array.from(
			{ length: out.blockLength },
			(_, k) => out.blockStart + k
		);
		selectedIndices = sel;
		anchorIndex = sel[0] ?? null;
		lastClickedIndex = sel[sel.length - 1] ?? null;
	}

	const canUngroup = $derived.by(() => {
		if (selectedIndices.length === 0) return false;
		const ids = new Set<string>();
		for (const i of selectedIndices) {
			const id = entities[i]?.job_group_id;
			if (id) ids.add(id);
		}
		return ids.size > 0;
	});

	function ungroupSelection() {
		const ids = new Set<string>();
		for (const i of selectedIndices) {
			const id = entities[i]?.job_group_id;
			if (id) ids.add(id);
		}
		let next = entities;
		for (const id of ids) {
			next = ungroupJobGroupId(next, id);
		}
		entities = next;
	}
</script>

<div class="ldk-card ldk-job-tree" data-testid="editor-job-tree">
	<h3 class="ldk-tree-title">Job tree</h3>
	<p class="ldk-muted" style="margin:0 0 0.75rem;font-size:0.82rem">
		Execution order is top → bottom. <strong>Shift+click</strong> for a range; select a row for preset and details;
		<strong>↑</strong> / <strong>↓</strong> / remove for the selected row or <strong>whole job group</strong> (folder or one member).
		<strong>Group</strong> when several are selected.
		<strong>Esc</strong> clears selection.
	</p>
	{#if selectedIndices.length > 1}
		<div class="ldk-tree-bulk">
			<button
				type="button"
				class="ldk-btn secondary"
				data-testid="editor-tree-group"
				onclick={() => groupSelectedEntities()}>Group selection</button
			>
			{#if canUngroup}
				<button
					type="button"
					class="ldk-btn secondary"
					data-testid="editor-tree-ungroup"
					onclick={() => ungroupSelection()}>Ungroup</button
				>
			{/if}
			<button
				type="button"
				class="ldk-btn secondary danger"
				data-testid="editor-tree-remove-bulk"
				onclick={() => removeAllSelected()}>Remove {selectedIndices.length} selected</button
			>
		</div>
	{:else if selectedIndices.length === 1 && canUngroup}
		<div class="ldk-tree-bulk">
			<button
				type="button"
				class="ldk-btn secondary"
				data-testid="editor-tree-ungroup-single"
				onclick={() => ungroupSelection()}>Ungroup</button
			>
		</div>
	{/if}
	{#if entities.length === 0}
		<p class="ldk-muted" data-testid="editor-job-tree-empty">No items</p>
	{:else}
		<ul class="ldk-tree-list" role="tree">
			{#each segments as seg (seg.kind === 'group' ? 'g-' + seg.id : 'e-' + seg.index)}
				{#if seg.kind === 'group'}
					<li class="ldk-tree-folder" role="presentation">
						<div class="ldk-tree-folder-header">
							<button
								type="button"
								class="ldk-tree-folder-title"
								data-testid="editor-job-tree-group-header"
								onclick={() => selectGroupIndices(seg.indices)}
								title="{seg.indices.length} entities"
							>
								<span class="ldk-tree-folder-chev">▾</span>
								<span class="ldk-tree-name">{seg.label}</span>
								<span class="ldk-tree-sum">{seg.indices.length} items</span>
							</button>
							{#if groupFullySelected(seg)}
								<div class="ldk-tree-actions">
									<button
										type="button"
										class="ldk-btn secondary ldk-tree-icon"
										disabled={effectiveBlockForMove(seg.indices[0]!).start <= 0}
										data-testid="editor-tree-group-up"
										onclick={() => moveUp(seg.indices[0]!)}
										title="Move group up">↑</button
									>
									<button
										type="button"
										class="ldk-btn secondary ldk-tree-icon"
										disabled={effectiveBlockForMove(seg.indices[0]!).start +
											effectiveBlockForMove(seg.indices[0]!).length >=
											entities.length}
										data-testid="editor-tree-group-down"
										onclick={() => moveDown(seg.indices[0]!)}
										title="Move group down">↓</button
									>
								</div>
							{/if}
						</div>
						<ul class="ldk-tree-nested" role="group">
							{#each seg.indices as i (i)}
								{@const e = entities[i]!}
								{@const expanded = isSelected(i)}
								<li
									class="ldk-tree-row ldk-tree-row-nested"
									class:ldk-tree-row-collapsed={!expanded}
									class:ldk-tree-sel={expanded}
									role="treeitem"
									aria-selected={expanded}
									aria-expanded={expanded}
									data-testid="editor-job-tree-item"
									data-tree-index={i}
								>
									<div class="ldk-tree-main">
										<button
											type="button"
											class="ldk-tree-label"
											class:ldk-tree-label-compact={!expanded}
											onclick={(ev) => handleRowClick(i, ev)}
											title={summary(e)}
										>
											<span class="ldk-tree-name">{label(i, e)}</span>
											{#if expanded}
												<span class="ldk-tree-sum">{summary(e)}</span>
											{/if}
										</button>
										{#if expanded}
											<label class="ldk-tree-group-wrap">
												<span class="ldk-tree-group-h">Preset</span>
												<select
													class="ldk-tree-group"
													value={e.laser_group_id ?? DEFAULT_LASER_GROUP_ID}
													data-testid="editor-tree-laser-group"
													data-tree-index={i}
													onclick={(ev) => ev.stopPropagation()}
													onchange={(ev) =>
														setEntityGroup(i, (ev.currentTarget as HTMLSelectElement).value)}
												>
													{#each laserGroups as g (g.id)}
														<option value={g.id}>{g.name}</option>
													{/each}
												</select>
											</label>
											{#if e.laser}
												<span class="ldk-tree-custom" title="Custom laser override">Custom laser</span>
											{/if}
										{/if}
									</div>
									{#if expanded && showNestedRowActions(i)}
										<div class="ldk-tree-actions">
											<button
												type="button"
												class="ldk-btn secondary ldk-tree-icon"
												disabled={effectiveBlockForMove(i).start <= 0}
												data-testid="editor-tree-up"
												data-tree-index={i}
												onclick={() => moveUp(i)}
												title="Move up">↑</button
											>
											<button
												type="button"
												class="ldk-btn secondary ldk-tree-icon"
												disabled={effectiveBlockForMove(i).start + effectiveBlockForMove(i).length >=
													entities.length}
												data-testid="editor-tree-down"
												data-tree-index={i}
												onclick={() => moveDown(i)}
												title="Move down">↓</button
											>
											<button
												type="button"
												class="ldk-btn secondary ldk-tree-icon danger"
												data-testid="editor-tree-remove"
												data-tree-index={i}
												onclick={() => removeAt(i)}
												title="Remove">×</button
											>
										</div>
									{/if}
								</li>
							{/each}
						</ul>
					</li>
				{:else}
					{@const i = seg.index}
					{@const e = entities[i]!}
					{@const expanded = isSelected(i)}
					<li
						class="ldk-tree-row"
						class:ldk-tree-row-collapsed={!expanded}
						class:ldk-tree-sel={expanded}
						role="treeitem"
						aria-selected={expanded}
						aria-expanded={expanded}
						data-testid="editor-job-tree-item"
						data-tree-index={i}
					>
						<div class="ldk-tree-main">
							<button
								type="button"
								class="ldk-tree-label"
								class:ldk-tree-label-compact={!expanded}
								onclick={(ev) => handleRowClick(i, ev)}
								title={summary(e)}
							>
								<span class="ldk-tree-name">{label(i, e)}</span>
								{#if expanded}
									<span class="ldk-tree-sum">{summary(e)}</span>
								{/if}
							</button>
							{#if expanded}
								<label class="ldk-tree-group-wrap">
									<span class="ldk-tree-group-h">Preset</span>
									<select
										class="ldk-tree-group"
										value={e.laser_group_id ?? DEFAULT_LASER_GROUP_ID}
										data-testid="editor-tree-laser-group"
										data-tree-index={i}
										onclick={(ev) => ev.stopPropagation()}
										onchange={(ev) =>
											setEntityGroup(i, (ev.currentTarget as HTMLSelectElement).value)}
									>
										{#each laserGroups as g (g.id)}
											<option value={g.id}>{g.name}</option>
										{/each}
									</select>
								</label>
								{#if e.laser}
									<span class="ldk-tree-custom" title="Custom laser override">Custom laser</span>
								{/if}
							{/if}
						</div>
						{#if expanded && isSelectionExactlyBlockAt(i)}
							<div class="ldk-tree-actions">
								<button
									type="button"
									class="ldk-btn secondary ldk-tree-icon"
									disabled={effectiveBlockForMove(i).start <= 0}
									data-testid="editor-tree-up"
									data-tree-index={i}
									onclick={() => moveUp(i)}
									title="Move up">↑</button
								>
								<button
									type="button"
									class="ldk-btn secondary ldk-tree-icon"
									disabled={effectiveBlockForMove(i).start + effectiveBlockForMove(i).length >=
										entities.length}
									data-testid="editor-tree-down"
									data-tree-index={i}
									onclick={() => moveDown(i)}
									title="Move down">↓</button
								>
								<button
									type="button"
									class="ldk-btn secondary ldk-tree-icon danger"
									data-testid="editor-tree-remove"
									data-tree-index={i}
									onclick={() => removeAt(i)}
									title="Remove">×</button
								>
							</div>
						{/if}
					</li>
				{/if}
			{/each}
		</ul>
	{/if}
</div>

<style>
	.ldk-job-tree {
		min-width: 0;
		max-width: 100%;
	}
	.ldk-tree-bulk {
		margin-bottom: 0.5rem;
		display: flex;
		flex-wrap: wrap;
		gap: 0.4rem;
		align-items: center;
	}
	.ldk-tree-title {
		margin: 0 0 0.5rem;
		font-size: 1rem;
		font-weight: 600;
	}
	.ldk-tree-list {
		list-style: none;
		margin: 0;
		padding: 0;
		border: 1px solid #d8dee6;
		border-radius: 6px;
		overflow: hidden;
		background: #fafbfc;
	}
	.ldk-tree-folder {
		border-bottom: 1px solid #e8ecf0;
	}
	.ldk-tree-folder:last-child {
		border-bottom: none;
	}
	.ldk-tree-folder-header {
		display: flex;
		align-items: stretch;
		gap: 0.25rem;
		background: #eef2f7;
		border-bottom: 1px solid #dde4ee;
		padding: 0.25rem 0.45rem 0.25rem 0.5rem;
	}
	.ldk-tree-folder-header .ldk-tree-folder-title {
		flex: 1;
		min-width: 0;
	}
	.ldk-tree-folder-title {
		display: flex;
		align-items: center;
		gap: 0.4rem;
		width: 100%;
		padding: 0.2rem 0;
		border: none;
		background: transparent;
		cursor: pointer;
		text-align: left;
		font: inherit;
		min-width: 0;
	}
	.ldk-tree-folder-chev {
		color: #64748b;
		font-size: 0.75rem;
		line-height: 1;
	}
	.ldk-tree-nested {
		list-style: none;
		margin: 0;
		padding: 0;
		background: #fafbfc;
	}
	.ldk-tree-row-nested .ldk-tree-main {
		padding-left: 1.1rem;
	}
	.ldk-tree-row {
		display: flex;
		align-items: stretch;
		gap: 0.25rem;
		border-bottom: 1px solid #e8ecf0;
	}
	.ldk-tree-main {
		flex: 1;
		min-width: 0;
		display: flex;
		flex-direction: column;
		gap: 0.35rem;
		padding: 0.35rem 0 0.45rem 0.6rem;
	}
	.ldk-tree-group-wrap {
		display: flex;
		flex-direction: column;
		gap: 0.15rem;
		font-size: 0.72rem;
		color: #64748b;
	}
	.ldk-tree-group-h {
		font-weight: 500;
		color: #475569;
	}
	.ldk-tree-group {
		width: 100%;
		min-width: 0;
		padding: 0.2rem 0.35rem;
		font-size: 0.78rem;
		border: 1px solid #c5ced9;
		border-radius: 4px;
		background: #fff;
	}
	.ldk-tree-custom {
		font-size: 0.72rem;
		color: #b45309;
		font-weight: 600;
	}
	.ldk-tree-row:last-child {
		border-bottom: none;
	}
	.ldk-tree-sel {
		background: #e8f5ee;
	}
	.ldk-tree-label {
		display: flex;
		flex-direction: column;
		align-items: flex-start;
		gap: 0.15rem;
		padding: 0;
		border: none;
		background: transparent;
		cursor: pointer;
		text-align: left;
		font: inherit;
		width: 100%;
		min-width: 0;
	}
	.ldk-tree-label-compact {
		flex-direction: row;
		align-items: center;
		flex-wrap: nowrap;
		padding: 0.15rem 0;
	}
	.ldk-tree-label-compact .ldk-tree-name {
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.ldk-tree-row-collapsed {
		min-height: 0;
	}
	.ldk-tree-row-collapsed .ldk-tree-main {
		padding: 0.12rem 0.45rem 0.12rem 0.45rem;
		gap: 0;
	}
	.ldk-tree-row-collapsed .ldk-tree-name {
		font-size: 0.78rem;
		font-weight: 600;
		line-height: 1.2;
	}
	.ldk-tree-row-collapsed .ldk-tree-label-compact {
		padding: 0.05rem 0;
		min-height: 0;
	}
	.ldk-tree-list {
		font-size: 0.85rem;
	}
	.ldk-tree-name {
		font-weight: 600;
		color: #1e293b;
	}
	.ldk-tree-sum {
		font-size: 0.78rem;
		color: #64748b;
	}
	.ldk-tree-actions {
		display: flex;
		flex-direction: column;
		justify-content: center;
		gap: 0.2rem;
		padding: 0.25rem;
		border-left: 1px solid #e8ecf0;
		background: #fff;
	}
	.ldk-tree-icon {
		min-width: 2rem;
		padding: 0.15rem 0.35rem;
		font-size: 0.85rem;
		line-height: 1.2;
	}
	.ldk-tree-icon.danger:hover {
		border-color: #b91c1c;
		color: #b91c1c;
	}
</style>
