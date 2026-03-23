/** scene_v1 JSON — mirrors backend `scene_v1_to_parse_result` (docs: Phase H). */

import {
	clampLaserProperties,
	defaultLaserProperties,
	type LaserPropertiesV1
} from './laserProperties';

/** Default group id for new entities and legacy scenes without `laser_group_id`. */
export const DEFAULT_LASER_GROUP_ID = 'g0';

/** Built-in preset ids (same as `laser_group_id` on entities). */
export const CUT_LASER_GROUP_ID = 'g_cut';
export const MARK_LASER_GROUP_ID = 'g_mark';

export interface LaserGroupV1 {
	id: string;
	name: string;
	laser: LaserPropertiesV1;
}

type SceneEntityCommon = {
	/** Share parameters with other entities in the same group. */
	laser_group_id: string;
	/** When set, replaces the group laser for this entity only. */
	laser?: LaserPropertiesV1;
	/** Stable job-tree label (e.g. `Line 2`); set at creation — **not** derived from list order. */
	entity_label?: string;
	/**
	 * Optional job-tree grouping: entities with the same id are shown nested under one folder.
	 * Members must be contiguous in the array; backend ignores these fields.
	 */
	job_group_id?: string;
	job_group_label?: string;
};

export type SceneEntity =
	| (SceneEntityCommon & {
			type: 'line';
			x0: number;
			y0: number;
			z0: number;
			x1: number;
			y1: number;
			z1: number;
	  })
	| (SceneEntityCommon & {
			type: 'rect';
			x: number;
			y: number;
			width: number;
			height: number;
			rotation_deg?: number;
			z?: number;
	  })
	| (SceneEntityCommon & {
			type: 'arc';
			/** Center (world mm). */
			cx: number;
			cy: number;
			cz?: number;
			radius: number;
			/** Degrees from +X toward +Y (CCW). */
			start_angle_deg: number;
			/** Positive = CCW. */
			sweep_angle_deg: number;
	  })
	| (SceneEntityCommon & {
			type: 'text';
			/** Anchor: center of the label (world mm). */
			x: number;
			y: number;
			z?: number;
			text: string;
			/** Nominal cap height (mm). */
			height_mm: number;
			rotation_deg?: number;
	  });

export interface SceneLayerV1 {
	id?: string;
	name?: string;
	entities: SceneEntity[];
}

export interface SceneV1 {
	schemaVersion: 1;
	kind: 'scene_v1';
	source_name?: string;
	/** Legacy: mirror of default group laser for older consumers. */
	laser?: LaserPropertiesV1;
	/** Named laser parameter sets; entities reference via `laser_group_id`. */
	laser_groups?: LaserGroupV1[];
	default_laser_group_id?: string;
	layers: SceneLayerV1[];
}

export function defaultLaserGroups(): LaserGroupV1[] {
	const base = defaultLaserProperties();
	return [
		{ id: DEFAULT_LASER_GROUP_ID, name: 'Default', laser: { ...base } },
		{
			id: CUT_LASER_GROUP_ID,
			name: 'Cut',
			laser: clampLaserProperties({ ...base, power_percent: 85, mark_speed_mm_s: 50 })
		},
		{
			id: MARK_LASER_GROUP_ID,
			name: 'Mark',
			laser: clampLaserProperties({ ...base, power_percent: 35, mark_speed_mm_s: 800 })
		}
	];
}

export function emptySceneV1(sourceName = 'editor'): SceneV1 {
	return {
		schemaVersion: 1,
		kind: 'scene_v1',
		source_name: sourceName,
		laser_groups: defaultLaserGroups(),
		default_laser_group_id: DEFAULT_LASER_GROUP_ID,
		layers: [{ id: '0', name: '0', entities: [] }]
	};
}

/** Ensure entity has `laser_group_id` (migration). */
export function withLaserGroupId(
	entity: SceneEntity | (Omit<SceneEntity, 'laser_group_id'> & { laser_group_id?: string }),
	fallbackId: string
): SceneEntity {
	const id =
		'laser_group_id' in entity && typeof entity.laser_group_id === 'string' && entity.laser_group_id
			? entity.laser_group_id
			: fallbackId;
	return { ...entity, laser_group_id: id } as SceneEntity;
}

/** Effective laser for an entity (override or group). */
export function effectiveLaserForEntity(
	entity: SceneEntity,
	groups: LaserGroupV1[],
	fallbackGroupId: string
): LaserPropertiesV1 {
	if (entity.laser) return clampLaserProperties(entity.laser);
	const gid = entity.laser_group_id ?? fallbackGroupId;
	const g = groups.find((x) => x.id === gid) ?? groups[0];
	return g ? clampLaserProperties(g.laser) : defaultLaserProperties();
}

export function buildSceneV1(params: {
	entities: SceneEntity[];
	laserGroups: LaserGroupV1[];
	defaultLaserGroupId: string;
	sourceName?: string;
}): SceneV1 {
	const groups = params.laserGroups.map((g) => ({
		...g,
		laser: clampLaserProperties(g.laser)
	}));
	const defId = params.defaultLaserGroupId;
	const def = groups.find((g) => g.id === defId);
	const entities = params.entities.map((e) => withLaserGroupId(e, defId));
	const scene: SceneV1 = {
		schemaVersion: 1,
		kind: 'scene_v1',
		source_name: params.sourceName ?? 'editor',
		laser_groups: groups,
		default_laser_group_id: defId,
		layers: [{ id: '0', name: '0', entities }]
	};
	if (def) scene.laser = def.laser;
	return scene;
}

/** @deprecated Use `buildSceneV1` with groups */
export function sceneFromEntities(
	entities: SceneEntity[],
	sourceName = 'editor',
	laser?: LaserPropertiesV1
): SceneV1 {
	const groups = defaultLaserGroups();
	if (laser) groups[0] = { ...groups[0]!, laser: clampLaserProperties(laser) };
	return buildSceneV1({
		entities,
		laserGroups: groups,
		defaultLaserGroupId: DEFAULT_LASER_GROUP_ID,
		sourceName
	});
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

/** Remap multi-selection indices after `reorderEntities(from, to, …)`. */
export function selectionIndicesAfterReorder(from: number, to: number, indices: number[]): number[] {
	const next = indices.map((sel) => selectionAfterReorder(from, to, sel)).filter((x): x is number => x !== null);
	return [...new Set(next)].sort((a, b) => a - b);
}

let _gidSeq = 1;
export function nextLaserGroupId(): string {
	return `g_${Date.now()}_${++_gidSeq}`;
}

let _jobGidSeq = 1;
export function nextJobGroupId(): string {
	return `jg_${Date.now()}_${++_jobGidSeq}`;
}

/** Next label like `Group 2` from existing `job_group_label` values. */
export function nextJobGroupLabel(list: SceneEntity[]): string {
	const prefix = 'Group ';
	let max = 0;
	for (const e of list) {
		const lab = e.job_group_label;
		if (lab?.startsWith(prefix)) {
			const n = parseInt(lab.slice(prefix.length).trim(), 10);
			if (!Number.isNaN(n)) max = Math.max(max, n);
		}
	}
	return `${prefix}${max + 1}`;
}

/** Contiguous run of the same `job_group_id` containing `index`, or null. */
export function contiguousJobGroupRun(
	entities: SceneEntity[],
	index: number
): { start: number; length: number; id: string } | null {
	if (index < 0 || index >= entities.length) return null;
	const id = entities[index]?.job_group_id;
	if (!id) return null;
	let start = index;
	while (start > 0 && entities[start - 1]?.job_group_id === id) start--;
	let end = index;
	while (end < entities.length - 1 && entities[end + 1]?.job_group_id === id) end++;
	const length = end - start + 1;
	if (length < 2) return null;
	return { start, length, id };
}

/**
 * Put selected entities in one contiguous block at `min(selected)` (same relative order as before).
 * Returns null if fewer than two indices selected.
 */
export function reorderSelectedIntoContiguousBlock(
	entities: SceneEntity[],
	selectedIndices: number[]
): { merged: SceneEntity[]; blockStart: number; blockLength: number } | null {
	const sorted = [...new Set(selectedIndices)].sort((a, b) => a - b);
	if (sorted.length < 2) return null;
	const block = sorted.map((i) => entities[i]!);
	const insertAt = sorted[0]!;
	const before: SceneEntity[] = [];
	const after: SceneEntity[] = [];
	for (let j = 0; j < entities.length; j++) {
		if (sorted.includes(j)) continue;
		if (j < insertAt) before.push(entities[j]!);
		else after.push(entities[j]!);
	}
	const merged = [...before, ...block, ...after];
	return { merged, blockStart: before.length, blockLength: block.length };
}

export function assignJobGroupToRange(
	entities: SceneEntity[],
	blockStart: number,
	blockLength: number,
	groupId: string,
	groupLabel: string
): SceneEntity[] {
	return entities.map((e, i) =>
		i >= blockStart && i < blockStart + blockLength
			? { ...e, job_group_id: groupId, job_group_label: groupLabel }
			: e
	) as SceneEntity[];
}

export function stripJobGroupFields(e: SceneEntity): SceneEntity {
	const o = { ...e } as Record<string, unknown>;
	delete o.job_group_id;
	delete o.job_group_label;
	return o as SceneEntity;
}

/** Remove job-group fields from every entity with `jobGroupId`. */
export function ungroupJobGroupId(entities: SceneEntity[], jobGroupId: string): SceneEntity[] {
	return entities.map((e) => (e.job_group_id === jobGroupId ? stripJobGroupFields(e) : e));
}

/**
 * After reordering, any `job_group_id` run shorter than 2 in array order loses grouping
 * (folder UI requires contiguous members).
 */
export function sanitizeFragmentedJobGroups(entities: SceneEntity[]): SceneEntity[] {
	const next = entities.map((e) => ({ ...e }));
	const n = next.length;
	let i = 0;
	while (i < n) {
		const gid = next[i]?.job_group_id;
		if (!gid) {
			i++;
			continue;
		}
		let j = i + 1;
		while (j < n && next[j]?.job_group_id === gid) j++;
		const len = j - i;
		if (len < 2) {
			for (let k = i; k < j; k++) {
				next[k] = stripJobGroupFields(next[k]!);
			}
		}
		i = j;
	}
	return next;
}

export function moveEntityBlockUp(entities: SceneEntity[], blockStart: number, blockLen: number): SceneEntity[] {
	if (blockStart <= 0 || blockLen < 1) return entities;
	const next = [...entities];
	const block = next.splice(blockStart, blockLen);
	next.splice(blockStart - 1, 0, ...block);
	return next;
}

export function moveEntityBlockDown(
	entities: SceneEntity[],
	blockStart: number,
	blockLen: number
): SceneEntity[] {
	const end = blockStart + blockLen;
	if (end >= entities.length || blockLen < 1) return entities;
	const next = [...entities];
	const block = next.splice(blockStart, blockLen);
	next.splice(blockStart + 1, 0, ...block);
	return next;
}

export function remapIndexAfterBlockMoveUp(
	blockStart: number,
	blockLen: number,
	index: number
): number {
	const lo = blockStart - 1;
	const hi = blockStart + blockLen - 1;
	if (index === lo) return lo + blockLen;
	if (index >= blockStart && index <= hi) return index - 1;
	return index;
}

export function remapIndexAfterBlockMoveDown(
	blockStart: number,
	blockLen: number,
	n: number,
	index: number
): number {
	const below = blockStart + blockLen;
	if (below >= n) return index;
	if (index >= blockStart && index < blockStart + blockLen) return index + 1;
	if (index === below) return blockStart;
	return index;
}

export function selectionIndicesAfterBlockMoveUp(
	blockStart: number,
	blockLen: number,
	indices: number[]
): number[] {
	const next = indices.map((i) => remapIndexAfterBlockMoveUp(blockStart, blockLen, i));
	return [...new Set(next)].sort((a, b) => a - b);
}

export function selectionIndicesAfterBlockMoveDown(
	blockStart: number,
	blockLen: number,
	n: number,
	indices: number[]
): number[] {
	const next = indices.map((i) => remapIndexAfterBlockMoveDown(blockStart, blockLen, n, i));
	return [...new Set(next)].sort((a, b) => a - b);
}

export function anchorIndexAfterBlockMoveUp(
	blockStart: number,
	blockLen: number,
	anchor: number | null
): number | null {
	if (anchor === null) return null;
	return remapIndexAfterBlockMoveUp(blockStart, blockLen, anchor);
}

export function anchorIndexAfterBlockMoveDown(
	blockStart: number,
	blockLen: number,
	n: number,
	anchor: number | null
): number | null {
	if (anchor === null) return null;
	return remapIndexAfterBlockMoveDown(blockStart, blockLen, n, anchor);
}

export type JobTreeSegment =
	| { kind: 'single'; index: number }
	| { kind: 'group'; id: string; label: string; indices: number[] };

/** Build job-tree rows: contiguous entities sharing `job_group_id` (≥2) become one group segment. */
export function buildJobTreeSegments(entities: SceneEntity[]): JobTreeSegment[] {
	const out: JobTreeSegment[] = [];
	let i = 0;
	while (i < entities.length) {
		const gid = entities[i]?.job_group_id;
		if (gid) {
			let j = i + 1;
			while (j < entities.length && entities[j]?.job_group_id === gid) j++;
			const indices = Array.from({ length: j - i }, (_, k) => i + k);
			if (indices.length >= 2) {
				out.push({
					kind: 'group',
					id: gid,
					label: entities[i]?.job_group_label ?? 'Group',
					indices
				});
				i = j;
				continue;
			}
		}
		out.push({ kind: 'single', index: i });
		i++;
	}
	return out;
}

/**
 * Next stable label for a new line or rectangle (e.g. `Line 3`). Scans existing `entity_label`
 * values of that kind so reordering the list does not renumber names.
 */
export function nextEntityLabelForKind(
	kind: 'line' | 'rect' | 'arc' | 'text',
	list: SceneEntity[]
): string {
	const prefix =
		kind === 'line' ? 'Line ' : kind === 'rect' ? 'Rect ' : kind === 'arc' ? 'Arc ' : 'Text ';
	let max = 0;
	for (const e of list) {
		if (e.type !== kind) continue;
		const lab = e.entity_label;
		if (lab?.startsWith(prefix)) {
			const n = parseInt(lab.slice(prefix.length).trim(), 10);
			if (!Number.isNaN(n)) max = Math.max(max, n);
		}
	}
	return `${prefix}${max + 1}`;
}
