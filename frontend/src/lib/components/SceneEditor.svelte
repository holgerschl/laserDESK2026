<script lang="ts">
	import { onDestroy, onMount } from 'svelte';
	import type { Layer } from 'konva/lib/Layer.js';
	import type { Line as KonvaLine } from 'konva/lib/shapes/Line.js';
	import type { Rect as KonvaRect } from 'konva/lib/shapes/Rect.js';
	import type { Shape as KonvaShape } from 'konva/lib/Shape.js';
	import type { Stage } from 'konva/lib/Stage.js';
	import { fixedStageLayout, fmtMmLabel, mmTicks, niceMmStep } from '$lib/scene/mmAxes';
	import {
		DEFAULT_LASER_GROUP_ID,
		nextEntityLabelForKind,
		withLaserGroupId,
		type SceneEntity
	} from '$lib/scene/sceneV1';

	interface Props {
		stageWidth?: number;
		stageHeight?: number;
		entities?: SceneEntity[];
		/** Multi-selection indices (same as job tree). */
		selectedIndices?: number[];
		onSelectEntity?: (index: number, shiftKey: boolean) => void;
		/** Clear selection (e.g. empty canvas click, Esc handled by parent). */
		onClearSelection?: () => void;
		/** New shapes get this `laser_group_id`. */
		defaultLaserGroupId?: string;
	}
	let {
		stageWidth = 800,
		stageHeight = 400,
		entities = $bindable<SceneEntity[]>([
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
		]),
		selectedIndices = [] as number[],
		onSelectEntity,
		onClearSelection,
		defaultLaserGroupId = DEFAULT_LASER_GROUP_ID
	}: Props = $props();

	let container: HTMLDivElement | undefined = $state();
	let stage: Stage | null = null;
	let layer: Layer | null = null;
	let removeWindowKeyListeners: (() => void) | null = null;
	let konvaLib: typeof import('konva').default | null = null;
	let tool = $state<'select' | 'line' | 'rect'>('select');
	/** Rubber-band preview while drag-drawing a line (world mm). */
	let linePlacement = $state<{
		start: { x: number; y: number };
		current: { x: number; y: number };
	} | null>(null);
	/** Rubber-band preview while drag-drawing a rect (world mm). */
	let rectPlacement = $state<{
		start: { x: number; y: number };
		current: { x: number; y: number };
	} | null>(null);
	/** Remove window listeners if drag ends early (e.g. tool switch). */
	let placementPointerCleanup: (() => void) | null = null;

	function clearPlacementPointer() {
		placementPointerCleanup?.();
		placementPointerCleanup = null;
	}

	/** Screen-space pan (px) and uniform zoom for the world mm canvas. */
	let viewPanX = $state(0);
	let viewPanY = $state(0);
	let viewZoom = $state(1);
	const minZoom = 0.08;
	const maxZoom = 24;
	/** Wheel zoom per tick (was ~8%; lower = slower). */
	const wheelZoomStep = 1.025;
	let spaceDown = $state(false);
	let panDragged = $state(false);

	let history = $state<string[]>([]);

	/** Set in `dragstart` when moving multiple selected entities together. */
	let groupDragSnapshot: SceneEntity[] | null = null;

	function applyDragDeltaToSelection(
		snap: SceneEntity[],
		sel: number[],
		dx: number,
		dy: number
	): SceneEntity[] {
		return snap.map((e, i) => {
			if (!sel.includes(i)) return e;
			if (e.type === 'line') {
				return {
					...e,
					x0: e.x0 + dx,
					y0: e.y0 + dy,
					x1: e.x1 + dx,
					y1: e.y1 + dy
				};
			}
			return {
				...e,
				x: e.x + dx,
				y: e.y + dy
			};
		}) as SceneEntity[];
	}

	function isFormField(t: EventTarget | null): boolean {
		if (!t || !(t instanceof HTMLElement)) return false;
		const tag = t.tagName;
		return tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT' || t.isContentEditable;
	}
	const maxHist = 40;

	function pushHistory() {
		history = [...history.slice(-(maxHist - 1)), JSON.stringify(entities)];
	}

	function undo() {
		const prev = history.pop();
		history = [...history];
		if (!prev) return;
		try {
			const parsed = JSON.parse(prev) as SceneEntity[];
			entities = parsed.map((e) => withLaserGroupId(e, defaultLaserGroupId));
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

	/** Stage/canvas pixels → world mm (accounts for viewport pan/zoom). */
	function stageToWorldMm(sx: number, sy: number): { x: number; y: number } {
		const cx = (sx - viewPanX) / viewZoom;
		const cy = (sy - viewPanY) / viewZoom;
		return worldFromKonva(cx, cy);
	}

	function resetView() {
		viewPanX = 0;
		viewPanY = 0;
		viewZoom = 1;
	}

	let editorAxis = $derived.by(() => {
		const layout = fixedStageLayout(stageWidth, stageHeight);
		const previewStroke = Math.max(Math.max(layout.w, layout.h) * 0.004, 0.09);
		const axisStroke = Math.max(previewStroke * 0.7, 0.1);
		const labelSize = Math.max(Math.max(layout.w, layout.h) * 0.034, 0.65);
		const y0 = layout.flip(0);
		const tickStepX = niceMmStep(layout.w);
		const tickStepY = niceMmStep(layout.h);
		const xs = mmTicks(layout.minX, layout.maxX, tickStepX);
		const ysW = mmTicks(layout.viewWorldMinY, layout.viewWorldMaxY, tickStepY);
		const bottomY = layout.minY + layout.h;
		const tickLen = Math.max(Math.max(layout.w, layout.h) * 0.014, 0.22);
		const tickFont = Math.max(Math.max(layout.w, layout.h) * 0.026, 0.5);
		const xAxisInView = y0 >= layout.minY && y0 <= layout.minY + layout.h;
		const yAxisInView = 0 >= layout.minX && 0 <= layout.maxX;
		return {
			layout,
			axisStroke,
			labelSize,
			y0,
			xs,
			ysW,
			bottomY,
			tickLen,
			tickFont,
			xAxisInView,
			yAxisInView
		};
	});

	function worldLineEndpointsFromKonva(line: KonvaLine): {
		x0: number;
		y0: number;
		x1: number;
		y1: number;
	} | null {
		const abs = line.getAbsoluteTransform();
		const pts = line.points();
		if (pts.length < 4) return null;
		const p0 = abs.point({ x: pts[0]!, y: pts[1]! });
		const p1 = abs.point({ x: pts[2]!, y: pts[3]! });
		const w0 = stageToWorldMm(p0.x, p0.y);
		const w1 = stageToWorldMm(p1.x, p1.y);
		return { x0: w0.x, y0: w0.y, x1: w1.x, y1: w1.y };
	}

	function worldRectFromKonva(rect: KonvaRect): {
		x: number;
		y: number;
		width: number;
		height: number;
		rotation_deg: number;
	} {
		let w = rect.width() * rect.scaleX();
		let h = rect.height() * rect.scaleY();
		if (w < 1) w = 1;
		if (h < 1) h = 1;
		const rotDeg = -rect.rotation();
		const nx = rect.x();
		const cyWorld = worldFromKonva(nx, rect.y()).y;
		const x = nx - w / 2;
		const y = cyWorld - h / 2;
		return { x, y, width: w, height: h, rotation_deg: rotDeg };
	}

	function redraw() {
		if (!layer || !konvaLib) return;
		const K = konvaLib;
		layer.destroyChildren();

		const viewport = new K.Group({
			x: viewPanX,
			y: viewPanY,
			scaleX: viewZoom,
			scaleY: viewZoom,
			listening: true
		});
		layer.add(viewport);

		const drawn: KonvaShape[] = [];

		entities.forEach((ent, idx) => {
			const sel = selectedIndices.includes(idx);
			const stroke = sel ? '#b45309' : ent.type === 'line' ? '#1e3a5f' : '#059669';
			const strokeW = sel ? 3.5 : 2;
			/** In select mode, all shapes stay draggable (move). Selected items also use the Transformer for resize/rotate. */
			const canDragWhole = tool === 'select';
			const shapeListening = tool === 'select';

			if (ent.type === 'line') {
				const ln = new K.Line({
					points: [ent.x0, konvaY(ent.y0), ent.x1, konvaY(ent.y1)],
					stroke,
					strokeWidth: strokeW,
					lineCap: 'round',
					draggable: canDragWhole,
					listening: shapeListening,
					hitStrokeWidth: 12
				});
				viewport.add(ln);
				drawn[idx] = ln;
			} else {
				const rot = ent.rotation_deg ?? 0;
				const cx = ent.x + ent.width / 2;
				const cy = ent.y + ent.height / 2;
				const r = new K.Rect({
					x: cx,
					y: konvaY(cy),
					offsetX: ent.width / 2,
					offsetY: ent.height / 2,
					width: ent.width,
					height: ent.height,
					rotation: -rot,
					stroke,
					strokeWidth: strokeW,
					fill: sel ? 'rgba(180, 83, 9, 0.12)' : 'rgba(5, 150, 105, 0.08)',
					draggable: canDragWhole,
					listening: shapeListening
				});
				viewport.add(r);
				drawn[idx] = r;
			}
		});

		if (linePlacement) {
			const { start, current } = linePlacement;
			const rubber = new K.Line({
				points: [start.x, konvaY(start.y), current.x, konvaY(current.y)],
				stroke: '#64748b',
				strokeWidth: 1.5,
				dash: [6, 4],
				lineCap: 'round',
				listening: false
			});
			viewport.add(rubber);
		}
		if (rectPlacement) {
			const { start, current } = rectPlacement;
			const rx = Math.min(start.x, current.x);
			const ry = Math.min(start.y, current.y);
			const rw = Math.abs(current.x - start.x);
			const rh = Math.abs(current.y - start.y);
			const rcx = rx + rw / 2;
			const rcy = ry + rh / 2;
			const pr = new K.Rect({
				x: rcx,
				y: konvaY(rcy),
				offsetX: rw / 2,
				offsetY: rh / 2,
				width: rw,
				height: rh,
				stroke: '#64748b',
				strokeWidth: 1.5,
				dash: [6, 4],
				fill: 'rgba(100, 116, 139, 0.1)',
				listening: false
			});
			viewport.add(pr);
		}

		function syncGroupDragPeersFromLine(ln: KonvaLine, idx: number) {
			const snap = groupDragSnapshot;
			if (!snap || selectedIndices.length <= 1 || !selectedIndices.includes(idx)) return;
			const old = snap[idx];
			if (old.type !== 'line') return;
			const abs = ln.getAbsoluteTransform();
			const pts = ln.points();
			const p0 = abs.point({ x: pts[0]!, y: pts[1]! });
			const p1 = abs.point({ x: pts[2]!, y: pts[3]! });
			const w0 = stageToWorldMm(p0.x, p0.y);
			const w1 = stageToWorldMm(p1.x, p1.y);
			const oldMidX = (old.x0 + old.x1) / 2;
			const oldMidY = (old.y0 + old.y1) / 2;
			const newMidX = (w0.x + w1.x) / 2;
			const newMidY = (w0.y + w1.y) / 2;
			const dx = newMidX - oldMidX;
			const dy = newMidY - oldMidY;
			for (const j of selectedIndices) {
				if (j === idx) continue;
				const node = drawn[j];
				if (!node) continue;
				const oe = snap[j];
				if (oe.type === 'line') {
					(node as KonvaLine).points([
						oe.x0 + dx,
						konvaY(oe.y0 + dy),
						oe.x1 + dx,
						konvaY(oe.y1 + dy)
					]);
				} else {
					const rr = node as KonvaRect;
					const cx = oe.x + oe.width / 2 + dx;
					const cy = oe.y + oe.height / 2 + dy;
					rr.x(cx);
					rr.y(konvaY(cy));
				}
			}
		}

		function syncGroupDragPeersFromRect(r: KonvaRect, idx: number) {
			const snap = groupDragSnapshot;
			if (!snap || selectedIndices.length <= 1 || !selectedIndices.includes(idx)) return;
			const old = snap[idx];
			if (old.type !== 'rect') return;
			const nx = r.x();
			const ny = r.y();
			const w = r.width();
			const h = r.height();
			const cyWorld = worldFromKonva(nx, ny).y;
			const x = nx - w / 2;
			const y = cyWorld - h / 2;
			const oldCx = old.x + old.width / 2;
			const oldCy = old.y + old.height / 2;
			const newCx = x + w / 2;
			const newCy = y + h / 2;
			const dx = newCx - oldCx;
			const dy = newCy - oldCy;
			for (const j of selectedIndices) {
				if (j === idx) continue;
				const node = drawn[j];
				if (!node) continue;
				const oe = snap[j];
				if (oe.type === 'line') {
					(node as KonvaLine).points([
						oe.x0 + dx,
						konvaY(oe.y0 + dy),
						oe.x1 + dx,
						konvaY(oe.y1 + dy)
					]);
				} else {
					const rr = node as KonvaRect;
					const cx = oe.x + oe.width / 2 + dx;
					const cy = oe.y + oe.height / 2 + dy;
					rr.x(cx);
					rr.y(konvaY(cy));
				}
			}
		}

		entities.forEach((ent, idx) => {
			if (ent.type === 'line') {
				const ln = drawn[idx] as KonvaLine;
				ln.on('click', (e) => {
					e.cancelBubble = true;
					onSelectEntity?.(idx, e.evt.shiftKey);
				});
				ln.on('dragstart', () => {
					if (tool !== 'select') return;
					if (selectedIndices.length > 1 && selectedIndices.includes(idx)) {
						groupDragSnapshot = JSON.parse(JSON.stringify(entities)) as SceneEntity[];
					} else {
						groupDragSnapshot = null;
					}
				});
				ln.on('dragmove', () => syncGroupDragPeersFromLine(ln, idx));
				ln.on('dragend', () => {
					const abs = ln.getAbsoluteTransform();
					const pts = ln.points();
					const p0 = abs.point({ x: pts[0]!, y: pts[1]! });
					const p1 = abs.point({ x: pts[2]!, y: pts[3]! });
					const w0 = stageToWorldMm(p0.x, p0.y);
					const w1 = stageToWorldMm(p1.x, p1.y);

					if (groupDragSnapshot && selectedIndices.length > 1 && selectedIndices.includes(idx)) {
						const old = groupDragSnapshot[idx];
						if (old.type !== 'line') {
							groupDragSnapshot = null;
							return;
						}
						const oldMidX = (old.x0 + old.x1) / 2;
						const oldMidY = (old.y0 + old.y1) / 2;
						const newMidX = (w0.x + w1.x) / 2;
						const newMidY = (w0.y + w1.y) / 2;
						const dx = newMidX - oldMidX;
						const dy = newMidY - oldMidY;
						pushHistory();
						entities = applyDragDeltaToSelection(
							groupDragSnapshot,
							selectedIndices,
							dx,
							dy
						);
						groupDragSnapshot = null;
						return;
					}
					groupDragSnapshot = null;
					pushHistory();
					entities = entities.map((e, i) =>
						i === idx && e.type === 'line'
							? { ...e, x0: w0.x, y0: w0.y, x1: w1.x, y1: w1.y }
							: e
					);
				});
			} else {
				const r = drawn[idx] as KonvaRect;
				r.on('click', (e) => {
					e.cancelBubble = true;
					onSelectEntity?.(idx, e.evt.shiftKey);
				});
				r.on('dragstart', () => {
					if (tool !== 'select') return;
					if (selectedIndices.length > 1 && selectedIndices.includes(idx)) {
						groupDragSnapshot = JSON.parse(JSON.stringify(entities)) as SceneEntity[];
					} else {
						groupDragSnapshot = null;
					}
				});
				r.on('dragmove', () => syncGroupDragPeersFromRect(r, idx));
				r.on('dragend', () => {
					const nx = r.x();
					const ny = r.y();
					const w = r.width();
					const h = r.height();
					const rotR = -r.rotation();
					const cyWorld = worldFromKonva(nx, ny).y;
					const x = nx - w / 2;
					const y = cyWorld - h / 2;

					if (groupDragSnapshot && selectedIndices.length > 1 && selectedIndices.includes(idx)) {
						const old = groupDragSnapshot[idx];
						if (old.type !== 'rect') {
							groupDragSnapshot = null;
							return;
						}
						const oldCx = old.x + old.width / 2;
						const oldCy = old.y + old.height / 2;
						const newCx = x + w / 2;
						const newCy = y + h / 2;
						const dx = newCx - oldCx;
						const dy = newCy - oldCy;
						pushHistory();
						entities = applyDragDeltaToSelection(
							groupDragSnapshot,
							selectedIndices,
							dx,
							dy
						);
						groupDragSnapshot = null;
						return;
					}
					groupDragSnapshot = null;
					pushHistory();
					entities = entities.map((e, i) =>
						i === idx && e.type === 'rect'
							? { ...e, x, y, width: w, height: h, rotation_deg: rotR }
							: e
					);
				});
			}
		});

		function commitSelectionTransformFromDrawn() {
			pushHistory();
			entities = entities.map((e, i) => {
				if (!selectedIndices.includes(i)) return e;
				const n = drawn[i];
				if (!n) return e;
				if (e.type === 'line' && n.getClassName() === 'Line') {
					const pts = worldLineEndpointsFromKonva(n as KonvaLine);
					if (!pts) return e;
					return { ...e, ...pts };
				}
				if (e.type === 'rect' && n.getClassName() === 'Rect') {
					return { ...e, ...worldRectFromKonva(n as KonvaRect) };
				}
				return e;
			}) as SceneEntity[];
		}

		if (tool === 'select' && selectedIndices.length >= 1) {
			const nodes = selectedIndices
				.map((i) => drawn[i])
				.filter((n): n is KonvaShape => Boolean(n));
			if (nodes.length === selectedIndices.length) {
				const tr = new K.Transformer({
					nodes,
					rotateEnabled: true,
					resizeEnabled: true,
					borderStroke: '#246',
					borderStrokeWidth: 1,
					anchorFill: '#fff',
					anchorStroke: '#246',
					boundBoxFunc: (_old, nbox) => {
						if (selectedIndices.length === 1) {
							const only = drawn[selectedIndices[0]!];
							if (only?.getClassName() === 'Line') return nbox;
						}
						if (nbox.width < 4 || nbox.height < 4) return _old;
						return nbox;
					}
				});
				tr.on('transformend', () => {
					commitSelectionTransformFromDrawn();
				});
				viewport.add(tr);
			}
		}

		layer.batchDraw();
	}

	$effect(() => {
		tool;
		selectedIndices;
		entities;
		viewPanX;
		viewPanY;
		viewZoom;
		linePlacement;
		rectPlacement;
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

			let panning = false;
			let lastPan: { x: number; y: number } | null = null;

			const onKeyDown = (e: KeyboardEvent) => {
				if (e.code === 'Space' && !isFormField(e.target)) e.preventDefault();
				if (e.code === 'Space') spaceDown = true;
			};
			const onKeyUp = (e: KeyboardEvent) => {
				if (e.code === 'Space') spaceDown = false;
			};
			window.addEventListener('keydown', onKeyDown);
			window.addEventListener('keyup', onKeyUp);
			removeWindowKeyListeners = () => {
				window.removeEventListener('keydown', onKeyDown);
				window.removeEventListener('keyup', onKeyUp);
			};

			stage.on('wheel', (e) => {
				e.evt.preventDefault();
				const p = stage!.getPointerPosition();
				if (!p) return;
				const factor = e.evt.deltaY > 0 ? 1 / wheelZoomStep : wheelZoomStep;
				const oldZ = viewZoom;
				let newZ = oldZ * factor;
				if (newZ < minZoom) newZ = minZoom;
				if (newZ > maxZoom) newZ = maxZoom;
				const wx = (p.x - viewPanX) / oldZ;
				const ky = (p.y - viewPanY) / oldZ;
				viewPanX = p.x - newZ * wx;
				viewPanY = p.y - newZ * ky;
				viewZoom = newZ;
			});

			function clientToWorldMm(clientX: number, clientY: number) {
				if (!container) return { x: 0, y: 0 };
				const r = container.getBoundingClientRect();
				return stageToWorldMm(clientX - r.left, clientY - r.top);
			}

			function bindLinePlacement(start: { x: number; y: number }) {
				clearPlacementPointer();
				linePlacement = { start, current: start };
				rectPlacement = null;
				const onMove = (ev: MouseEvent) => {
					const w = clientToWorldMm(ev.clientX, ev.clientY);
					linePlacement = { start, current: w };
					redraw();
				};
				const onUp = (ev: MouseEvent) => {
					if (ev.button !== 0) return;
					window.removeEventListener('mousemove', onMove);
					window.removeEventListener('mouseup', onUp);
					placementPointerCleanup = null;
					const pl = linePlacement;
					linePlacement = null;
					redraw();
					if (!pl) return;
					const dx = pl.current.x - pl.start.x;
					const dy = pl.current.y - pl.start.y;
					if (Math.hypot(dx, dy) >= 0.5) {
						pushHistory();
						entities = [
							...entities,
							{
								type: 'line',
								x0: pl.start.x,
								y0: pl.start.y,
								z0: 0,
								x1: pl.current.x,
								y1: pl.current.y,
								z1: 0,
								entity_label: nextEntityLabelForKind('line', entities),
								laser_group_id: defaultLaserGroupId
							}
						];
					}
					redraw();
				};
				window.addEventListener('mousemove', onMove);
				window.addEventListener('mouseup', onUp);
				placementPointerCleanup = () => {
					window.removeEventListener('mousemove', onMove);
					window.removeEventListener('mouseup', onUp);
				};
				redraw();
			}

			function bindRectPlacement(start: { x: number; y: number }) {
				clearPlacementPointer();
				rectPlacement = { start, current: start };
				linePlacement = null;
				const onMove = (ev: MouseEvent) => {
					const w = clientToWorldMm(ev.clientX, ev.clientY);
					rectPlacement = { start, current: w };
					redraw();
				};
				const onUp = (ev: MouseEvent) => {
					if (ev.button !== 0) return;
					window.removeEventListener('mousemove', onMove);
					window.removeEventListener('mouseup', onUp);
					placementPointerCleanup = null;
					const pl = rectPlacement;
					rectPlacement = null;
					redraw();
					if (!pl) return;
					const x0 = Math.min(pl.start.x, pl.current.x);
					const y0 = Math.min(pl.start.y, pl.current.y);
					const rw = Math.abs(pl.current.x - pl.start.x);
					const rh = Math.abs(pl.current.y - pl.start.y);
					if (rw >= 0.5 && rh >= 0.5) {
						pushHistory();
						entities = [
							...entities,
							{
								type: 'rect',
								x: x0,
								y: y0,
								width: rw,
								height: rh,
								z: 0,
								rotation_deg: 0,
								entity_label: nextEntityLabelForKind('rect', entities),
								laser_group_id: defaultLaserGroupId
							}
						];
					}
					redraw();
				};
				window.addEventListener('mousemove', onMove);
				window.addEventListener('mouseup', onUp);
				placementPointerCleanup = () => {
					window.removeEventListener('mousemove', onMove);
					window.removeEventListener('mouseup', onUp);
				};
				redraw();
			}

			stage.on('mousedown', (e) => {
				const wantPan = e.evt.button === 1 || (e.evt.button === 0 && spaceDown);
				if (wantPan) {
					e.evt.preventDefault();
					panning = true;
					panDragged = false;
					const pos = stage!.getPointerPosition();
					lastPan = pos ? { x: pos.x, y: pos.y } : null;
					return;
				}
				if (e.evt.button === 0 && tool === 'line' && !spaceDown) {
					e.evt.preventDefault();
					const w = clientToWorldMm(e.evt.clientX, e.evt.clientY);
					bindLinePlacement(w);
					return;
				}
				if (e.evt.button === 0 && tool === 'rect' && !spaceDown) {
					e.evt.preventDefault();
					const w = clientToWorldMm(e.evt.clientX, e.evt.clientY);
					bindRectPlacement(w);
					return;
				}
			});

			stage.on('mousemove', () => {
				if (!panning || !lastPan) return;
				const p = stage!.getPointerPosition();
				if (!p) return;
				const dx = p.x - lastPan.x;
				const dy = p.y - lastPan.y;
				if (Math.abs(dx) + Math.abs(dy) > 0.5) panDragged = true;
				viewPanX += dx;
				viewPanY += dy;
				lastPan = { x: p.x, y: p.y };
			});

			const endPan = () => {
				panning = false;
				lastPan = null;
			};
			stage.on('mouseup', endPan);
			stage.on('mouseleave', endPan);

			stage.on('click', (e) => {
				if (panDragged) {
					panDragged = false;
					return;
				}
				if (tool === 'select') {
					const t = e.target;
					if (t === stage || t.getClassName?.() === 'Layer') {
						onClearSelection?.();
					}
				}
			});

			redraw();
		})();
	});

	onDestroy(() => {
		placementPointerCleanup?.();
		placementPointerCleanup = null;
		removeWindowKeyListeners?.();
		removeWindowKeyListeners = null;
		stage?.destroy();
		stage = null;
		layer = null;
		konvaLib = null;
	});

	function addDemoLine() {
		pushHistory();
		entities = [
			...entities,
			{
				type: 'line',
				x0: 10,
				y0: 10,
				z0: 0,
				x1: 100,
				y1: 80,
				z1: 0,
				entity_label: nextEntityLabelForKind('line', entities),
				laser_group_id: defaultLaserGroupId
			}
		];
		redraw();
	}

	function addDemoRect() {
		pushHistory();
		entities = [
			...entities,
			{
				type: 'rect',
				x: 300,
				y: 80,
				width: 50,
				height: 35,
				z: 0,
				rotation_deg: 0,
				entity_label: nextEntityLabelForKind('rect', entities),
				laser_group_id: defaultLaserGroupId
			}
		];
		redraw();
	}
</script>

<div class="ldk-scene-tools scene-editor-root">
	<div class="ldk-scene-tool-row">
		<span class="ldk-muted">Tool:</span>
		<button
			type="button"
			class="ldk-btn secondary"
			class:ldk-btn-active={tool === 'select'}
			data-testid="editor-tool-select"
			onclick={() => {
				tool = 'select';
				clearPlacementPointer();
				linePlacement = null;
				rectPlacement = null;
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
				clearPlacementPointer();
				linePlacement = null;
				rectPlacement = null;
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
				clearPlacementPointer();
				linePlacement = null;
				rectPlacement = null;
				redraw();
			}}>Rect</button
		>
		<button type="button" class="ldk-btn secondary" data-testid="editor-undo" onclick={() => undo()}
			>Undo</button
		>
		<button
			type="button"
			class="ldk-btn secondary"
			data-testid="editor-reset-view"
			onclick={() => resetView()}>Reset view</button
		>
	</div>
	<p class="ldk-muted scene-editor-hint">
		<strong>Wheel</strong>: zoom toward cursor. <strong>Space+drag</strong> or <strong>middle mouse</strong>: pan.
		<strong>Select</strong>: click shapes; <strong>Shift+click</strong> extends range (same as job list). Drag to move; multiple selected shapes move together.
		<strong>Selection</strong>: handles rotate/resize (one shape or the whole selection together). <strong>Esc</strong> or empty canvas: clear selection.
		<strong>Line</strong>: click-drag-release for a segment (dashed preview). <strong>Rect</strong>: drag an axis-aligned box (dashed preview). Units: mm; +Y up.
	</p>
	<div
		class="ldk-scene-stage-wrap editor-stage-stack"
		class:editor-space-pan={spaceDown}
		data-testid="editor-stage-wrap"
		style="width:{stageWidth}px;height:{stageHeight}px;"
	>
		<div bind:this={container} class="konva-host"></div>
		<!--
			Same affine as Konva viewport Group: x' = zoom*x + panX, y' = zoom*y + panY (user units = stage px; viewBox 1:1 with explicit width/height).
			Do not use CSS transform on the root <svg> — it can hide the overlay in some browsers. %-sized SVG without fixed px desyncs from Konva; keep width/height = stage.
		-->
		<svg
			class="editor-coords-svg"
			data-testid="editor-coords-svg"
			width={stageWidth}
			height={stageHeight}
			viewBox="{editorAxis.layout.minX} {editorAxis.layout.minY} {editorAxis.layout.w} {editorAxis.layout.h}"
			preserveAspectRatio="xMinYMin meet"
			overflow="visible"
			role="img"
			aria-label="Scene editor axes with millimetre ticks"
		>
			<g
				class="editor-coords"
				pointer-events="none"
				aria-hidden="true"
				transform="matrix({viewZoom} 0 0 {viewZoom} {viewPanX} {viewPanY})"
			>
				<rect
					x={editorAxis.layout.minX}
					y={editorAxis.layout.minY}
					width={editorAxis.layout.w}
					height={editorAxis.layout.h}
					fill="none"
					stroke="#cbd5e1"
					stroke-width={editorAxis.axisStroke}
				/>
				{#each editorAxis.xs as xt (xt)}
					{#if xt >= editorAxis.layout.minX - 1e-9 && xt <= editorAxis.layout.maxX + 1e-9}
						<line
							x1={xt}
							y1={editorAxis.bottomY}
							x2={xt}
							y2={editorAxis.bottomY + editorAxis.tickLen}
							stroke="#94a3b8"
							stroke-width={editorAxis.axisStroke}
						/>
						<text
							x={xt}
							y={editorAxis.bottomY + editorAxis.tickLen + editorAxis.tickFont * 0.92}
							fill="#64748b"
							font-size={editorAxis.tickFont}
							font-family="system-ui, Segoe UI, sans-serif"
							text-anchor="middle">{fmtMmLabel(xt)}</text>
					{/if}
				{/each}
				{#each editorAxis.ysW as yt (yt)}
					{@const ys = editorAxis.layout.flip(yt)}
					{#if ys >= editorAxis.layout.minY - 1e-9 && ys <= editorAxis.bottomY + 1e-9}
						<line
							x1={editorAxis.layout.minX - editorAxis.tickLen}
							y1={ys}
							x2={editorAxis.layout.minX}
							y2={ys}
							stroke="#94a3b8"
							stroke-width={editorAxis.axisStroke}
						/>
						<text
							x={editorAxis.layout.minX - editorAxis.tickLen - editorAxis.tickFont * 0.25}
							y={ys + editorAxis.tickFont * 0.32}
							fill="#64748b"
							font-size={editorAxis.tickFont}
							font-family="system-ui, Segoe UI, sans-serif"
							text-anchor="end">{fmtMmLabel(yt)}</text>
					{/if}
				{/each}
				{#if editorAxis.xAxisInView}
					<line
						x1={editorAxis.layout.minX}
						y1={editorAxis.y0}
						x2={editorAxis.layout.maxX}
						y2={editorAxis.y0}
						stroke="#94a3b8"
						stroke-width={editorAxis.axisStroke}
						stroke-dasharray={`${editorAxis.axisStroke * 3} ${editorAxis.axisStroke * 2}`}
					/>
				{/if}
				{#if editorAxis.yAxisInView}
					<line
						x1="0"
						y1={editorAxis.layout.minY}
						x2="0"
						y2={editorAxis.layout.minY + editorAxis.layout.h}
						stroke="#94a3b8"
						stroke-width={editorAxis.axisStroke}
						stroke-dasharray={`${editorAxis.axisStroke * 3} ${editorAxis.axisStroke * 2}`}
					/>
				{/if}
				{#if editorAxis.xAxisInView && editorAxis.yAxisInView}
					<circle
						cx="0"
						cy={editorAxis.y0}
						r={Math.max(editorAxis.axisStroke * 2.2, 0.35)}
						fill="#64748b"
					/>
				{/if}
				{#if editorAxis.xAxisInView}
					<text
						x={editorAxis.layout.maxX - editorAxis.labelSize * 0.15}
						y={editorAxis.y0 + editorAxis.labelSize * 0.85}
						fill="#64748b"
						font-size={editorAxis.labelSize}
						font-family="system-ui, Segoe UI, sans-serif"
						text-anchor="end"
						font-weight="600">+X</text>
				{/if}
				{#if editorAxis.yAxisInView}
					<text
						x={editorAxis.labelSize * 0.35}
						y={editorAxis.layout.minY + editorAxis.labelSize * 0.95}
						fill="#64748b"
						font-size={editorAxis.labelSize}
						font-family="system-ui, Segoe UI, sans-serif"
						text-anchor="start"
						font-weight="600">+Y</text>
				{/if}
			</g>
		</svg>
	</div>
	<p class="ldk-muted editor-coords-caption">
		Axes and tick labels use <strong>millimetres</strong>; origin at (0, 0); <strong>+Y</strong> points up on screen (same
		convention as the DXF demo).
	</p>
	<div class="ldk-scene-tool-row scene-editor-demo-row">
		<button type="button" class="ldk-btn secondary" data-testid="editor-add-line" onclick={() => addDemoLine()}
			>Add line</button
		>
		<button type="button" class="ldk-btn secondary" data-testid="editor-add-rect" onclick={() => addDemoRect()}
			>Add rect</button
		>
	</div>
</div>

<style>
	.scene-editor-root {
		width: 100%;
		max-width: 100%;
		min-width: 0;
	}
	.ldk-scene-tools {
		display: flex;
		flex-direction: column;
		gap: 0.3rem;
	}
	.scene-editor-hint {
		margin: 0.15rem 0 0;
		font-size: 0.8rem;
		line-height: 1.32;
	}
	.scene-editor-demo-row {
		margin-top: 0.35rem;
	}
	.editor-stage-stack {
		position: relative;
		display: block;
		width: fit-content;
		max-width: 100%;
		margin-top: 0.3rem;
		border: 1px solid #d8dee6;
		border-radius: 6px;
		overflow: auto;
		background: #fafbfc;
	}
	.konva-host {
		position: relative;
		z-index: 0;
		background: #fafbfc;
	}
	.editor-coords-svg {
		position: absolute;
		left: 0;
		top: 0;
		z-index: 1;
		pointer-events: none;
		display: block;
		box-sizing: border-box;
	}
	.editor-stage-stack.editor-space-pan {
		cursor: grab;
	}
	.editor-stage-stack.editor-space-pan:active {
		cursor: grabbing;
	}
	.editor-coords-caption {
		margin: 0.2rem 0 0;
		font-size: 0.78rem;
		line-height: 1.3;
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
		gap: 0.28rem;
	}
</style>
