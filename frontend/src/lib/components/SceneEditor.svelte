<script lang="ts">
	import { onDestroy, onMount } from 'svelte';
	import type { Layer } from 'konva/lib/Layer.js';
	import type { Stage } from 'konva/lib/Stage.js';
	import { sceneFromEntities, type SceneEntity, type SceneV1 } from '$lib/scene/sceneV1';

	interface Props {
		stageWidth?: number;
		stageHeight?: number;
	}
	let { stageWidth = 800, stageHeight = 400 }: Props = $props();

	let container: HTMLDivElement | undefined = $state();
	let stage: Stage | null = null;
	let layer: Layer | null = null;
	/** Set after dynamic import — avoids Konva’s Node `canvas` path during SSR/prerender. */
	let konvaLib: typeof import('konva').default | null = null;
	let tool = $state<'select' | 'line' | 'rect'>('select');
	let lineStart = $state<{ x: number; y: number } | null>(null);

	/** World mm, y up; origin bottom-left of stage. */
	let entities = $state<SceneEntity[]>([
		{ type: 'line', x0: 20, y0: 30, z0: 0, x1: 120, y1: 30, z1: 0 },
		{ type: 'rect', x: 200, y: 50, width: 80, height: 40, z: 0 }
	]);

	let history = $state<string[]>([]);
	const maxHist = 40;

	function pushHistory() {
		history = [...history.slice(-(maxHist - 1)), JSON.stringify(entities)];
	}

	function undo() {
		const prev = history.pop();
		history = [...history];
		if (!prev) return;
		try {
			entities = JSON.parse(prev) as SceneEntity[];
			redraw();
		} catch {
			/* ignore */
		}
	}

	function konvaY(wy: number): number {
		return stageHeight - wy;
	}

	function worldFromKonva(x: number, y: number): { x: number; y: number } {
		return { x, y: stageHeight - y };
	}

	function redraw() {
		if (!layer || !konvaLib) return;
		const K = konvaLib;
		layer.destroyChildren();

		const g = new K.Group();
		// light grid
		const step = 50;
		for (let x = 0; x <= stageWidth; x += step) {
			g.add(
				new K.Line({
					points: [x, 0, x, stageHeight],
					stroke: '#e8ecf0',
					strokeWidth: 1,
					listening: false
				})
			);
		}
		for (let y = 0; y <= stageHeight; y += step) {
			g.add(
				new K.Line({
					points: [0, y, stageWidth, y],
					stroke: '#e8ecf0',
					strokeWidth: 1,
					listening: false
				})
			);
		}
		layer.add(g);

		entities.forEach((ent, idx) => {
			if (ent.type === 'line') {
				const ln = new K.Line({
					points: [ent.x0, konvaY(ent.y0), ent.x1, konvaY(ent.y1)],
					stroke: '#1e3a5f',
					strokeWidth: 2,
					lineCap: 'round',
					draggable: tool === 'select',
					hitStrokeWidth: 12
				});
				ln.on('dragend', () => {
					const abs = ln.getAbsoluteTransform();
					const pts = ln.points();
					const p0 = abs.point({ x: pts[0]!, y: pts[1]! });
					const p1 = abs.point({ x: pts[2]!, y: pts[3]! });
					const w0 = worldFromKonva(p0.x, p0.y);
					const w1 = worldFromKonva(p1.x, p1.y);
					pushHistory();
					entities = entities.map((e, i) =>
						i === idx && e.type === 'line'
							? { ...e, x0: w0.x, y0: w0.y, x1: w1.x, y1: w1.y }
							: e
					);
					redraw();
				});
				layer!.add(ln);
			} else {
				const ky = konvaY(ent.y + ent.height);
				const r = new K.Rect({
					x: ent.x,
					y: ky,
					width: ent.width,
					height: ent.height,
					stroke: '#059669',
					strokeWidth: 2,
					fill: 'rgba(5, 150, 105, 0.08)',
					draggable: tool === 'select'
				});
				r.on('dragend', () => {
					const nx = r.x();
					const ny = r.y();
					const w = r.width();
					const h = r.height();
					const bottomLeft = worldFromKonva(nx, ny + h);
					pushHistory();
					entities = entities.map((e, i) =>
						i === idx && e.type === 'rect'
							? { ...e, x: bottomLeft.x, y: bottomLeft.y, width: w, height: h }
							: e
					);
					redraw();
				});
				layer!.add(r);
			}
		});
		layer.batchDraw();
	}

	$effect(() => {
		tool;
		if (layer) redraw();
	});

	onMount(() => {
		if (!container) return;
		void (async () => {
			const K = (await import('konva')).default;
			konvaLib = K;
			stage = new K.Stage({
				container,
				width: stageWidth,
				height: stageHeight
			});
			layer = new K.Layer();
			stage.add(layer);

			stage.on('click', () => {
			if (tool === 'line') {
				const p = stage!.getPointerPosition();
				if (!p) return;
				const w = worldFromKonva(p.x, p.y);
				if (!lineStart) {
					lineStart = { x: w.x, y: w.y };
				} else {
					pushHistory();
					entities = [
						...entities,
						{
							type: 'line',
							x0: lineStart.x,
							y0: lineStart.y,
							z0: 0,
							x1: w.x,
							y1: w.y,
							z1: 0
						}
					];
					lineStart = null;
					redraw();
				}
			} else if (tool === 'rect') {
				const p = stage!.getPointerPosition();
				if (!p) return;
				const w = worldFromKonva(p.x, p.y);
				pushHistory();
				entities = [
					...entities,
					{ type: 'rect', x: w.x, y: w.y, width: 60, height: 40, z: 0 }
				];
				redraw();
			}
			});

			redraw();
		})();
	});

	onDestroy(() => {
		stage?.destroy();
		stage = null;
		layer = null;
		konvaLib = null;
	});

	/** Called from parent via `bind:this` (Phase H). */
	export function getSceneV1(): SceneV1 {
		return sceneFromEntities(entities, 'editor');
	}

	function addDemoLine() {
		pushHistory();
		entities = [
			...entities,
			{ type: 'line', x0: 10, y0: 10, z0: 0, x1: 100, y1: 80, z1: 0 }
		];
		redraw();
	}

	function addDemoRect() {
		pushHistory();
		entities = [...entities, { type: 'rect', x: 300, y: 80, width: 50, height: 35, z: 0 }];
		redraw();
	}
</script>

<div class="ldk-scene-tools">
	<div class="ldk-scene-tool-row">
		<span class="ldk-muted">Tool:</span>
		<button
			type="button"
			class="ldk-btn secondary"
			class:ldk-btn-active={tool === 'select'}
			data-testid="editor-tool-select"
			onclick={() => {
				tool = 'select';
				lineStart = null;
				redraw();
			}}>Select</button
		>
		<button
			type="button"
			class="ldk-btn secondary"
			class:ldk-btn-active={tool === 'line'}
			data-testid="editor-tool-line"
			onclick={() => {
				tool = 'line';
				lineStart = null;
				redraw();
			}}>Line</button
		>
		<button
			type="button"
			class="ldk-btn secondary"
			class:ldk-btn-active={tool === 'rect'}
			data-testid="editor-tool-rect"
			onclick={() => {
				tool = 'rect';
				lineStart = null;
				redraw();
			}}>Rect</button
		>
		<button type="button" class="ldk-btn secondary" data-testid="editor-undo" onclick={() => undo()}
			>Undo</button
		>
	</div>
	<p class="ldk-muted" style="margin:0.35rem 0 0;font-size:0.85rem">
		<strong>Select</strong>: drag shapes. <strong>Line</strong>: two clicks. <strong>Rect</strong>: one click places a
		60×40 mm rectangle. Units: mm; +Y is up (matches DXF preview).
	</p>
	<div class="ldk-scene-stage-wrap" data-testid="editor-stage-wrap">
		<div bind:this={container} class="konva-host"></div>
	</div>
	<div class="ldk-scene-tool-row" style="margin-top:0.5rem">
		<button type="button" class="ldk-btn secondary" data-testid="editor-add-line" onclick={() => addDemoLine()}
			>Add line</button
		>
		<button type="button" class="ldk-btn secondary" data-testid="editor-add-rect" onclick={() => addDemoRect()}
			>Add rect</button
		>
	</div>
</div>

<style>
	.konva-host {
		border: 1px solid #d8dee6;
		border-radius: 6px;
		background: #fafbfc;
	}
	.ldk-scene-stage-wrap {
		margin-top: 0.5rem;
	}
	.ldk-scene-tools .ldk-btn-active {
		border-color: #246;
		background: #e8eef5;
		font-weight: 600;
	}
	.ldk-scene-tool-row {
		display: flex;
		flex-wrap: wrap;
		align-items: center;
		gap: 0.35rem;
	}
</style>
