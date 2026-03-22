/** Mirrors docs/workflows/workflow-schema.json (subset used by the UI). */

export type WorkflowStepKind =
	| 'connect'
	| 'parameters'
	| 'load-job'
	| 'run'
	| 'status'
	| 'custom';

export interface ConnectOptions {
	defaultMode?: 'mock' | 'ethernet';
}

export interface ParameterField {
	id: string;
	label: string;
	type: 'string' | 'number' | 'boolean' | 'select';
	default?: unknown;
	options?: string[];
	apiBinding?: string;
}

export interface WorkflowStep {
	id: string;
	title: string;
	description?: string;
	kind: WorkflowStepKind;
	allowedActions?: string[];
	connectOptions?: ConnectOptions;
	parameterFields?: ParameterField[];
}

export interface WorkflowDefinition {
	id: string;
	version: string;
	title: string;
	description?: string;
	steps: WorkflowStep[];
}
