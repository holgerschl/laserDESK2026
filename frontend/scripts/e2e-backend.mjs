#!/usr/bin/env node
/**
 * Starts laserdesk_backend for Playwright. Set LASERDESK_BACKEND_EXE to override binary path.
 * Default: searches backend/build for Release/Debug laserdesk_backend (.exe on Windows).
 */
import { spawn } from 'node:child_process';
import { existsSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(__dirname, '..', '..');
const port = process.env.LASERDESK_PORT || '18080';

const candidates = [
	process.env.LASERDESK_BACKEND_EXE,
	path.join(repoRoot, 'backend', 'build', 'Release', 'laserdesk_backend.exe'),
	path.join(repoRoot, 'backend', 'build', 'Debug', 'laserdesk_backend.exe'),
	path.join(repoRoot, 'backend', 'build', 'laserdesk_backend.exe'),
	path.join(repoRoot, 'backend', 'build', 'Release', 'laserdesk_backend'),
	path.join(repoRoot, 'backend', 'build', 'Debug', 'laserdesk_backend'),
	path.join(repoRoot, 'backend', 'build', 'laserdesk_backend')
].filter(Boolean);

const exe = candidates.find((p) => p && existsSync(p));
if (!exe) {
	console.error(
		'[e2e-backend] laserdesk_backend not found. Build the backend (CMake) or set LASERDESK_BACKEND_EXE.'
	);
	process.exit(1);
}

const child = spawn(exe, ['--port', port], {
	stdio: 'inherit',
	cwd: repoRoot,
	env: { ...process.env, LASERDESK_PORT: port }
});

child.on('exit', (code, signal) => {
	if (signal) process.kill(process.pid, signal);
	else process.exit(code ?? 0);
});
