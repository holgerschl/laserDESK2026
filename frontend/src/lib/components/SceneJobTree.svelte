<script lang="ts">
	import type { SceneEntity } from '$lib/scene/sceneV1';
	import { reorderEntities, selectionAfterReorder } from '$lib/scene/sceneV1';

	let {
		entities = $bindable<SceneEntity[]>([]),
		selectedIndex = $bindable<number | null>(null)
	} = $props();

	function label(i: number, e: SceneEntity): string {
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

	function moveUp(i: number) {
		if (i <= 0) return;
		const from = i;
		const to = i - 1;
		entities = reorderEntities(entities, from, to);
		selectedIndex = selectionAfterReorder(from, to, selectedIndex);
	}

	function moveDown(i: number) {
		if (i >= entities.length - 1) return;
		const from = i;
		const to = i + 1;
		entities = reorderEntities(entities, from, to);
		selectedIndex = selectionAfterReorder(from, to, selectedIndex);
	}

	function removeAt(i: number) {
		entities = entities.filter((_, j) => j !== i);
		if (selectedIndex === i) selectedIndex = null;
		else if (selectedIndex !== null && selectedIndex > i) selectedIndex = selectedIndex - 1;
	}
</script>

<div class="ldk-card ldk-job-tree" data-testid="editor-job-tree">
	<h3 class="ldk-tree-title">Job tree</h3>
	<p class="ldk-muted" style="margin:0 0 0.75rem;font-size:0.82rem">
		Execution order is top → bottom. Use <strong>↑</strong> / <strong>↓</strong> to reorder.
	</p>
	{#if entities.length === 0}
		<p class="ldk-muted" data-testid="editor-job-tree-empty">No items</p>
	{:else}
		<ul class="ldk-tree-list" role="tree">
			{#each entities as e, i (i)}
				<li
					class="ldk-tree-row"
					class:ldk-tree-sel={selectedIndex === i}
					role="treeitem"
					aria-selected={selectedIndex === i}
					data-testid="editor-job-tree-item"
					data-tree-index={i}
				>
					<button
						type="button"
						class="ldk-tree-label"
						onclick={() => (selectedIndex = i)}
						title={summary(e)}
					>
						<span class="ldk-tree-name">{label(i, e)}</span>
						<span class="ldk-tree-sum">{summary(e)}</span>
					</button>
					<div class="ldk-tree-actions">
						<button
							type="button"
							class="ldk-btn secondary ldk-tree-icon"
							disabled={i === 0}
							data-testid="editor-tree-up"
							data-tree-index={i}
							onclick={() => moveUp(i)}
							title="Move up">↑</button
						>
						<button
							type="button"
							class="ldk-btn secondary ldk-tree-icon"
							disabled={i >= entities.length - 1}
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
				</li>
			{/each}
		</ul>
	{/if}
</div>

<style>
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
	.ldk-tree-row {
		display: flex;
		align-items: stretch;
		gap: 0.25rem;
		border-bottom: 1px solid #e8ecf0;
	}
	.ldk-tree-row:last-child {
		border-bottom: none;
	}
	.ldk-tree-sel {
		background: #e8f5ee;
	}
	.ldk-tree-label {
		flex: 1;
		display: flex;
		flex-direction: column;
		align-items: flex-start;
		gap: 0.15rem;
		padding: 0.45rem 0.6rem;
		border: none;
		background: transparent;
		cursor: pointer;
		text-align: left;
		font: inherit;
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
