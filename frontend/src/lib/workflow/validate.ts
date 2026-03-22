import type { WorkflowDefinition, WorkflowStepKind } from './types';

const STEP_KINDS: WorkflowStepKind[] = [
	'connect',
	'parameters',
	'load-job',
	'run',
	'status',
	'custom'
];

function isRecord(v: unknown): v is Record<string, unknown> {
	return typeof v === 'object' && v !== null && !Array.isArray(v);
}

/** Lightweight structural validation (no JSON Schema engine in the bundle). */
export function assertValidWorkflow(data: unknown): WorkflowDefinition {
	if (!isRecord(data)) throw new Error('Workflow root must be an object');
	for (const key of ['id', 'version', 'title', 'steps'] as const) {
		if (!(key in data)) throw new Error(`Missing workflow field: ${key}`);
	}
	if (typeof data.id !== 'string' || !data.id) throw new Error('workflow.id must be a non-empty string');
	if (typeof data.version !== 'string') throw new Error('workflow.version must be a string');
	if (typeof data.title !== 'string') throw new Error('workflow.title must be a string');
	if (!Array.isArray(data.steps) || data.steps.length === 0) {
		throw new Error('workflow.steps must be a non-empty array');
	}
	const seen = new Set<string>();
	const steps = data.steps.map((raw, i) => {
		if (!isRecord(raw)) throw new Error(`Step ${i} must be an object`);
		if (typeof raw.id !== 'string' || !raw.id) throw new Error(`Step ${i}: id required`);
		if (seen.has(raw.id)) throw new Error(`Duplicate step id: ${raw.id}`);
		seen.add(raw.id);
		if (typeof raw.title !== 'string') throw new Error(`Step ${raw.id}: title required`);
		if (typeof raw.kind !== 'string' || !STEP_KINDS.includes(raw.kind as WorkflowStepKind)) {
			throw new Error(`Step ${raw.id}: invalid kind`);
		}
		return raw as unknown as WorkflowDefinition['steps'][number];
	});
	const def: WorkflowDefinition = {
		id: data.id,
		version: data.version,
		title: data.title,
		description: typeof data.description === 'string' ? data.description : undefined,
		steps
	};
	return def;
}
