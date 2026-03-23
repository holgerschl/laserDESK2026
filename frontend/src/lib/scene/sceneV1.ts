/** scene_v1 JSON — mirrors backend `scene_v1_to_parse_result` (docs: Phase H). */

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

export function sceneFromEntities(entities: SceneEntity[], sourceName = 'editor'): SceneV1 {
	return {
		schemaVersion: 1,
		kind: 'scene_v1',
		source_name: sourceName,
		layers: [{ id: '0', name: '0', entities: [...entities] }]
	};
}
