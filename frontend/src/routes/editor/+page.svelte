<script lang="ts">
	import { base } from '$app/paths';
	import EntityLaserPanel from '$lib/components/EntityLaserPanel.svelte';
	import RtcConnectionPanel from '$lib/components/RtcConnectionPanel.svelte';
	import SceneEditor from '$lib/components/SceneEditor.svelte';
	import SceneJobTree from '$lib/components/SceneJobTree.svelte';
	import * as api from '$lib/api/laserdesk';
	import type { DxfJobResponse } from '$lib/api/laserdesk';
	import {
		buildSceneV1,
		DEFAULT_LASER_GROUP_ID,
		defaultLaserGroups,
		type SceneEntity
	} from '$lib/scene/sceneV1';
	import { applyShiftSelection } from '$lib/scene/selection';
	import { postRtcLog } from '$lib/laser/rtcChannel';
	import { onMount } from 'svelte';

	function rtcLog(line: string) {
		postRtcLog(line);
	}

	const initialEntities: SceneEntity[] = [
		{
			type: 'line',
			x0: 20,
			y0: 30,
			z0: 0,
			x1: 120,
			y1: 30,
			z1: 0,
			entity_label: 'Line 1',
			laser_group_id: DEFAULT_LASER_GROUP_ID
		},
		{
			type: 'rect',
			x: 200,
			y: 50,
			width: 80,
			height: 40,
			z: 0,
			rotation_deg: 0,
			entity_label: 'Rect 1',
			laser_group_id: DEFAULT_LASER_GROUP_ID
		}
	];

	let entities = $state<SceneEntity[]>([...initialEntities]);
	let selectedIndices = $state<number[]>([]);
	let anchorIndex = $state<number | null>(null);
	let lastClickedIndex = $state<number | null>(null);
	let laserGroups = $state(defaultLaserGroups());

	let panelEditIndex = $derived.by(() => {
		if (selectedIndices.length === 0) return null;
		if (lastClickedIndex !== null && selectedIndices.includes(lastClickedIndex)) return lastClickedIndex;
		return selectedIndices[0] ?? null;
	});

	function selectAt(i: number, shift: boolean) {
		const out = applyShiftSelection(entities.length, i, shift, anchorIndex, lastClickedIndex);
		selectedIndices = out.selectedIndices;
		anchorIndex = out.anchorIndex;
		lastClickedIndex = out.lastClickedIndex;
	}

	function clearSelection() {
		selectedIndices = [];
		anchorIndex = null;
		lastClickedIndex = null;
	}
	let defaultLaserGroupId = $state(DEFAULT_LASER_GROUP_ID);

	function isFormField(t: EventTarget | null): boolean {
		if (!t || !(t instanceof HTMLElement)) return false;
		const tag = t.tagName;
		return tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT' || t.isContentEditable;
	}

	onMount(() => {
		rtcLog(
			`Scene editor: page ready (${typeof window !== 'undefined' ? window.location.origin : ''}${base}) — use Connect, Submit scene, Load into RTC, Start/Stop.`
		);

		let lastFocusLog = 0;
		const onVis = () => {
			if (document.visibilityState !== 'visible') return;
			const now = Date.now();
			if (now - lastFocusLog < 2500) return;
			lastFocusLog = now;
			rtcLog('Scene editor: tab focused — Connect (mock) or Submit scene if the log is still empty.');
		};
		document.addEventListener('visibilitychange', onVis);

		const onKeyDown = (e: KeyboardEvent) => {
			if (e.key === 'Escape' && !isFormField(e.target)) clearSelection();
		};
		window.addEventListener('keydown', onKeyDown);

		return () => {
			document.removeEventListener('visibilitychange', onVis);
			window.removeEventListener('keydown', onKeyDown);
		};
	});

	let busy = $state(false);
	let err = $state<string | null>(null);
	let hint = $state<string | null>(null);
	let jobId = $state<string | null>(null);
	let job = $state<DxfJobResponse | null>(null);
	let rtcState = $state<string>('—');
	let dxfLineCount = $state<number | null>(null);

	function clearHints() {
		err = null;
		hint = null;
	}

	async function withBusy(fn: () => Promise<void>) {
		clearHints();
		busy = true;
		try {
			await fn();
		} catch (e) {
			err = e instanceof Error ? e.message : String(e);
		} finally {
			busy = false;
		}
	}

	async function refreshRtc() {
		try {
			const s = await api.getRtcStatus();
			rtcState = s.connection_state ?? '—';
			dxfLineCount = s.dxf_line_count ?? null;
		} catch {
			rtcState = '—';
			dxfLineCount = null;
		}
	}

	$effect(() => {
		void refreshRtc();
	});

	async function submitScene() {
		await withBusy(async () => {
			const scene = buildSceneV1({
				entities,
				laserGroups,
				defaultLaserGroupId,
				sourceName: 'editor'
			});
			const r = await api.postJobsScene(scene);
			jobId = r.job_id;
			job = await api.getJobsDxf(r.job_id);
			clearSelection();
			hint = `Parsed ${job.line_count} LINE segments from scene.`;
			rtcLog(`Scene editor: POST /jobs/scene → job_id=${r.job_id}, ${job.line_count} segments`);
		});
	}

	async function loadIntoRtc() {
		const id = jobId;
		if (!id) return;
		await withBusy(async () => {
			await api.postJobsDxfLoad(id);
			await refreshRtc();
			hint = 'Scene job loaded into RTC session.';
			rtcLog('Scene editor: POST /jobs/dxf/…/load');
		});
	}

	async function startRun() {
		const id = jobId;
		if (!id) return;
		await withBusy(async () => {
			await api.postJobsDxfRun(id);
			await refreshRtc();
			hint = 'Execution started.';
			rtcLog('Scene editor: POST /jobs/dxf/…/run');
		});
	}

	async function stopRun() {
		const id = jobId;
		if (!id) return;
		await withBusy(async () => {
			await api.postJobsDxfStop(id);
			await refreshRtc();
			hint = 'Execution stopped.';
			rtcLog('Scene editor: POST /jobs/dxf/…/stop');
		});
	}

	let running = $derived(rtcState === 'running');
	let loaded = $derived(rtcState === 'loaded');
</script>

<svelte:head>
	<title>Scene editor · laserDESK 2026</title>
</svelte:head>

<article class="ldk-doc ldk-editor-page">
	<h1 class="ldk-editor-page-title">Phase H – Scene editor</h1>
	<p class="ldk-muted ldk-editor-lead">
		Draw lines and rectangles (mm, +Y up). Use <strong>presets</strong> (shared laser parameters) or a per-entity
		<strong>override</strong> in the parameter column; order items in the job tree, then submit as
		<code>scene_v1</code>. Backend geometry path:
		<code>/api/v1/jobs/scene</code> → <code>/api/v1/jobs/dxf/&#123;id&#125;</code> (load / run / stop). Optional
		<code>laser</code> block is stored for future RTC use.
	</p>

	{#if err}
		<p class="ldk-error" role="alert" data-testid="editor-page-error">{err}</p>
	{/if}
	{#if hint}
		<p class="ldk-success" role="status" data-testid="editor-page-hint">{hint}</p>
	{/if}

	<RtcConnectionPanel
		onSessionChanged={() => {
			void refreshRtc();
		}}
		onAfterConnect={({ mode: m, host }) => {
			if (m === 'mock') {
				rtcLog('Scene editor: POST /rtc/connect (mock)');
			} else {
				rtcLog(`Scene editor: POST /rtc/connect (ethernet ${host ?? ''})`);
			}
		}}
		onAfterDisconnect={() => {
			jobId = null;
			job = null;
			void refreshRtc();
			rtcLog('Scene editor: POST /rtc/disconnect');
		}}
	/>
	<div class="ldk-row" style="flex-wrap:wrap;margin-bottom:0.45rem">
		<button type="button" class="ldk-btn secondary" disabled={busy} onclick={() => refreshRtc()}>Refresh RTC</button>
	</div>
	<p class="ldk-muted" data-testid="editor-rtc-state">
		RTC: <strong>{rtcState}</strong>
		{#if dxfLineCount != null}
			· DXF segments: <strong>{dxfLineCount}</strong>
		{/if}
	</p>
	<p class="ldk-muted" style="font-size:0.85rem">
		<a href="{base}/rtc">RTC activity log</a> — same host as this page.
	</p>

	<div class="ldk-editor-layout">
		<div class="ldk-editor-sidebar">
			<SceneJobTree
				bind:entities
				bind:selectedIndices
				bind:anchorIndex
				bind:lastClickedIndex
				laserGroups={laserGroups}
			/>
			<EntityLaserPanel
				bind:entities
				bind:laserGroups
				bind:defaultLaserGroupId
				selectedIndex={panelEditIndex}
				selectedIndices={selectedIndices}
				selectionCount={selectedIndices.length}
			/>
		</div>
		<section class="ldk-editor-canvas">
			<h2 class="ldk-editor-canvas-h">Canvas</h2>
			<SceneEditor
				bind:entities
				selectedIndices={selectedIndices}
				onSelectEntity={selectAt}
				onClearSelection={clearSelection}
				defaultLaserGroupId={defaultLaserGroupId}
			/>
			<div class="ldk-row" style="flex-wrap:wrap;margin:0.75rem 0 0">
				<button
					type="button"
					class="ldk-btn"
					disabled={busy}
					data-testid="editor-submit-scene"
					onclick={() => submitScene()}>Submit scene to backend</button
				>
			</div>
		</section>
	</div>

	{#if job}
		<p class="ldk-muted" data-testid="editor-entity-count" style="margin-top:0.6rem">
			<strong>{job.line_count}</strong> LINE entities · source <code>{job.source_name}</code> · job
			<code>{job.job_id}</code>
		</p>

		<div style="overflow:auto;max-height:14rem;border:1px solid #d8dee6;border-radius:6px">
			<table class="ldk-table" data-testid="editor-entity-table">
				<thead>
					<tr>
						<th>#</th>
						<th>Layer</th>
						<th>Length</th>
						<th>From</th>
						<th>To</th>
					</tr>
				</thead>
				<tbody>
					{#each job.entities as e, i (e.index)}
						<tr data-editor-entity-row={i}>
							<td>{e.index}</td>
							<td>{e.layer}</td>
							<td>{e.length.toFixed(3)}</td>
							<td>({e.x0.toFixed(2)}, {e.y0.toFixed(2)})</td>
							<td>({e.x1.toFixed(2)}, {e.y1.toFixed(2)})</td>
						</tr>
					{/each}
				</tbody>
			</table>
		</div>

		<h2 class="ldk-editor-rtc-h">RTC</h2>
		<div class="ldk-row" style="flex-wrap:wrap">
			<button
				type="button"
				class="ldk-btn"
				disabled={busy || !jobId}
				data-testid="editor-load-rtc"
				onclick={() => loadIntoRtc()}>Load into RTC</button
			>
			<button
				type="button"
				class="ldk-btn"
				disabled={busy || !jobId || running || !loaded}
				data-testid="editor-start"
				onclick={() => startRun()}>Start</button
			>
			<button
				type="button"
				class="ldk-btn danger"
				disabled={busy || !jobId || !running}
				data-testid="editor-stop"
				onclick={() => stopRun()}>Stop</button
			>
		</div>
	{/if}
</article>

<style>
	.ldk-editor-page {
		max-width: none;
		width: 100%;
	}
	.ldk-editor-page-title {
		margin: 0;
		font-size: 1.05rem;
		font-weight: 650;
	}
	.ldk-editor-lead {
		margin: 0.25rem 0 0.45rem;
		font-size: 0.875rem;
		line-height: 1.38;
	}
	.ldk-editor-canvas-h {
		font-size: 0.95rem;
		margin: 0 0 0.35rem;
		font-weight: 600;
	}
	.ldk-editor-rtc-h {
		font-size: 0.95rem;
		margin-top: 0.75rem;
		font-weight: 600;
	}
	.ldk-editor-layout {
		display: grid;
		grid-template-columns: minmax(18rem, 38rem) minmax(0, 1fr);
		gap: 0.55rem;
		align-items: start;
		margin-top: 0.35rem;
		width: 100%;
	}
	@media (max-width: 52rem) {
		.ldk-editor-layout {
			grid-template-columns: 1fr;
		}
	}
	.ldk-editor-sidebar {
		display: grid;
		grid-template-columns: minmax(0, 1fr) minmax(11rem, 18rem);
		gap: 0.45rem;
		align-items: start;
		min-width: 0;
	}
	@media (max-width: 52rem) {
		.ldk-editor-sidebar {
			grid-template-columns: 1fr;
		}
	}
	.ldk-editor-canvas {
		min-width: 0;
		max-width: 100%;
		overflow-x: auto;
	}
	.ldk-table {
		width: 100%;
		border-collapse: collapse;
		font-size: 0.85rem;
	}
	.ldk-table th,
	.ldk-table td {
		padding: 0.25rem 0.4rem;
		text-align: left;
		border-bottom: 1px solid #e8ecf0;
	}
	.ldk-table th {
		background: #f4f6f8;
		font-weight: 600;
	}
</style>
