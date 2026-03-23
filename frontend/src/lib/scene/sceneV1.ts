/** scene_v1 JSON — mirrors backend `scene_v1_to_parse_result` (docs: Phase H). */

import {
	clampLaserProperties,
	defaultLaserProperties,
	type LaserPropertiesV1
} from './laserProperties';

/** Default group id for new entities and legacy scenes without `laser_group_id`. */
export const DEFAULT_LASER_GROUP_ID = 'g0';

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
	return [
		{
			id: DEFAULT_LASER_GROUP_ID,
			name: 'Default',
			laser: defaultLaserProperties()
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

let _gidSeq = 1;
export function nextLaserGroupId(): string {
	return `g_${Date.now()}_${++_gidSeq}`;
}
