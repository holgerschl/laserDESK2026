<script lang="ts">
	import { base } from '$app/paths';
	import EntityLaserPanel from '$lib/components/EntityLaserPanel.svelte';
	import LaserGroupsPanel from '$lib/components/LaserGroupsPanel.svelte';
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
			laser_group_id: DEFAULT_LASER_GROUP_ID
		}
	];

	let entities = $state<SceneEntity[]>([...initialEntities]);
	let selectedIndex = $state<number | null>(null);
	let laserGroups = $state(defaultLaserGroups());
	let defaultLaserGroupId = $state(DEFAULT_LASER_GROUP_ID);

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
		return () => document.removeEventListener('visibilitychange', onVis);
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
			selectedIndex = null;
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
	<h1 style="margin-top:0">Phase H – Scene editor</h1>
	<p class="ldk-muted" style="margin-top:0">
		Draw lines and rectangles (mm, +Y up), define <strong>laser groups</strong> (shared parameters), assign entities to
		a group or set a <strong>custom laser</strong> per entity, order items in the job tree, then submit as
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
	<div class="ldk-row" style="flex-wrap:wrap;margin-bottom:0.75rem">
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
		<aside class="ldk-editor-aside">
			<SceneJobTree bind:entities bind:selectedIndex laserGroups={laserGroups} />
			<LaserGroupsPanel
				bind:laserGroups
				bind:defaultLaserGroupId
				onBeforeRemoveGroup={(removedId, fallbackId) => {
					entities = entities.map((e) =>
						e.laser_group_id === removedId ? { ...e, laser_group_id: fallbackId } : e
					);
				}}
			/>
			<EntityLaserPanel
				bind:entities
				selectedIndex={selectedIndex}
				laserGroups={laserGroups}
				defaultLaserGroupId={defaultLaserGroupId}
			/>
		</aside>
		<section class="ldk-editor-canvas">
			<h2 style="font-size:1.05rem;margin:0 0 0.5rem">Canvas</h2>
			<SceneEditor bind:entities bind:selectedIndex defaultLaserGroupId={defaultLaserGroupId} />
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
		<p class="ldk-muted" data-testid="editor-entity-count" style="margin-top:1rem">
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

		<h2 style="font-size:1.05rem;margin-top:1.25rem">RTC</h2>
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
		max-width: 72rem;
	}
	.ldk-editor-layout {
		display: grid;
		grid-template-columns: minmax(14rem, 18rem) minmax(0, 1fr);
		gap: 1rem;
		align-items: start;
		margin-top: 0.5rem;
		width: 100%;
	}
	@media (max-width: 52rem) {
		.ldk-editor-layout {
			grid-template-columns: 1fr;
		}
	}
	.ldk-editor-aside {
		display: flex;
		flex-direction: column;
		gap: 0.75rem;
		min-width: 0;
		max-width: 100%;
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
		padding: 0.35rem 0.5rem;
		text-align: left;
		border-bottom: 1px solid #e8ecf0;
	}
	.ldk-table th {
		background: #f4f6f8;
		font-weight: 600;
	}
</style>
