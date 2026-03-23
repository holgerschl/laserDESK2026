/** scene_v1 JSON — mirrors backend `scene_v1_to_parse_result` (docs: Phase H). */

import type { LaserPropertiesV1 } from './laserProperties';

export type SceneEntity =
	| {
			type: 'line';
			x0: number;
			y0: number;
			z0: number;
			x1: number;
			y1: number;
			z1: number;
	  }
	| {
			type: 'rect';
			x: number;
			y: number;
			width: number;
			height: number;
			/** Counter-clockwise degrees around rect center (+X right, +Y up). */
			rotation_deg?: number;
			z?: number;
	  };

export interface SceneLayerV1 {
	id?: string;
	name?: string;
	entities: SceneEntity[];
}

export interface SceneV1 {
	schemaVersion: 1;
	kind: 'scene_v1';
	source_name?: string;
	/** Optional process parameters (frontend + future RTC); backend parser ignores this block. */
	laser?: LaserPropertiesV1;
	layers: SceneLayerV1[];
}

export function emptySceneV1(sourceName = 'editor'): SceneV1 {
	return {
		schemaVersion: 1,
		kind: 'scene_v1',
		source_name: sourceName,
		layers: [{ id: '0', name: '0', entities: [] }]
	};
}

export function sceneFromEntities(
	entities: SceneEntity[],
	sourceName = 'editor',
	laser?: LaserPropertiesV1
): SceneV1 {
	const scene: SceneV1 = {
		schemaVersion: 1,
		kind: 'scene_v1',
		source_name: sourceName,
		layers: [{ id: '0', name: '0', entities: [...entities] }]
	};
	if (laser) scene.laser = laser;
	return scene;
}

/** Reorder one item from `from` index to `to` index (0-based). */
export function reorderEntities<T>(list: T[], from: number, to: number): T[] {
	if (from === to || from < 0 || to < 0 || from >= list.length || to >= list.length) return list;
	const next = [...list];
	const [item] = next.splice(from, 1);
	next.splice(to, 0, item!);
	return next;
}

/** Keep tree/canvas selection coherent after a single reorder. */
export function selectionAfterReorder(from: number, to: number, sel: number | null): number | null {
	if (sel === null) return null;
	if (sel === from) return to;
	if (from < to) {
		if (sel > from && sel <= to) return sel - 1;
	} else if (from > to) {
		if (sel >= to && sel < from) return sel + 1;
	}
	return sel;
}
