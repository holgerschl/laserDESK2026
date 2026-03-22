import { defineConfig, devices } from '@playwright/test';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

export default defineConfig({
	testDir: 'e2e',
	fullyParallel: false,
	forbidOnly: !!process.env.CI,
	retries: process.env.CI ? 1 : 0,
	workers: 1,
	reporter: 'list',
	use: {
		baseURL: 'http://127.0.0.1:5173',
		trace: 'on-first-retry'
	},
	projects: [{ name: 'chromium', use: { ...devices['Desktop Chrome'] } }],
	webServer: [
		{
			command: 'node scripts/e2e-backend.mjs',
			cwd: __dirname,
			url: 'http://127.0.0.1:18080/api/v1/health',
			reuseExistingServer: !process.env.CI,
			timeout: 120_000,
			env: { ...process.env, LASERDESK_PORT: '18080' }
		},
		{
			command: 'npm run dev -- --host 127.0.0.1 --port 5173',
			cwd: __dirname,
			url: 'http://127.0.0.1:5173',
			reuseExistingServer: !process.env.CI,
			timeout: 120_000,
			env: {
				...process.env,
				LASERDESK_BACKEND_URL: 'http://127.0.0.1:18080'
			}
		}
	]
});
